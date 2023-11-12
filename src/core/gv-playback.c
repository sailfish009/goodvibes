/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023 Arnaud Rebillout
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

#include <glib-object.h>
#include <glib.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core-enum-types.h"
#include "core/gv-engine.h"
#include "core/gv-metadata.h"
#include "core/gv-station.h"
#include "core/gv-streaminfo.h"

#include "core/gv-playback.h"

// Beware that current station is going to be set/unset in both player and
// playback, both hold a reference -- is that really needed? Some thoughts:
// + player needs current station as a cursor in the station list, and
//   maybe nothing more... even wondering if that could be moved to the
//   station list itself, but OTOH it's not strictly needed now, and I
//   don't want to depart from the GList-style API that I currently have,
//   so maybe best to do nothing.
// + objects (feat, ui) now mostly watch GvPlayback, and that's where they
//   get the current station from. To avoid confusion:
//   + maybe player shouldn't export the current station as a property
//     anymore, otherwise callers don't know where to watch this property
//     from (player or playback?)
//   + however, it's also convenient to just watch Player, and get notified
//     of playing, and then fetch station (as does feat/gv-notification.c).
//     So maybe we can have both, but must ensure that they're in sync
//     (unit testing...)
//     + however, with a property comes notifications, so to keep things
//       simple, it would be better to NOT duplicate
//   + I also see parts of the code where, in reaction to playing=true, we
//     fetch the station details. Which make sense, if playing, what are we
//     playing is the next question. It also means: we must set the station
//     *before* changing the playing state (unit testing...)
// + maybe playback shouldn't export a method "_set_station()" as it's
//   supposed to be done via gv_player_...
//
// bad-certificate also comes from engine
//
// station state must go away


/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Construct-only properties */
	PROP_ENGINE,
	/* Engine mirrored properties */
	PROP_REDIRECTION_URI,
	PROP_STREAMINFO,
	PROP_METADATA,
	/* Properties */
	PROP_STATION,
	PROP_STATE,
	PROP_ERROR,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

enum {
	SIGNAL_BAD_CERTIFICATE,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvPlaybackPrivate {
	/* Construct-only properties */
	GvEngine *engine;
	/* Properties */
	GvStation *station;
	GvPlaybackState state;
	GvPlaybackError *error;
	/* Internal status */
	gboolean playback_on;
	guint retry_count;
	guint retry_timeout_id;
};

typedef struct _GvPlaybackPrivate GvPlaybackPrivate;

struct _GvPlayback {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvPlaybackPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvPlayback, gv_playback, G_TYPE_OBJECT)

/*
 * Playback state
 */

const gchar *
gv_playback_state_to_string(GvPlaybackState state)
{
	const gchar *str;

	switch (state) {
	case GV_PLAYBACK_STATE_STOPPED:
		str = _("Stopped");
		break;
	case GV_PLAYBACK_STATE_DOWNLOADING_PLAYLIST:
		str = _("Downloading playlist…");
		break;
	case GV_PLAYBACK_STATE_CONNECTING:
		str = _("Connecting…");
		break;
	case GV_PLAYBACK_STATE_BUFFERING:
		str = _("Buffering…");
		break;
	case GV_PLAYBACK_STATE_PLAYING:
		str = _("Playing");
		break;
	case GV_PLAYBACK_STATE_WAITING_RETRY:
		str = _("Retrying soon…");
		break;
	default:
		WARNING("Unhandled state: %d", state);
		str = _("Stopped");
		break;
	}

	return str;
}

/*
 * Playback error
 */

G_DEFINE_BOXED_TYPE(GvPlaybackError, gv_playback_error,
		    gv_playback_error_copy, gv_playback_error_free)

void
gv_playback_error_free(GvPlaybackError *self)
{
	g_return_if_fail(self != NULL);

	g_free(self->message);
	g_free(self->details);
	g_free(self);
}

GvPlaybackError *
gv_playback_error_copy(GvPlaybackError *self)
{
	GvPlaybackError *copy;

	g_return_val_if_fail(self != NULL, NULL);

	copy = gv_playback_error_new(self->message, self->details);

	return copy;
}

GvPlaybackError *
gv_playback_error_new(const gchar *message, const gchar *details)
{
	GvPlaybackError *self;

	self = g_new0(GvPlaybackError, 1);
	self->message = g_strdup(message);
	self->details = g_strdup(details);

	return self;
}

/*
 * Helpers
 */

static void
start_playback(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	if (priv->station != NULL)
		gv_station_play(priv->station);
}

static void
stop_playback(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	if (priv->station != NULL)
		gv_station_stop(priv->station);

	g_clear_handle_id(&priv->retry_timeout_id, g_source_remove);
	priv->retry_count = 0;
}

static gboolean
when_timeout_retry(gpointer data)
{
	GvPlayback *self = GV_PLAYBACK(data);
	GvPlaybackPrivate *priv = self->priv;

	if (priv->playback_on == TRUE)
		start_playback(self);

	priv->retry_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static void
retry_playback(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	guint delay;

	/* We retry playback after there's been a failure of some sort.
	 * We don't know what kind of failure, maybe the network is down,
	 * or the server is down, and in any case we don't want to retry
	 * asap. We need to wait a bit, and increase the waiting time for
	 * each retry.
	 */

	/* If a retry is already scheduled, bail out */
	if (priv->retry_timeout_id != 0)
		return;

	/* Increase the retry count */
	priv->retry_count++;
	delay = priv->retry_count - 1;
	if (delay > 10)
		delay = 10;

	INFO("Restarting playback in %u seconds", delay);
	priv->retry_timeout_id = g_timeout_add_seconds(delay, when_timeout_retry, self);
}

/*
 * Signal handlers & callbacks
 */

static void gv_playback_set_state(GvPlayback *self, GvPlaybackState value);
static void gv_playback_set_error(GvPlayback *self, const gchar *message, const gchar *details);

static void
on_station_notify(GvStation *station, GParamSpec *pspec, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", station, property_name, self);

	g_assert(station == priv->station);

	if (!g_strcmp0(property_name, "state")) {
		GvStationState station_state;
		GvPlaybackState playback_state;

		station_state = gv_station_get_state(priv->station);

		/* A change of state always invalidates any playback error */
		gv_playback_set_error(self, NULL, NULL);

		/* Map station state to playback state - trivial */
		switch (station_state) {
		case GV_STATION_STATE_STOPPED:
			playback_state = GV_PLAYBACK_STATE_STOPPED;
			gv_playback_set_state(self, playback_state);
			break;
		case GV_STATION_STATE_DOWNLOADING_PLAYLIST:
			playback_state = GV_PLAYBACK_STATE_DOWNLOADING_PLAYLIST;
			gv_playback_set_state(self, playback_state);
			break;
		case GV_STATION_STATE_PLAYING_STREAM:
			/* The playback state will be updated by the engine now */
			break;
		default:
			ERROR("Unhandled station state: %d", station_state);
			/* Program execution stops here */
			break;
		}
	}
}

static void
on_station_playlist_error(GvStation *station, const gchar *message, const gchar *details, GvPlayback *self)
{
	GvStationState station_state;

	TRACE("%p, %s, %s, %p", station, message, details, self);

	station_state = gv_station_get_state(station);
	g_assert(station_state == GV_STATION_STATE_STOPPED);

	gv_playback_set_error(self, message, details);
}

static void
on_station_bad_certificate(GvStation *station G_GNUC_UNUSED, GvPlayback *self)
{
	/* Just forward the signal ... */
	g_signal_emit(self, signals[SIGNAL_BAD_CERTIFICATE], 0);
}

static void
on_engine_notify(GvEngine *engine, GParamSpec *pspec, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", engine, property_name, self);

	if (!g_strcmp0(property_name, "redirection-uri")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_REDIRECTION_URI]);

	} else if (!g_strcmp0(property_name, "streaminfo")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAMINFO]);

	} else if (!g_strcmp0(property_name, "metadata")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_METADATA]);

	} else if (!g_strcmp0(property_name, "playback-state")) {
		GvEngineState engine_state;
		GvPlaybackState playback_state;

		engine_state = gv_engine_get_state(priv->engine);

		/* A change of state always invalidates any playback error,
		 * except when the new state is "stopped". */
		if (engine_state != GV_ENGINE_STATE_STOPPED)
			gv_playback_set_error(self, NULL, NULL);

		/* Map engine state to playback state - trivial */
		switch (engine_state) {
		case GV_ENGINE_STATE_STOPPED:
			if (priv->playback_on == TRUE)
				playback_state = GV_PLAYBACK_STATE_WAITING_RETRY;
			else
				playback_state = GV_PLAYBACK_STATE_STOPPED;
			break;
		case GV_ENGINE_STATE_CONNECTING:
			playback_state = GV_PLAYBACK_STATE_CONNECTING;
			break;
		case GV_ENGINE_STATE_BUFFERING:
			playback_state = GV_PLAYBACK_STATE_BUFFERING;
			break;
		case GV_ENGINE_STATE_PLAYING:
			playback_state = GV_PLAYBACK_STATE_PLAYING;
			break;
		default:
			ERROR("Unhandled engine state: %d", engine_state);
			/* Program execution stops here */
			break;
		}

		/* Set state */
		gv_playback_set_state(self, playback_state);
	}
}

static void
on_engine_bad_certificate(GvEngine *engine, GvPlayback *self)
{
	TRACE("%p, %p", engine, self);

	/* Just forward the signal */
	g_signal_emit(self, signals[SIGNAL_BAD_CERTIFICATE], 0);
}

static void
on_engine_end_of_stream(GvEngine *engine, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %p", engine, self);

	gv_playback_set_error(self, _("End of stream"), NULL);

	if (priv->playback_on == TRUE)
		retry_playback(self);
}

static void
on_engine_playback_error(GvEngine *engine, GError *error, const gchar *debug,
		GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %s, %s, %p", engine, error, debug, self);

	gv_playback_set_error(self, error->message, debug);

	if (priv->playback_on == TRUE)
		retry_playback(self);
}

/*
 * Property accessors - construct-only properties
 */

static void
gv_playback_set_engine(GvPlayback *self, GvEngine *engine)
{
	GvPlaybackPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert(priv->engine == NULL);
	g_assert(engine != NULL);
	priv->engine = g_object_ref(engine);

	/* Some signal handlers */
	g_signal_connect_object(engine, "notify",
			G_CALLBACK(on_engine_notify), self, 0);
	g_signal_connect_object(engine, "bad-certificate",
			G_CALLBACK(on_engine_bad_certificate), self, 0);
	g_signal_connect_object(engine, "end-of-stream",
			G_CALLBACK(on_engine_end_of_stream), self, 0);
	g_signal_connect_object(engine, "playback-error",
			G_CALLBACK(on_engine_playback_error), self, 0);
}

/*
 * Property accessors - engine mirrored properties
 * We don't notify here. It's done in the engine notify handler instead.
 */

const gchar *
gv_playback_get_redirection_uri(GvPlayback *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_redirection_uri(engine);
}

GvStreaminfo *
gv_playback_get_streaminfo(GvPlayback *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_streaminfo(engine);
}

GvMetadata *
gv_playback_get_metadata(GvPlayback *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_metadata(engine);
}

/*
 * Property accessors
 */

GvStation *
gv_playback_get_station(GvPlayback *self)
{
	return self->priv->station;
}

void
gv_playback_set_station(GvPlayback *self, GvStation *station)
{
	// XXX The whole thing needs rework

	GvPlaybackPrivate *priv = self->priv;

	if (station == priv->station)
		return;

	stop_playback(self);

	if (priv->station) {
		g_signal_handlers_disconnect_by_data(priv->station, self);
		g_object_unref(priv->station);
		priv->station = NULL;
	}

	if (station) {
		// XXX Why sink?
		priv->station = g_object_ref_sink(station);
		g_signal_connect_object(station, "notify",
				G_CALLBACK(on_station_notify), self, 0);
		g_signal_connect_object(station, "playlist-error",
				G_CALLBACK(on_station_playlist_error), self, 0);
		g_signal_connect_object(station, "bad-certificate",
				G_CALLBACK(on_station_bad_certificate), self, 0);
	}

	if (priv->playback_on == TRUE)
		start_playback(self);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION]);

	INFO("Station set to '%s'", station ? gv_station_get_name_or_uri(station) : NULL);
}

GvPlaybackState
gv_playback_get_state(GvPlayback *self)
{
	return self->priv->state;
}

static void
gv_playback_set_state(GvPlayback *self, GvPlaybackState state)
{
	GvPlaybackPrivate *priv = self->priv;

	if (priv->state == state)
		return;

	priv->state = state;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATE]);
}

GvPlaybackError *
gv_playback_get_error(GvPlayback *self)
{
	return self->priv->error;
}

static void
gv_playback_set_error(GvPlayback *self, const gchar *message, const gchar *details)
{
	GvPlaybackPrivate *priv = self->priv;

	if (priv->error == NULL && message == NULL && details == NULL)
		return;

	if (priv->error)
		gv_playback_error_free(priv->error);

	if (message == NULL && details == NULL)
		priv->error = NULL;
	else
		priv->error = gv_playback_error_new(message, details);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ERROR]);
}

static void
gv_playback_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GvPlayback *self = GV_PLAYBACK(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_STATION:
		g_value_set_object(value, gv_playback_get_station(self));
		break;
	case PROP_STATE:
		g_value_set_enum(value, gv_playback_get_state(self));
		break;
	case PROP_ERROR:
		g_value_set_object(value, gv_playback_get_error(self));
		break;
	case PROP_REDIRECTION_URI:
		g_value_set_string(value, gv_playback_get_redirection_uri(self));
		break;
	case PROP_STREAMINFO:
		g_value_set_boxed(value, gv_playback_get_streaminfo(self));
		break;
	case PROP_METADATA:
		g_value_set_boxed(value, gv_playback_get_metadata(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_playback_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GvPlayback *self = GV_PLAYBACK(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_ENGINE:
		gv_playback_set_engine(self, g_value_get_object(value));
		break;
	case PROP_STATION:
		gv_playback_set_station(self, g_value_get_object(value));
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
gv_playback_stop(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	/* Remember what we're doing */
	priv->playback_on = FALSE;

	/* Stop playback */
	stop_playback(self);
}

void
gv_playback_start(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	if (priv->station == NULL) {
		WARNING("No station set");
		return;
	}

	/* Remember what we're doing */
	priv->playback_on = TRUE;

	/* Start playback */
	start_playback(self);
}

GvPlayback *
gv_playback_new(GvEngine *engine)
{
	return g_object_new(GV_TYPE_PLAYBACK, "engine", engine, NULL);
}

/*
 * GObject methods
 */

static void
gv_playback_finalize(GObject *object)
{
	GvPlayback *self = GV_PLAYBACK(object);
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Remove pending operations */
	g_clear_handle_id(&priv->retry_timeout_id, g_source_remove);

	/* Free the playback error */
	if (priv->error)
		gv_playback_error_free(priv->error);

	/* Unref the current station */
	if (priv->station)
		g_object_unref(priv->station);

	/* Unref the engine */
	g_object_unref(priv->engine);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_playback, object);
}

static void
gv_playback_constructed(GObject *object)
{
	GvPlayback *self = GV_PLAYBACK(object);
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Ensure construct-only properties have been set */
	g_assert(priv->engine != NULL);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_playback, object);
}

static void
gv_playback_init(GvPlayback *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_playback_get_instance_private(self);
}

static void
gv_playback_class_init(GvPlaybackClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_playback_finalize;
	object_class->constructed = gv_playback_constructed;

	/* Properties */
	object_class->get_property = gv_playback_get_property;
	object_class->set_property = gv_playback_set_property;

	/* Construct-only properties */
	properties[PROP_ENGINE] =
		g_param_spec_object("engine", "Engine", NULL,
				    GV_TYPE_ENGINE,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	/* Properties */
	properties[PROP_STATION] =
		g_param_spec_object("station", "Current station", NULL,
				    GV_TYPE_STATION,
				    GV_PARAM_READWRITE);

	properties[PROP_STATE] =
		g_param_spec_enum("state", "State", NULL,
				  GV_TYPE_PLAYBACK_STATE,
				  GV_PLAYBACK_STATE_STOPPED,
				  GV_PARAM_READABLE);

	properties[PROP_ERROR] =
		g_param_spec_boxed("error", "Playback error", NULL,
				   GV_TYPE_PLAYBACK_ERROR,
				   GV_PARAM_READABLE);

	properties[PROP_REDIRECTION_URI] =
		g_param_spec_string("redirection-uri", "Redirection URI", NULL,
				    NULL,
				    GV_PARAM_READABLE);

	properties[PROP_STREAMINFO] =
		g_param_spec_boxed("streaminfo", "Stream information", NULL,
				   GV_TYPE_STREAMINFO,
				   GV_PARAM_READABLE);

	properties[PROP_METADATA] =
		g_param_spec_boxed("metadata", "Current metadata", NULL,
				   GV_TYPE_METADATA,
				   GV_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_BAD_CERTIFICATE] =
		g_signal_new("bad-certificate", G_OBJECT_CLASS_TYPE(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
