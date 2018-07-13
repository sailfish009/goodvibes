/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2018 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gst/audio/streamvolume.h>

#include "framework/glib-object-additions.h"
#include "framework/gst-additions.h"
#include "framework/gv-framework.h"
#include "core/gv-core-enum-types.h"
#include "core/gv-core-internal.h"
#include "core/gv-metadata.h"
#include "core/gv-station.h"

#include "core/gv-engine.h"

/* Uncomment to dump more stuff from GStreamer */

//#define DEBUG_GST_TAGS
//#define DEBUG_GST_STATE_CHANGES

/*
 * Properties
 */

#define DEFAULT_VOLUME 100
#define DEFAULT_MUTE   FALSE

enum {
	/* Reserved */
	PROP_0,
	/* Properties - refer to class_init() for more details */
	PROP_STATE,
	PROP_BITRATE,
	PROP_STATION,
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
 * GObject definitions
 */

struct _GvEnginePrivate {
	/* GStreamer stuff */
	GstElement    *playbin;
	GstBus        *bus;
	/* Properties */
	GvEngineState  state;
	guint          bitrate;
	GvStation     *station;
	GvMetadata    *metadata;
	guint          volume;
	gboolean       mute;
	gboolean       pipeline_enabled;
	gchar         *pipeline_string;
	/* Retry on error with a delay */
	guint          error_count;
	guint          when_timeout_start_playback_id;
};

typedef struct _GvEnginePrivate GvEnginePrivate;

struct _GvEngine {
	/* Parent instance structure */
	GObject          parent_instance;
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
	const gchar *state_name = gst_element_state_get_name(state);
	GstStateChangeReturn change_return;

	change_return = gst_element_set_state(playbin, state);

	switch (change_return) {
	case GST_STATE_CHANGE_SUCCESS:
		DEBUG("Setting gst state '%s'... success", state_name);
		break;
	case GST_STATE_CHANGE_ASYNC:
		DEBUG("Setting gst state '%s'... will change async", state_name);
		break;
	case GST_STATE_CHANGE_FAILURE:
		/* This might happen if the uri is invalid */
		DEBUG("Setting gst state '%s'... failed !", state_name);
		break;
	case GST_STATE_CHANGE_NO_PREROLL:
		DEBUG("Setting gst state '%s'... no preroll", state_name);
		break;
	default:
		WARNING("Unhandled state change: %d", change_return);
		break;
	}
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
		DEBUG("Getting gst state... failed !");
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

static guint
taglist_get_nominal_bitrate(GstTagList *taglist)
{
	guint bitrate = 0;

	gst_tag_list_get_uint_index(taglist, GST_TAG_NOMINAL_BITRATE, 0, &bitrate);

	/* Should I divide by 1000 or 1024 ? */
	bitrate /= 1000;

	return bitrate;
}

static guint
taglist_get_bitrate(GstTagList *taglist)
{
	guint bitrate = 0;

	gst_tag_list_get_uint_index(taglist, GST_TAG_BITRATE, 0, &bitrate);

	/* Should I divide by 1000 or 1024 ? */
	bitrate /= 1000;

	return bitrate;
}

static GvMetadata *
taglist_to_metadata(GstTagList *taglist)
{
	GvMetadata *metadata;
	const gchar *artist = NULL;
	const gchar *title = NULL;
	const gchar *album = NULL;
	const gchar *genre = NULL;
	const gchar *comment = NULL;
	GDate *date = NULL;
	gchar *year = NULL;

	/* Get info from taglist */
	gst_tag_list_peek_string_index(taglist, GST_TAG_ARTIST, 0, &artist);
	gst_tag_list_peek_string_index(taglist, GST_TAG_TITLE, 0, &title);
	gst_tag_list_peek_string_index(taglist, GST_TAG_ALBUM, 0, &album);
	gst_tag_list_peek_string_index(taglist, GST_TAG_GENRE, 0, &genre);
	gst_tag_list_peek_string_index(taglist, GST_TAG_COMMENT, 0, &comment);
	gst_tag_list_get_date_index   (taglist, GST_TAG_DATE, 0, &date);
	if (date && g_date_valid(date))
		year = g_strdup_printf("%d", g_date_get_year(date));

	/* Create new metadata object */
	metadata = g_object_new(GV_TYPE_METADATA,
	                        "artist", artist,
	                        "title", title,
	                        "album", album,
	                        "genre", genre,
	                        "year", year,
	                        "comment", comment,
	                        NULL);

	/* Freedom for the braves */
	g_free(year);
	if (date)
		g_date_free(date);

	return metadata;
}


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

	DEBUG("Current audio sink: %s", cur_audio_sink ? GST_ELEMENT_NAME(cur_audio_sink) :
	      "null (default)");

	/* Create a new audio sink */
	if (pipeline_enabled == FALSE || pipeline_string == NULL) {
		new_audio_sink = NULL;
	} else {
		GError *err = NULL;

		new_audio_sink = gst_parse_launch(pipeline_string, &err);
		if (err) {
			WARNING("Failed to parse pipeline description: %s", err->message);
			gv_errorable_emit_error(GV_ERRORABLE(self), _("%s: %s"),
			                        _("Failed to parse pipeline description"),
			                        err->message);
			g_error_free(err);
		}
	}

	DEBUG("New audio sink: %s", new_audio_sink ? GST_ELEMENT_NAME(new_audio_sink) :
	      "null (default)");

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
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATE]);
}

guint
gv_engine_get_bitrate(GvEngine *self)
{
	return self->priv->bitrate;
}

static void
gv_engine_set_bitrate(GvEngine *self, guint bitrate)
{
	GvEnginePrivate *priv = self->priv;

	if (priv->bitrate == bitrate)
		return;

	priv->bitrate = bitrate;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_BITRATE]);
}

GvStation *
gv_engine_get_station(GvEngine *self)
{
	return self->priv->station;
}

static void
gv_engine_set_station(GvEngine *self, GvStation *station)
{
	GvEnginePrivate *priv = self->priv;

	if (g_set_object(&priv->station, station))
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION]);
}

GvMetadata *
gv_engine_get_metadata(GvEngine *self)
{
	return self->priv->metadata;
}

static void
gv_engine_set_metadata(GvEngine *self, GvMetadata *metadata)
{
	GvEnginePrivate *priv = self->priv;

	/* Compare content */
	if (priv->metadata && metadata &&
	    gv_metadata_is_equal(priv->metadata, metadata)) {
		DEBUG("Metadata identical, ignoring...");
		return;
	}

	/* Assign */
	if (g_set_object(&priv->metadata, metadata))
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
gv_engine_get_property(GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
	GvEngine *self = GV_ENGINE(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_STATE:
		g_value_set_enum(value, gv_engine_get_state(self));
		break;
	case PROP_BITRATE:
		g_value_set_uint(value, gv_engine_get_bitrate(self));
		break;
	case PROP_STATION:
		g_value_set_object(value, gv_engine_get_station(self));
		break;
	case PROP_METADATA:
		g_value_set_object(value, gv_engine_get_metadata(self));
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
gv_engine_set_property(GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
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
 * Public methods
 */

void
gv_engine_play(GvEngine *self, GvStation *station)
{
	GvEnginePrivate *priv = self->priv;
	const gchar *station_stream_uri;

	g_return_if_fail(station != NULL);

	/* Station must have a stream uri */
	station_stream_uri = gv_station_get_first_stream_uri(station);
	if (station_stream_uri == NULL) {
		WARNING("Station '%s' has no stream uri",
		        gv_station_get_name_or_uri(station));
		return;
	}

	/* Cleanup error handling */
	priv->error_count = 0;
	if (priv->when_timeout_start_playback_id) {
		g_source_remove(priv->when_timeout_start_playback_id);
		priv->when_timeout_start_playback_id = 0;
	}

	/* Set station */
	gv_engine_set_station(self, station);

	/* Clear metadata */
	gv_engine_set_metadata(self, NULL);

	/* According to the doc:
	 *
	 * > State changes to GST_STATE_READY or GST_STATE_NULL never return
	 * > GST_STATE_CHANGE_ASYNC.
	 *
	 * https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/
	 * GstElement.html#gst-element-set-state
	 */

	/* Ensure playback is stopped */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Set the stream uri */
	g_object_set(priv->playbin, "uri", station_stream_uri, NULL);

	/* Go to the ready stop (not sure it's needed) */
	set_gst_state(priv->playbin, GST_STATE_READY);

	/* Set gst state to PAUSE, so that the playbin starts buffering data.
	 * Playback will start as soon as buffering is finished.
	 */
	set_gst_state(priv->playbin, GST_STATE_PAUSED);
	gv_engine_set_state(self, GV_ENGINE_STATE_CONNECTING);
}

void
gv_engine_stop(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;

	/* Cleanup error handling */
	priv->error_count = 0;
	if (priv->when_timeout_start_playback_id) {
		g_source_remove(priv->when_timeout_start_playback_id);
		priv->when_timeout_start_playback_id = 0;
	}

	/* Radical way to stop: set state to NULL */
	set_gst_state(priv->playbin, GST_STATE_NULL);
	gv_engine_set_state(self, GV_ENGINE_STATE_STOPPED);
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
on_playbin_source_setup(GstElement *playbin G_GNUC_UNUSED,
                        GstElement *source,
                        GvEngine   *self)
{
	GvEnginePrivate *priv = self->priv;
	GvStation *station = priv->station;
	static gchar *default_user_agent;
	const gchar *user_agent;

	if (default_user_agent == NULL) {
		gchar *gst_version;

		gst_version = gst_version_string();
		default_user_agent = g_strdup_printf("%s %s", gv_core_user_agent, gst_version);
		g_free(gst_version);
	}

	user_agent = default_user_agent;

	if (station) {
		const gchar *station_user_agent;

		station_user_agent = gv_station_get_user_agent(station);
		if (station_user_agent)
			user_agent = station_user_agent;
	}

	g_object_set(source, "user-agent", user_agent, NULL);
	DEBUG("Source setup with user-agent '%s'", user_agent);
}

/*
 * GStreamer bus signal handlers
 */

static gboolean
when_timeout_start_playback(gpointer data)
{
	GvEngine *self = GV_ENGINE(data);
	GvEnginePrivate *priv = self->priv;

	if (self->priv->state != GV_ENGINE_STATE_STOPPED) {
		set_gst_state(priv->playbin, GST_STATE_NULL);
		set_gst_state(priv->playbin, GST_STATE_READY);
		set_gst_state(priv->playbin, GST_STATE_PAUSED);
		gv_engine_set_state(self, GV_ENGINE_STATE_CONNECTING);
	}

	priv->when_timeout_start_playback_id = 0;
	return G_SOURCE_REMOVE;
}

static void
retry_playback(GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	guint delay;

	/* We retry playback after there's been a failure of some sort.
	 * We don't know what kind of failure, and maybe the network is down,
	 * in such case we don't want to keep retrying in a wild loop. So the
	 * strategy here is to start retrying with a minimal delay, and increment
	 * the delay with the failure count.
	 */

	if (priv->when_timeout_start_playback_id)
		return;

	g_assert(priv->error_count > 0);
	delay = priv->error_count - 1;
	if (delay > 10)
		delay = 10;

	INFO("Restarting playback in %u seconds", delay);
	priv->when_timeout_start_playback_id =
	        g_timeout_add_seconds(delay, when_timeout_start_playback, self);
}

static gboolean
on_bus_message_eos(GstBus *bus G_GNUC_UNUSED, GstMessage *msg G_GNUC_UNUSED, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;

	priv->error_count++;

	WARNING("Gst eos message");

	/* Stop immediately otherwise gst keeps on spitting errors */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Restart playback if needed */
	if (self->priv->state != GV_ENGINE_STATE_STOPPED)
		retry_playback(self);

	/* Emit an error */
	//gv_errorable_emit_error(GV_ERRORABLE(self), "%s", _("End of stream"));

	return TRUE;
}

static gboolean
on_bus_message_error(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GError *error;
	gchar  *debug;

	priv->error_count++;

	/* Parse message */
	gst_message_parse_error(msg, &error, &debug);

	/* Display error */
	WARNING("Gst bus error msg: %s:%d: %s",
	        g_quark_to_string(error->domain), error->code, error->message);
	WARNING("Gst bus error debug: %s", debug);

	/* Cleanup */
	g_error_free(error);
	g_free(debug);

	/* Stop immediately otherwise gst keeps on spitting errors */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Restart playback on error */
	if (self->priv->state != GV_ENGINE_STATE_STOPPED)
		retry_playback(self);

	/* Emit an error signal */
	//gv_errorable_emit_error(GV_ERRORABLE(self), "GStreamer error: %s", error->message);

	return TRUE;

	/* Here are some gst error messages that I've dealt with so far.
	 *
	 * GST_RESOURCE_ERROR: GST_RESOURCE_ERROR_NOT_FOUND
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
	 *   It means that the protocol (http, lilely) is correct, the server
	 *   name as well, but the file is wrong or couldn't be found. I guess
	 *   the server could be reached and replied something like "file not found".
	 *
	 *   To reproduce: at the end of the url, set a wrong filename
	 *
	 * Not Available
	 *
	 *   Don't remember about this one...
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

static gboolean
on_bus_message_warning(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self G_GNUC_UNUSED)
{
	GError *warning;
	gchar  *debug;

	/* Parse message */
	gst_message_parse_warning(msg, &warning, &debug);

	/* Display warning */
	DEBUG("Gst bus warning msg: %s:%d: %s",
	      g_quark_to_string(warning->domain), warning->code, warning->message);
	DEBUG("Gst bus warning debug : %s", debug);

	/* Cleanup */
	g_error_free(warning);
	g_free(debug);

	return TRUE;
}

static gboolean
on_bus_message_info(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self G_GNUC_UNUSED)
{
	GError *info;
	gchar  *debug;

	/* Parse message */
	gst_message_parse_info(msg, &info, &debug);

	/* Display info */
	DEBUG("Gst bus info msg: %s:%d: %s",
	      g_quark_to_string(info->domain), info->code, info->message);
	DEBUG("Gst bus info debug : %s", debug);

	/* Cleanup */
	g_error_free(info);
	g_free(debug);

	return TRUE;
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

static gboolean
on_bus_message_tag(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	GvMetadata *metadata;
	GstTagList *taglist = NULL;
	const gchar *tag_title = NULL;

	TRACE("... %s, %p", GST_OBJECT_NAME(msg->src), self);

	/* Parse message */
	gst_message_parse_tag(msg, &taglist);

#ifdef DEBUG_GST_TAGS
	/* Dumping may be needed to debug */
	DEBUG("-- Dumping taglist...");
	gst_tag_list_foreach(taglist, (GstTagForeachFunc) tag_list_foreach_dump, NULL);
	DEBUG("-- Done --");
#endif /* DEBUG_GST_TAGS */

	/* Get nominal bitrate */
	guint nominal_bitrate = taglist_get_nominal_bitrate(taglist);
	if (nominal_bitrate > 0) {
		GvStation *station = priv->station;

		if (station)
			gv_station_set_nominal_bitrate(station, nominal_bitrate);
	}

	/* Get bitrate */
	gv_engine_set_bitrate(self, taglist_get_bitrate(taglist));

	/* Tags can be quite noisy, so let's cut it short.
	 * From my experience, 'title' is the most important field,
	 * and it's likely that it's the only one filled, containing
	 * everything (title, artist and more).
	 * So, we require this field to be filled. If it's not, this
	 * metadata is considered to be noise, and discarded.
	 */
	gst_tag_list_peek_string_index(taglist, GST_TAG_TITLE, 0, &tag_title);
	if (tag_title == NULL) {
		DEBUG("No 'title' field in the tag list, discarding");
		goto taglist_unref;
	}

	/* Turn taglist into metadata and assign it */
	metadata = taglist_to_metadata(taglist);
	gv_engine_set_metadata(self, metadata);
	g_object_unref(metadata);

taglist_unref:
	/* Unref taglist */
	gst_tag_list_unref(taglist);

	return TRUE;
}

static gboolean
on_bus_message_buffering(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, GvEngine *self)
{
	GvEnginePrivate *priv = self->priv;
	static gint prev_percent = 0;
	gint percent = 0;

	/* Handle the buffering message. Some documentation:
	 * https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/
	 * GstMessage.html#gst-message-new-buffering
	 */

	/* Parse message */
	gst_message_parse_buffering(msg, &percent);

	/* Display buffering steps 20 by 20 */
	if (ABS(percent - prev_percent) > 20) {
		prev_percent = percent;
		DEBUG("Buffering (%3u %%)", percent);
	}

	/* Now, let's react according to our current state */
	switch (priv->state) {
	case GV_ENGINE_STATE_STOPPED:
		/* This shouldn't happen */
		WARNING("Received 'bus buffering' while stopped");
		break;

	case GV_ENGINE_STATE_CONNECTING:
		/* We successfully connected ! */
		gv_engine_set_state(self, GV_ENGINE_STATE_BUFFERING);

		/* NO BREAK HERE !
		 * This is to handle the (very special) case where the first
		 * buffering message received is already 100%. I'm not sure this
		 * can happen, but it doesn't hurt to be ready.
		 *
		 * To avoid warnings, the next line could be:
		 * __attribute__((fallthrough));
		 *
		 * However this is gcc-7 only, and I don't know about clang.
		 * So we use a more portable (and more magic) marker comment.
		 */
		// fall through

	case GV_ENGINE_STATE_BUFFERING:
		/* When buffering complete, start playing */
		if (percent >= 100) {
			DEBUG("Buffering complete, starting playback");
			set_gst_state(priv->playbin, GST_STATE_PLAYING);
			gv_engine_set_state(self, GV_ENGINE_STATE_PLAYING);
			priv->error_count = 0;
		}
		break;

	case GV_ENGINE_STATE_PLAYING:
		/* In case buffering is < 100%, according to the documentation,
		 * we should pause. However, more than often, I constantly
		 * receive 'buffering < 100%' messages. In such cases, following
		 * the doc and pausing/playing makes constantly cuts the sound.
		 * While ignoring the messages works just fine, do let's ignore
		 * for now.
		 */
		if (percent < 100) {
			DEBUG("Buffering < 100%%, ignoring instead of setting to pause");
			//set_gst_state(priv->playbin, GST_STATE_PAUSED);
			//gv_engine_set_state(self, GV_ENGINE_STATE_BUFFERING);
		}
		break;

	default:
		WARNING("Unhandled engine state %d", priv->state);
	}

	return TRUE;
}

static gboolean
on_bus_message_state_changed(GstBus *bus G_GNUC_UNUSED, GstMessage *msg,
                             GvEngine *self G_GNUC_UNUSED)
{
#ifdef DEBUG_GST_STATE_CHANGES
	GstState old, new, pending;

	/* Parse message */
	gst_message_parse_state_changed(msg, &old, &new, &pending);

	/* Just used for debug */
	DEBUG("Gst state changed: old: %s, new: %s, pending: %s",
	      gst_element_state_get_name(old),
	      gst_element_state_get_name(new),
	      gst_element_state_get_name(pending));
#else
	(void) msg;
#endif

	return TRUE;
}

/*
 * GObject methods
 */

static void
gv_engine_finalize(GObject *object)
{
	GvEngine *self = GV_ENGINE(object);
	GvEnginePrivate *priv = self->priv;

	TRACE("%p", object);

	/* Remove pending operations */
	if (priv->when_timeout_start_playback_id)
		g_source_remove(priv->when_timeout_start_playback_id);

	/* Stop playback */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Unref the bus */
	gst_bus_remove_signal_watch(priv->bus);
	gst_object_unref(priv->bus);

	/* Unref the playbin */
	gst_object_unref(priv->playbin);

	/* Unref metadata */
	g_clear_object(&priv->station);
	g_clear_object(&priv->metadata);

	/* Free resources */
	g_free(priv->pipeline_string);

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

	/* Initialize properties */
	priv->volume = DEFAULT_VOLUME;
	priv->mute   = DEFAULT_MUTE;
	priv->pipeline_enabled = FALSE;
	priv->pipeline_string  = NULL;

	/* GStreamer must be initialized, let's check that */
	g_assert(gst_is_initialized());

	/* Make the playbin - returns floating ref */
	playbin = gst_element_factory_make("playbin", "playbin");
	g_assert_nonnull(playbin);
	priv->playbin = g_object_ref_sink(playbin);

	/* Connect playbin signal handlers */
	g_signal_connect_object(playbin, "source-setup", G_CALLBACK(on_playbin_source_setup), self, 0);

	/* Disable video - returns floating ref */
	fakesink = gst_element_factory_make("fakesink", "fakesink");
	g_assert_nonnull(fakesink);
	g_object_set(playbin, "video-sink", fakesink, NULL);

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

	properties[PROP_STATE] =
	        g_param_spec_enum("state", "Playback state", NULL,
	                          GV_TYPE_ENGINE_STATE,
	                          GV_ENGINE_STATE_STOPPED,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_BITRATE] =
	        g_param_spec_uint("bitrate", "Bitrate", NULL,
	                          0, G_MAXUINT, 0,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_STATION] =
	        g_param_spec_object("station", "Current station", NULL,
	                            GV_TYPE_STATION,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_METADATA] =
	        g_param_spec_object("metadata", "Current metadata", NULL,
	                            GV_TYPE_METADATA,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_VOLUME] =
	        g_param_spec_uint("volume", "Volume in percent", NULL,
	                          0, 100, DEFAULT_VOLUME,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_MUTE] =
	        g_param_spec_boolean("mute", "Mute", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_PIPELINE_ENABLED] =
	        g_param_spec_boolean("pipeline-enabled", "Enable custom pipeline", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_PIPELINE_STRING] =
	        g_param_spec_string("pipeline-string", "Custom pipeline string", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
