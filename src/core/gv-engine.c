/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * SECTION:gv-engine
 * @title: GvEngine
 * @short_description: The engine, where Goodvibes meets GStreamer
 *
 * Documentation of interest for this part:
 * - https://gstreamer.freedesktop.org/documentation/application-development/advanced/buffering.html
 * - https://gstreamer.freedesktop.org/documentation/tutorials/basic/streaming.html
 */

#include <glib-object.h>
#include <glib.h>
#include <gst/audio/streamvolume.h>
#include <gst/gst.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core-enum-types.h"
#include "core/gv-core-internal.h"
#include "core/gv-metadata.h"
#include "core/gv-streaminfo.h"

#include "core/gv-engine.h"

/* Uncomment to dump more stuff from GStreamer */

//#define DEBUG_GST_TAGS
//#define DEBUG_GST_ELEMENT_SETUP

/* Whether we prefer to ignore buffering messages while playing */
#define IGNORE_BUFFERING_MESSAGES

/*
 * Properties
 */

#define DEFAULT_VOLUME 100
#define DEFAULT_MUTE   FALSE

enum {
	/* Reserved */
	PROP_0,
	/* Properties - refer to class_init() for more details */
	PROP_PLAYBACK_STATE,
	PROP_STREAMINFO,
	PROP_METADATA,
	PROP_VOLUME,
	PROP_MUTE,
	PROP_PIPELINE_ENABLED,
	PROP_PIPELINE_STRING,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

enum {
	SIGNAL_SSL_FAILURE,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvEnginePrivate {
	/* GStreamer stuff */
	GstElement *playbin;
	GstBus *bus;
	/* Playbin state */
	gboolean buffering;
	GstState target_state;
	guint retry_count;
	guint retry_timeout_id;
	/* Current stream */
	gchar *uri;
	gchar *user_agent;
	/* Defaults */
	gchar *default_user_agent;
	/* Properties */
	GvEngineState state;
	GvStreaminfo *streaminfo;
	GvMetadata *metadata;
	guint volume;
	gboolean mute;
	gboolean pipeline_enabled;
	gchar *pipeline_string;
};

typedef struct _GvEnginePrivate GvEnginePrivate;

struct _GvEngine {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvEnginePrivate *priv;
};

G_DEFINE_TYPE_WITH_CODE(GvEngine, gv_engine, G_TYPE_OBJECT,
			G_ADD_PRIVATE(GvEngine)
			G_IMPLEMENT_INTERFACE(GV_TYPE_ERRORABLE, NULL))

/*
 * GStreamer helpers
 */

static void
set_gst_state(GstElement *playbin, GstState state)
{
	gboolean expect_success;
	GstStateChangeReturn ret;
	const gchar *state_name;
	const gchar *return_name;

	/* Based on the documentation of GstElement [1]:
	 *
	 *   State changes to GST_STATE_READY or GST_STATE_NULL never return
	 *   GST_STATE_CHANGE_ASYNC.
	 *
	 * [1]: https://gstreamer.freedesktop.org/documentation/gstreamer/gstelement.html
	 *      #gst_element_set_state
	 */
	expect_success = state <= GST_STATE_READY ? TRUE : FALSE;

	ret = gst_element_set_state(playbin, state);
	state_name = gst_element_state_get_name(state);
	return_name = gst_element_state_change_return_get_name(ret);

	if (ret == GST_STATE_CHANGE_FAILURE)
		WARNING("Failed to set playbin state to %s", state_name);
	else if (ret != GST_STATE_CHANGE_SUCCESS && expect_success == TRUE)
		WARNING("Set playbin state to %s, got unexpected %s", state_name, return_name);
	else
		DEBUG("Set playbin state to %s, got %s", state_name, return_name);
}

#if 0
static GstState
get_gst_state(GstElement *playbin)
{
	GstStateChangeReturn change_return;
	GstState state, pending;

	change_return = gst_element_get_state(playbin, &state, &pending, 100 * GST_MSECOND);

	switch (change_return) {
	case GST_STATE_CHANGE_SUCCESS:
		DEBUG("Getting gst state... success");
		break;
	case GST_STATE_CHANGE_ASYNC:
		DEBUG("Getting gst state... will change async");
		break;
	case GST_STATE_CHANGE_FAILURE:
		DEBUG("Getting gst state... failed!");
		break;
	case GST_STATE_CHANGE_NO_PREROLL:
		DEBUG("Getting gst state... no preroll");
		break;
	default:
		WARNING("Unhandled state change: %d", change_return);
		break;
	}

	DEBUG("Gst state '%s', pending '%s'",
	      gst_element_state_get_name(state),
	      gst_element_state_get_name(pending));

	return state;
}
#endif

/*
 * Private methods
 */

static void
gv_engine_reload_pipeline(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GstElement *playbin = priv->playbin;
	gboolean pipeline_enabled = priv->pipeline_enabled;
	const gchar *pipeline_string = priv->pipeline_string;
	GstElement *cur_audio_sink = NULL;
	GstElement *new_audio_sink = NULL;

	g_return_if_fail(playbin != NULL);

	/* Get current audio sink */
	g_object_get(playbin, "audio-sink", &cur_audio_sink, NULL);

	DEBUG("Current audio sink: %s", cur_audio_sink ? GST_ELEMENT_NAME(cur_audio_sink) : "null (default)");

	/* Create a new audio sink */
	if (pipeline_enabled == FALSE || pipeline_string == NULL) {
		new_audio_sink = NULL;
	} else {
		GError *err = NULL;

		new_audio_sink = gst_parse_launch(pipeline_string, &err);
		if (err) {
			WARNING("Failed to parse pipeline description: %s", err->message);
			gv_errorable_emit_error(GV_ERRORABLE(self),
						_("Failed to parse pipeline description"),
						err->message);
			g_error_free(err);
		}
	}

	DEBUG("New audio sink: %s", new_audio_sink ? GST_ELEMENT_NAME(new_audio_sink) : "null (default)");

	/* True when one of them is NULL */
	if (cur_audio_sink != new_audio_sink) {
		gv_engine_stop(self);

		if (new_audio_sink == NULL)
			INFO("Setting gst audio sink to default");
		else
			INFO("Setting gst audio sink from pipeline '%s'", pipeline_string);

		g_object_set(playbin, "audio-sink", new_audio_sink, NULL);
	}

	if (cur_audio_sink)
		gst_object_unref(cur_audio_sink);
}

/*
 * Property accessors
 */

GvEngineState
gv_engine_get_state(GvEngine *self)
{
	return self->priv->state;
}

static void
gv_engine_set_state(GvEngine *self, GvEngineState state)
{
	GvEnginePrivate *priv = self->priv;

	if (priv->state == state)
		return;

	priv->state = state;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PLAYBACK_STATE]);
}

GvStreaminfo *
gv_engine_get_streaminfo(GvEngine *self)
{
	return self->priv->streaminfo;
}

static void
gv_engine_update_streaminfo_from_element_setup(GvEngine *self, GstElement *element)
{
	GvEnginePrivate *priv = self->priv;
	gboolean notify = FALSE;

	if (priv->streaminfo == NULL) {
		priv->streaminfo = gv_streaminfo_new();
		notify = TRUE;
	}

	if (gv_streaminfo_update_from_element_setup(priv->streaminfo, element) == TRUE)
		notify = TRUE;

	if (notify)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAMINFO]);
}

static void
gv_engine_update_streaminfo_from_tags(GvEngine *self, GstTagList *taglist)
{
	GvEnginePrivate *priv = self->priv;
	gboolean notify = FALSE;

	if (priv->streaminfo == NULL) {
		priv->streaminfo = gv_streaminfo_new();
		notify = TRUE;
	}

	if (gv_streaminfo_update_from_gst_taglist(priv->streaminfo, taglist) == TRUE)
		notify = TRUE;

	if (notify)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAMINFO]);
}

static void
gv_engine_update_streaminfo_from_audio_pad(GvEngine *self, GstPad *pad)
{
	GvEnginePrivate *priv = self->priv;
	gboolean notify = FALSE;

	if (priv->streaminfo == NULL) {
		priv->streaminfo = gv_streaminfo_new();
		notify = TRUE;
	}

	if (gv_streaminfo_update_from_gst_audio_pad(priv->streaminfo, pad) == TRUE)
		notify = TRUE;

	if (notify)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAMINFO]);
}

static void
gv_engine_unset_streaminfo(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;

	if (priv->streaminfo == NULL)
		return;

	gv_clear_streaminfo(&priv->streaminfo);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAMINFO]);
}

GvMetadata *
gv_engine_get_metadata(GvEngine *self)
{
	return self->priv->metadata;
}

static void
gv_engine_update_metadata_from_tags(GvEngine *self, GstTagList *taglist)
{
	GvEnginePrivate *priv = self->priv;
	gboolean notify = FALSE;

	if (priv->metadata == NULL)
		priv->metadata = gv_metadata_new();

	notify = gv_metadata_update_from_gst_taglist(priv->metadata, taglist);

	/* We don't want empty metadata objects, it makes life complicated
	 * for consumers who will forever need to write this kind of code:
	 *
	 *     if (ptr && !gv_metadata_is_empty(ptr))
	 *         ...
	 */
	if (gv_metadata_is_empty(priv->metadata))
		gv_clear_metadata(&priv->metadata);

	if (notify)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_METADATA]);
}

static void
gv_engine_unset_metadata(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;

	if (priv->metadata == NULL)
		return;

	gv_clear_metadata(&priv->metadata);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_METADATA]);
}

guint
gv_engine_get_volume(GvEngine *self)
{
	return self->priv->volume;
}

void
gv_engine_set_volume(GvEngine *self, guint volume)
{
	GvEnginePrivate *priv = self->priv;
	gdouble gst_volume;

	if (volume > 100)
		volume = 100;

	if (priv->volume == volume)
		return;

	priv->volume = volume;

	gst_volume = (gdouble) volume / 100.0;
	gst_stream_volume_set_volume(GST_STREAM_VOLUME(priv->playbin),
				     GST_STREAM_VOLUME_FORMAT_CUBIC, gst_volume);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VOLUME]);
}

gboolean
gv_engine_get_mute(GvEngine *self)
{
	return self->priv->mute;
}

void
gv_engine_set_mute(GvEngine *self, gboolean mute)
{
	GvEnginePrivate *priv = self->priv;

	if (priv->mute == mute)
		return;

	priv->mute = mute;
	gst_stream_volume_set_mute(GST_STREAM_VOLUME(priv->playbin), mute);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MUTE]);
}

gboolean
gv_engine_get_pipeline_enabled(GvEngine *self)
{
	return self->priv->pipeline_enabled;
}

void
gv_engine_set_pipeline_enabled(GvEngine *self, gboolean enabled)
{
	GvEnginePrivate *priv = self->priv;

	if (priv->pipeline_enabled == enabled)
		return;

	priv->pipeline_enabled = enabled;

	gv_engine_reload_pipeline(self);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PIPELINE_ENABLED]);
}

const gchar *
gv_engine_get_pipeline_string(GvEngine *self)
{
	return self->priv->pipeline_string;
}

void
gv_engine_set_pipeline_string(GvEngine *self, const gchar *pipeline_string)
{
	GvEnginePrivate *priv = self->priv;

	if (!g_strcmp0(pipeline_string, ""))
		pipeline_string = NULL;

	if (!g_strcmp0(priv->pipeline_string, pipeline_string))
		return;

	g_free(priv->pipeline_string);
	priv->pipeline_string = g_strdup(pipeline_string);

	gv_engine_reload_pipeline(self);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PIPELINE_STRING]);
}

static void
gv_engine_get_property(GObject *object,
		       guint property_id,
		       GValue *value,
		       GParamSpec *pspec)
{
	GvEngine *self = GV_ENGINE(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_PLAYBACK_STATE:
		g_value_set_enum(value, gv_engine_get_state(self));
		break;
	case PROP_STREAMINFO:
		g_value_set_boxed(value, gv_engine_get_streaminfo(self));
		break;
	case PROP_METADATA:
		g_value_set_boxed(value, gv_engine_get_metadata(self));
		break;
	case PROP_VOLUME:
		g_value_set_uint(value, gv_engine_get_volume(self));
		break;
	case PROP_MUTE:
		g_value_set_boolean(value, gv_engine_get_mute(self));
		break;
	case PROP_PIPELINE_ENABLED:
		g_value_set_boolean(value, gv_engine_get_pipeline_enabled(self));
		break;
	case PROP_PIPELINE_STRING:
		g_value_set_string(value, gv_engine_get_pipeline_string(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_engine_set_property(GObject *object,
		       guint property_id,
		       const GValue *value,
		       GParamSpec *pspec)
{
	GvEngine *self = GV_ENGINE(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_VOLUME:
		gv_engine_set_volume(self, g_value_get_uint(value));
		break;
	case PROP_MUTE:
		gv_engine_set_mute(self, g_value_get_boolean(value));
		break;
	case PROP_PIPELINE_ENABLED:
		gv_engine_set_pipeline_enabled(self, g_value_get_boolean(value));
		break;
	case PROP_PIPELINE_STRING:
		gv_engine_set_pipeline_string(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Private methods
 */

static void
_stop_playback(GvEngine *self, gboolean reset_retry)
{
	GvEnginePrivate *priv = self->priv;
	GstElement *playbin = priv->playbin;

	/* To stop the playback, we can set it to either READY or NULL. The
	 * difference is that when we set it to READY, we'll receive a bunch of
	 * "state-changed" message, that we can use to update our own state.
	 * While if we set it to NULL, we won't receive any "state-changed"
	 * message, therefore we need to update our state manually.
	 *
	 * We go for the latter, as it's what has always been done in Goodvibes
	 * so far, and it's also a sure guarantee that will receive a
	 * "state-changed=ready" message when we'll want to start the playback
	 * (since we'll go from NULL to READY).
	 */

	if (reset_retry == TRUE) {
		priv->retry_count = 0;
		g_clear_handle_id(&priv->retry_timeout_id, g_source_remove);
	}
	priv->buffering = FALSE;
	priv->target_state = GST_STATE_NULL;
	set_gst_state(playbin, GST_STATE_NULL);
	gv_engine_set_state(self, GV_ENGINE_STATE_STOPPED);
}

#define stop_playback(self) _stop_playback(self, TRUE)

static void
_start_playback(GvEngine *self, gboolean force_stop)
{
	GvEnginePrivate *priv = self->priv;
	GstElement *playbin = priv->playbin;

	/* First, stop the playback, in case it's not stopped already. Then set
	 * the playbin state to PAUSED, so that it starts buffering data.  When
	 * buffering reaches 100%, we'll start playing for real. This is
	 * handled in the callback for buffering messages.
	 *
	 * It's also possible that there's no buffering, and this is handled
	 * in the callback for state-changed messages.
	 */

	if (force_stop == TRUE)
		_stop_playback(self, TRUE);
	priv->target_state = GST_STATE_PAUSED;
	set_gst_state(playbin, GST_STATE_PAUSED);
}

#define start_playback(self) _start_playback(self, TRUE)

static void
start_playback_for_real(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GstElement *playbin = priv->playbin;

	priv->target_state = GST_STATE_PLAYING;
	set_gst_state(playbin, GST_STATE_PLAYING);
}

static gboolean
when_timeout_retry(gpointer data)
{
	GvEngine *self = GV_ENGINE(data);
	GvEnginePrivate *priv = self->priv;

	/* Make sure we still want to play. Then play in a "special way",
	 * as we don't want to touch the retry counter.
	 */
	if (priv->target_state >= GST_STATE_PAUSED) {
		_stop_playback(self, FALSE);
		_start_playback(self, FALSE);
	}

	priv->retry_timeout_id = 0;
	return G_SOURCE_REMOVE;
}

static void
retry_playback(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	guint delay;

	/* We retry playback after there's been a failure of some sort.
	 * We don't know what kind of failure, and maybe the network is down,
	 * in such case we don't want to keep retrying in a wild loop.
	 * So we schedule the retry, and increase the delay each time.
	 */

	/* If retry was already scheduled, bail out */
	if (priv->retry_timeout_id != 0)
		return;

	/* Stop playback now to stop gst from retrying */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Increase retry count */
	priv->retry_count++;
	delay = priv->retry_count - 1;
	if (delay > 10)
		delay = 10;

	INFO("Restarting playback in %u seconds", delay);
	priv->retry_timeout_id = g_timeout_add_seconds(delay, when_timeout_retry, self);
}

/*
 * Public methods
 */

void
gv_engine_stop(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;

	stop_playback(self);

	gv_engine_unset_streaminfo(self);
	gv_engine_unset_metadata(self);

	g_clear_pointer(&priv->uri, g_free);
	g_clear_pointer(&priv->user_agent, g_free);
}

void
gv_engine_play(GvEngine *self, const gchar *uri, const gchar *user_agent)
{
	GvEnginePrivate *priv = self->priv;

	/* Ensure playback is stopped */
	stop_playback(self);

	/* Save uri and user-agent */
	g_assert(priv->uri == NULL);
	priv->uri = g_strdup(uri);
	g_assert(priv->user_agent == NULL);
	priv->user_agent = g_strdup(user_agent);

	INFO("Playing stream: %s", uri);

	/* Start playback */
	g_object_set(priv->playbin, "uri", uri, NULL);
	start_playback(self);
}

GvEngine *
gv_engine_new(void)
{
	return g_object_new(GV_TYPE_ENGINE, NULL);
}

/*
 * GStreamer playbin signal handlers
 */

static void
on_playbin_audio_pad_notify_caps(GstPad *pad,
				 GParamSpec *pspec,
				 GvEngine *self)
{
	/* WARNING! We're likely in the GStreamer streaming thread! */

	const gchar *property_name = g_param_spec_get_name(pspec);
	GstElement *playbin = self->priv->playbin;
	GstMessage *msg;

	TRACE("%p, %s, %p", pad, property_name, self);

	msg = gst_message_new_application(GST_OBJECT(playbin),
					  gst_structure_new_empty("audio-caps-changed"));
	gst_element_post_message(playbin, msg);
}

static void
on_playbin_element_setup(GstElement *playbin G_GNUC_UNUSED,
			 GstElement *element,
			 GvEngine *self)
{
	/* WARNING! We're likely in the GStreamer streaming thread! */

#ifdef DEBUG_GST_ELEMENT_SETUP
	gchar *element_name;
	element_name = gst_element_get_name(element);
	DEBUG("Element setup: %s", element_name);
	g_free(element_name);
#endif

	gv_engine_update_streaminfo_from_element_setup(self, element);
}

static void
on_playbin_source_setup(GstElement *playbin G_GNUC_UNUSED,
			GstElement *source,
			GvEngine *self)
{
	/* WARNING! We're likely in the GStreamer streaming thread! */

	GvEnginePrivate *priv = self->priv;
	const gchar *user_agent;

	user_agent = priv->user_agent;
	if (user_agent == NULL)
		user_agent = priv->default_user_agent;

	g_object_set(source, "user-agent", user_agent, NULL);
	DEBUG("Source setup: user-agent='%s'", user_agent);

#if 0
	// XXX ssl-strict needs to be fixed
	gboolean ssl_strict;
	ssl_strict = gv_station_get_insecure(station) ? FALSE : TRUE;
	g_object_set(source, "user-agent", user_agent, "ssl-strict", ssl_strict, NULL);
	DEBUG("Source setup: ssl-strict=%s, user-agent='%s'",
	      ssl_strict ? "true" : "false", user_agent);
#endif
}

/*
 * GStreamer bus signal handlers
 */

static void
on_bus_message_eos(GstBus *bus G_GNUC_UNUSED, GstMessage *msg G_GNUC_UNUSED, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;

	INFO("Gst bus EOS message");

	if (priv->target_state >= GST_STATE_PAUSED)
		/* Schedule playback restart */
		retry_playback(self);
	else
		/* Stop gst from spitting errors */
		stop_playback(self);

	/* Emit an error */
	//gv_errorable_emit_error(GV_ERRORABLE(self), _("End of stream"), NULL);
}

static void
on_bus_message_error(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GError *err;
	gchar *debug;

	/* Parse message */
	gst_message_parse_error(msg, &err, &debug);

	/* Display error */
	WARNING("Gst bus error msg: %s:%d: %s",
		g_quark_to_string(err->domain), err->code, err->message);
	WARNING("Gst bus error debug: %s", debug);

	/* Retry unless for some weird reason we're stopped */
	if (priv->target_state >= GST_STATE_PAUSED)
		retry_playback(self);
	else
		stop_playback(self);

	/* Cleanup */
	g_error_free(err);
	g_free(debug);

	/* Emit an error signal */
	//gv_errorable_emit_error(GV_ERRORABLE(self), err->message, NULL);

	/* Here are some gst error messages that I've dealt with so far.
	 *
	 * GST_RESOURCE_ERROR: GST_RESOURCE_ERROR_OPEN_READ (5)
	 *
	 * Secure connection setup failed.
	 *
	 *   The protocol is https, however the server failed to return a valid
	 *   certificate. Note that souphttpsrc has a 'ssl-strict' property that
	 *   default to true. Setting it to false is enough to play the stream.
	 *
	 *   See #128.
	 *
	 *
	 * GST_RESOURCE_ERROR: GST_RESOURCE_ERROR_NOT_FOUND (3)
	 *
	 * Could not resolve server name.
	 *
	 *   This might happen for different reasons:
	 *   - wrong url, the protocol (http, likely) is correct, but the name of
	 *     the server is wrong.
	 *   - name resolution is down.
	 *   - network is down.
	 *
	 *   To reproduce:
	 *   - in the url, set a wrong server name.
	 *   - just disconnect from the network.
	 *
	 * File Not Found
	 *
	 *   It means that the protocol (http, likely) is correct, the server
	 *   name as well, but the file is wrong or couldn't be found. I guess
	 *   the server could be reached and replied something like "file not found".
	 *
	 *   To reproduce: at the end of the url, set a wrong filename
	 *
	 * Not Available
	 *
	 *   Don't remember about this one...
	 *
	 *
	 * GST_RESOURCE_ERROR: GST_RESOURCE_ERROR_SEEK (11)
	 *
	 * Server does not support seeking
	 *
	 *   It means that gstreamer tried to seek back. It happens for some streams
	 *   (not all) if the computer is suspended for a while, then resumed. Upon
	 *   resume, gst tries to seek. Seen that with a mp3 stream.
	 *
	 *   To reproduce: suspend the computer then resume.
	 *
	 *
	 * GST_CORE_ERROR: GST_CORE_ERROR_MISSING_PLUGIN
	 *
	 * No URI handler implemented for ...
	 *
	 *   It means that the protocol is wrong or unsupported.
	 *
	 *   To reproduce: in the url, replace 'http' by some random stuff
	 *
	 * Your GStreamer installation is missing a plug-in.
	 *
	 *   It means that gstreamer doesn't know what to do with what it received
	 *   from the server. So it's likely that the server didn't send the stream
	 *   expected. And it's even more likely that the server returned an html
	 *   page, because that's what servers usually do.
	 *   Some cases where it can happen:
	 *   - wrong url, that exists but is not a stream.
	 *   - outgoing requests are blocked, and return an html page. For example,
	 *     you're connected to a public Wi-Fi that expects you to authenticate
	 *     or agree the terms of use, before allowing outgoing traffic.
	 *
	 *   To reproduce: in the url, remove anything after the server name
	 */
}

static void
on_bus_message_warning(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self G_GNUC_UNUSED)
{
	GError *warning;
	gchar *debug;

	/* Parse message */
	gst_message_parse_warning(msg, &warning, &debug);

	/* Display warning */
	DEBUG("Gst bus warning msg: %s:%d: %s",
	      g_quark_to_string(warning->domain), warning->code, warning->message);
	DEBUG("Gst bus warning debug : %s", debug);

	/* Cleanup */
	g_error_free(warning);
	g_free(debug);
}

static void
on_bus_message_info(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self G_GNUC_UNUSED)
{
	GError *info;
	gchar *debug;

	/* Parse message */
	gst_message_parse_info(msg, &info, &debug);

	/* Display info */
	DEBUG("Gst bus info msg: %s:%d: %s",
	      g_quark_to_string(info->domain), info->code, info->message);
	DEBUG("Gst bus info debug : %s", debug);

	/* Cleanup */
	g_error_free(info);
	g_free(debug);
}

#ifdef DEBUG_GST_TAGS
static void
tag_list_foreach_dump(const GstTagList *list, const gchar *tag,
		      gpointer data G_GNUC_UNUSED)
{
	gchar *str = NULL;

	switch (gst_tag_get_type(tag)) {
	case G_TYPE_STRING:
		gst_tag_list_get_string_index(list, tag, 0, &str);
		break;
	case G_TYPE_INT: {
		gint val;
		gst_tag_list_get_int_index(list, tag, 0, &val);
		str = g_strdup_printf("%d", val);
		break;
	}
	case G_TYPE_UINT: {
		guint val;
		gst_tag_list_get_uint_index(list, tag, 0, &val);
		str = g_strdup_printf("%u", val);
		break;
	}
	case G_TYPE_DOUBLE: {
		gdouble val;
		gst_tag_list_get_double_index(list, tag, 0, &val);
		str = g_strdup_printf("%lg", val);
		break;
	}
	case G_TYPE_BOOLEAN: {
		gboolean val;
		gst_tag_list_get_boolean_index(list, tag, 0, &val);
		str = g_strdup_printf("%s", val ? "true" : "false");
		break;
	}
	}

	DEBUG("%10s '%20s' (%d elem): %s",
	      g_type_name(gst_tag_get_type(tag)),
	      tag,
	      gst_tag_list_get_tag_size(list, tag),
	      str);
	g_free(str);
}
#endif /* DEBUG_GST_TAGS */

static void
on_bus_message_tag(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GstTagList *taglist = NULL;

	TRACE("... %s, %p", GST_MESSAGE_SRC_NAME(msg), self);

	gst_message_parse_tag(msg, &taglist);

#ifdef DEBUG_GST_TAGS
	DEBUG("->>- GST TAGLIST DUMP ->>-->>-->>-->>-->>-->>-->>-->>----");
	gst_tag_list_foreach(taglist, (GstTagForeachFunc) tag_list_foreach_dump, NULL);
	DEBUG("-<<--<<--<<--<<--<<--<<--<<--<<--<<--<<--<<--<<--<<--<<--");
#endif /* DEBUG_GST_TAGS */

	gv_engine_update_streaminfo_from_tags(self, taglist);
	gv_engine_update_metadata_from_tags(self, taglist);

	gst_tag_list_unref(taglist);
}

static void
on_bus_message_buffering(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	static gint prev_percent = 0;
	gint percent = 0;

	/* There are mostly two situtations: either buffering is 100% and we
	 * want to be playing,  either it's below and the pipeline should be
	 * paused.
	 *
	 * During the initialization phase (when we start playing a stream), we
	 * expect to receive plenty of buffering messages until the buffer gets
	 * full. The pipeline is PAUSED during all this phase,  and it's set to
	 * PLAY only after 100% is reached.
	 *
	 * Then, while we play the stream, it's not uncommon (it's even very
	 * common, from where I stand) to receive buffering messages with a
	 * buffer value below 100%. In this case, the documentation states very
	 * clearly that we should PAUSE the pipeline:
	 *
	 * * https://gstreamer.freedesktop.org/documentation/playback/playbin.html
	 *
	 *   Note that applications should keep/set the pipeline in the PAUSED
	 *   state when a BUFFERING message is received with a buffer percent
	 *   value < 100 and set the pipeline back to PLAYING state when a
	 *   BUFFERING message with a value of 100 percent is received (if
	 *   PLAYING is the desired state, that is).
	 *
	 * * https://gstreamer.freedesktop.org/documentation/gstreamer/gstmessage.html
	 *   #gst_message_new_buffering
	 *
	 *   When percent is < 100 the application should PAUSE a PLAYING
	 *   pipeline. When percent is 100, the application can set the pipeline
	 *   (back) to PLAYING.
	 *
	 * However, in practice, it doesn't yield the best result.
	 *
	 * Setting the pipeline to PAUSE interrupts the sound. In the best case
	 * it's a very short interruption, the buffer gets back to 100%
	 * immediately after setting the pipeline to PAUSED (which makes me
	 * think it's an issue with gstreamer, rather than a real buffering
	 * issue), but it still creates a glitch that is audible. I see that
	 * all the time with Radio Nova mp3 streams.
	 *
	 * In the worst case, it's a real interruption, like 1 second. I must
	 * admit that I didn't see that in a while though...
	 *
	 * So instead of setting the pipeline to PAUSED, we can just ignore the
	 * buffering messages during playback. Everything still works, and we
	 * don't interrupt the sound.
	 */

	/* Parse message */
	gst_message_parse_buffering(msg, &percent);

	/* We want to know what's going on, without being spammed */
	if (ABS(percent - prev_percent) > 20) {
		prev_percent = percent;
		DEBUG("Buffering (%3u %%)", percent);
	}

	/* Now let's handle the buffering value */
#ifdef IGNORE_BUFFERING_MESSAGES
	if (percent >= 100) {
		if (priv->target_state == GST_STATE_PAUSED) {
			DEBUG("Buffering complete, setting pipeline to PLAYING");
			start_playback_for_real(self);
		}
		priv->buffering = FALSE;
		prev_percent = percent;
	} else {
		if (priv->target_state == GST_STATE_PLAYING)
			DEBUG("Buffering < 100%%, ignore and keep playing");
		else
			priv->buffering = TRUE;
	}
#else
	if (percent >= 100) {
		if (priv->target_state == GST_STATE_PAUSED) {
			DEBUG("Buffering complete, setting pipeline to PLAYING");
			start_playback_for_real(self);
		} else if (priv->target_state == GST_STATE_PLAYING) {
			DEBUG("Done buffering, setting pipeline to PLAYING");
			set_gst_state(priv->playbin, GST_STATE_PLAYING);
		}
		priv->buffering = FALSE;
		prev_percent = percent;
	} else {
		if (priv->target_state == GST_STATE_PLAYING && priv->buffering == FALSE) {
			/* We were not buffering but PLAYING, PAUSE the pipeline */
			DEBUG("Buffering < 100%%, setting pipeline to PAUSED");
			set_gst_state(priv->playbin, GST_STATE_PAUSED);
		}
		priv->buffering = TRUE;
	}
#endif
}

static void
on_bus_message_state_changed(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GstState old, new, pending;

	/* Only care about playbin */
	if (GST_MESSAGE_SRC(msg) != GST_OBJECT_CAST(priv->playbin))
		return;

	/* Parse message */
	gst_message_parse_state_changed(msg, &old, &new, &pending);

	/* Some debug */
	DEBUG("Gst state changed: old: %s, new: %s, pending: %s",
	      gst_element_state_get_name(old),
	      gst_element_state_get_name(new),
	      gst_element_state_get_name(pending));

	/* Update our own state */
	switch (new) {
	case GST_STATE_NULL:
		gv_engine_set_state(self, GV_ENGINE_STATE_STOPPED);
		break;
	case GST_STATE_READY:
		if (priv->target_state >= GST_STATE_PAUSED)
			gv_engine_set_state(self, GV_ENGINE_STATE_CONNECTING);
		else
			gv_engine_set_state(self, GV_ENGINE_STATE_STOPPED);
		break;
	case GST_STATE_PAUSED:
		if (priv->buffering == TRUE)
			gv_engine_set_state(self, GV_ENGINE_STATE_BUFFERING);
		else
			gv_engine_set_state(self, GV_ENGINE_STATE_CONNECTING);
		break;
	case GST_STATE_PLAYING:
		gv_engine_set_state(self, GV_ENGINE_STATE_PLAYING);
		break;
	case GST_STATE_VOID_PENDING:
		/* Can't happen anyway */
		break;
	}

	/* Might want to start playback */
	if (new == GST_STATE_PAUSED && priv->target_state == GST_STATE_PAUSED) {
		if (priv->buffering == TRUE) {
			DEBUG("Pipeline is PREROLLED, waiting for buffering to finish");
		} else {
			DEBUG("Pipeline is PREROLLED, no buffering, let's start");
			start_playback_for_real(self);
		}
	}

	/* If playing, then it's time to reset the retry counter */
	if (new == GST_STATE_PLAYING)
		priv->retry_count = 0;
}

static void
on_bus_message_stream_start(GstBus *bus G_GNUC_UNUSED, GstMessage *msg,
			    GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GstElement *playbin = priv->playbin;
	GstPad *pad = NULL;

	TRACE("... %s, %p", GST_MESSAGE_SRC_NAME(msg), self);
	DEBUG("Stream started");

	g_signal_emit_by_name(playbin, "get-audio-pad", 0, &pad);
	if (pad == NULL) {
		DEBUG("No audio pad after stream started");
		return;
	}

	g_signal_connect_object(pad, "notify::caps",
				G_CALLBACK(on_playbin_audio_pad_notify_caps), self, 0);
	gv_engine_update_streaminfo_from_audio_pad(self, pad);
}

static void
on_bus_message_application(GstBus *bus G_GNUC_UNUSED, GstMessage *msg,
			   GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	const GstStructure *s;
	const gchar *msg_name;

	TRACE("... %s, %p", GST_MESSAGE_SRC_NAME(msg), self);

	s = gst_message_get_structure(msg);
	msg_name = gst_structure_get_name(s);
	g_return_if_fail(msg_name != NULL);

	if (!g_strcmp0(msg_name, "audio-caps-changed")) {
		GstElement *playbin = priv->playbin;
		GstPad *pad = NULL;

		g_signal_emit_by_name(playbin, "get-audio-pad", 0, &pad);
		gv_engine_update_streaminfo_from_audio_pad(self, pad);
	} else {
		WARNING("Unhandled application message %s", msg_name);
	}
}

/*
 * GObject methods
 */

static gchar *
make_default_user_agent(void)
{
	gchar *gst_version;
	gchar *user_agent;

	gst_version = gst_version_string();
	user_agent = g_strdup_printf("%s %s", gv_core_user_agent, gst_version);
	g_free(gst_version);

	return user_agent;
}

static void
gv_engine_finalize(GObject *object)
{
	GvEngine *self = GV_ENGINE(object);
	GvEnginePrivate *priv = self->priv;

	TRACE("%p", object);

	/* Remove pending operations */
	g_clear_handle_id(&priv->retry_timeout_id, g_source_remove);

	/* Stop playback, the hard way */
	gst_element_set_state(priv->playbin, GST_STATE_NULL);

	/* Unref the bus */
	gst_bus_remove_signal_watch(priv->bus);
	gst_object_unref(priv->bus);

	/* Unref the playbin */
	gst_object_unref(priv->playbin);

	/* Unref metadata */
	gv_clear_streaminfo(&priv->streaminfo);
	gv_clear_metadata(&priv->metadata);

	/* Free resources */
	g_free(priv->pipeline_string);
	g_free(priv->uri);
	g_free(priv->user_agent);
	g_free(priv->default_user_agent);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_engine, object);
}

static void
gv_engine_constructed(GObject *object)
{
	GvEngine *self = GV_ENGINE(object);
	GvEnginePrivate *priv = self->priv;
	GstElement *playbin;
	GstElement *fakesink;
	GstBus *bus;

	/* Initialize defaults */
	priv->default_user_agent = make_default_user_agent();

	/* Initialize properties */
	priv->volume = DEFAULT_VOLUME;
	priv->mute = DEFAULT_MUTE;
	priv->pipeline_enabled = FALSE;
	priv->pipeline_string = NULL;

	/* Check that GStreamer is initialized */
	g_assert(gst_is_initialized());

	/* Make the playbin - returns floating ref */
	playbin = gst_element_factory_make("playbin", "playbin");
	g_assert_nonnull(playbin);
	priv->playbin = g_object_ref_sink(playbin);

	/* Disable video - returns floating ref */
	fakesink = gst_element_factory_make("fakesink", "fakesink");
	g_assert_nonnull(fakesink);
	g_object_set(playbin, "video-sink", fakesink, NULL);

	/* Connect playbin signal handlers */
	g_signal_connect_object(playbin, "element-setup",
				G_CALLBACK(on_playbin_element_setup), self, 0);
	g_signal_connect_object(playbin, "source-setup",
				G_CALLBACK(on_playbin_source_setup), self, 0);

	/* Get a reference to the message bus - returns full ref */
	bus = gst_element_get_bus(playbin);
	g_assert_nonnull(bus);
	priv->bus = bus;

	/* Add a bus signal watch (so that 'message' signals are emitted) */
	gst_bus_add_signal_watch(bus);

	/* Connect bus signal handlers */
	g_signal_connect_object(bus, "message::eos",
				G_CALLBACK(on_bus_message_eos), self, 0);
	g_signal_connect_object(bus, "message::error",
				G_CALLBACK(on_bus_message_error), self, 0);
	g_signal_connect_object(bus, "message::warning",
				G_CALLBACK(on_bus_message_warning), self, 0);
	g_signal_connect_object(bus, "message::info",
				G_CALLBACK(on_bus_message_info), self, 0);
	g_signal_connect_object(bus, "message::tag",
				G_CALLBACK(on_bus_message_tag), self, 0);
	g_signal_connect_object(bus, "message::buffering",
				G_CALLBACK(on_bus_message_buffering), self, 0);
	g_signal_connect_object(bus, "message::state-changed",
				G_CALLBACK(on_bus_message_state_changed), self, 0);
	g_signal_connect_object(bus, "message::stream-start",
				G_CALLBACK(on_bus_message_stream_start), self, 0);
	g_signal_connect_object(bus, "message::application",
				G_CALLBACK(on_bus_message_application), self, 0);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_engine, object);
}

static void
gv_engine_init(GvEngine *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_engine_get_instance_private(self);
}

static void
gv_engine_class_init(GvEngineClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_engine_finalize;
	object_class->constructed = gv_engine_constructed;

	/* Properties */
	object_class->get_property = gv_engine_get_property;
	object_class->set_property = gv_engine_set_property;

	properties[PROP_PLAYBACK_STATE] =
		g_param_spec_enum("playback-state", "Playback state", NULL,
				  GV_TYPE_ENGINE_STATE,
				  GV_ENGINE_STATE_STOPPED,
				  GV_PARAM_READABLE);

	properties[PROP_STREAMINFO] =
		g_param_spec_boxed("streaminfo", "Stream information", NULL,
				   GV_TYPE_STREAMINFO,
				   GV_PARAM_READABLE);

	properties[PROP_METADATA] =
		g_param_spec_boxed("metadata", "Stream metadata", NULL,
				   GV_TYPE_METADATA,
				   GV_PARAM_READABLE);

	properties[PROP_VOLUME] =
		g_param_spec_uint("volume", "Volume in percent", NULL,
				  0, 100, DEFAULT_VOLUME,
				  GV_PARAM_READWRITE);

	properties[PROP_MUTE] =
		g_param_spec_boolean("mute", "Mute", NULL,
				     FALSE,
				     GV_PARAM_READWRITE);

	properties[PROP_PIPELINE_ENABLED] =
		g_param_spec_boolean("pipeline-enabled", "Enable custom pipeline", NULL,
				     FALSE,
				     GV_PARAM_READWRITE);

	properties[PROP_PIPELINE_STRING] =
		g_param_spec_string("pipeline-string", "Custom pipeline string", NULL, NULL,
				    GV_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_SSL_FAILURE] =
		g_signal_new("ssl-failure", G_TYPE_FROM_CLASS(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_STRING);
}
