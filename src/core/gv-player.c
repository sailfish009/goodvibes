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

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <math.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core-enum-types.h"
#include "core/gv-core-internal.h"
#include "core/gv-engine.h"
#include "core/gv-metadata.h"
#include "core/gv-station-list.h"
#include "core/gv-station.h"
#include "core/gv-streaminfo.h"

#include "core/gv-player.h"

/*
 * Properties
 */

#define DEFAULT_VOLUME	 100
#define DEFAULT_MUTE	 FALSE
#define DEFAULT_REPEAT	 FALSE
#define DEFAULT_SHUFFLE	 FALSE
#define DEFAULT_AUTOPLAY FALSE

enum {
	/* Reserved */
	PROP_0,
	/* Construct-only properties */
	PROP_ENGINE,
	PROP_STATION_LIST,
	/* Engine mirrored properties */
	PROP_STREAMINFO,
	PROP_METADATA,
	PROP_VOLUME,
	PROP_MUTE,
	PROP_PIPELINE_ENABLED,
	PROP_PIPELINE_STRING,
	/* Properties */
	PROP_PLAYBACK_STATE,
	PROP_REPEAT,
	PROP_SHUFFLE,
	PROP_AUTOPLAY,
	PROP_STATION,
	PROP_STATION_URI,
	PROP_PREV_STATION,
	PROP_NEXT_STATION,
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

typedef enum {
	GV_PLAYER_WISH_TO_STOP,
	GV_PLAYER_WISH_TO_PLAY,
} GvPlayerWish;

struct _GvPlayerPrivate {
	/* Construct-only properties */
	GvEngine *engine;
	GvStationList *station_list;
	/* Properties */
	GvPlaybackState state;
	gboolean repeat;
	gboolean shuffle;
	gboolean autoplay;
	/* Current station */
	GvStation *station;
	/* Wished state */
	GvPlayerWish wish;
};

typedef struct _GvPlayerPrivate GvPlayerPrivate;

struct _GvPlayer {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvPlayerPrivate *priv;
};

static void gv_player_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GvPlayer, gv_player, G_TYPE_OBJECT,
			G_ADD_PRIVATE(GvPlayer)
			G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
					      gv_player_configurable_interface_init)
			G_IMPLEMENT_INTERFACE(GV_TYPE_ERRORABLE, NULL))

/*
 * Playback
 */

const gchar *
gv_playback_state_to_string(GvPlaybackState state)
{
	const gchar *str;

	switch (state) {
	case GV_PLAYBACK_STATE_PLAYING:
		str = _("Playing");
		break;
	case GV_PLAYBACK_STATE_CONNECTING:
		str = _("Connecting…");
		break;
	case GV_PLAYBACK_STATE_BUFFERING:
		str = _("Buffering…");
		break;
	case GV_PLAYBACK_STATE_STOPPED:
	default:
		str = _("Stopped");
		break;
	}

	return str;
}

/*
 * Signal handlers
 */

static void gv_player_set_playback_state(GvPlayer *self, GvPlaybackState value);

static void
on_station_notify(GvStation *station,
		  GParamSpec *pspec,
		  GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", station, property_name, self);

	g_assert(station == priv->station);

	/* In any case, we notify if something was changed in the station */
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION]);
}

static void
on_station_ssl_failure(GvStation *station G_GNUC_UNUSED,
		       const gchar *uri,
		       GvPlayer *self)
{
	/* Just forward the signal ... */
	g_signal_emit(self, signals[SIGNAL_SSL_FAILURE], 0, uri);
}

static void
on_engine_notify(GvEngine *engine,
		 GParamSpec *pspec,
		 GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", engine, property_name, self);

	if (!g_strcmp0(property_name, "streaminfo")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAMINFO]);

	} else if (!g_strcmp0(property_name, "metadata")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_METADATA]);

	} else if (!g_strcmp0(property_name, "volume")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VOLUME]);

	} else if (!g_strcmp0(property_name, "mute")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MUTE]);

	} else if (!g_strcmp0(property_name, "pipeline-enabled")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PIPELINE_ENABLED]);

	} else if (!g_strcmp0(property_name, "pipeline-string")) {
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PIPELINE_STRING]);

	} else if (!g_strcmp0(property_name, "playback-state")) {
		GvEngineState engine_state;
		GvPlaybackState playback_state;

		engine_state = gv_engine_get_state(priv->engine);

		/* Map engine state to player state - trivial */
		switch (engine_state) {
		case GV_ENGINE_STATE_STOPPED:
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
		gv_player_set_playback_state(self, playback_state);
	}
}

static void
on_engine_error(GvEngine *engine G_GNUC_UNUSED,
		const gchar *error_string G_GNUC_UNUSED,
		GvPlayer *self)
{
	/* Whatever the error, just stop */
	gv_player_stop(self);
}

static void
on_engine_ssl_failure(GvEngine *engine G_GNUC_UNUSED,
		      const gchar *uri,
		      GvPlayer *self)
{
	/* Just forward the signal ... */
	g_signal_emit(self, signals[SIGNAL_SSL_FAILURE], 0, uri);
}

/*
 * Property accessors - construct-only properties
 */

static void
gv_player_set_engine(GvPlayer *self, GvEngine *engine)
{
	GvPlayerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_null(priv->engine);
	g_assert_nonnull(engine);
	priv->engine = g_object_ref(engine);

	/* Some signal handlers */
	g_signal_connect_object(engine, "notify", G_CALLBACK(on_engine_notify), self, 0);
	g_signal_connect_object(engine, "error", G_CALLBACK(on_engine_error), self, 0);
	g_signal_connect_object(engine, "ssl-failure", G_CALLBACK(on_engine_ssl_failure), self, 0);
}

static void
gv_player_set_station_list(GvPlayer *self, GvStationList *station_list)
{
	GvPlayerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_null(priv->station_list);
	g_assert_nonnull(station_list);
	priv->station_list = g_object_ref(station_list);
}

/*
 * Property accessors - engine mirrored properties
 * We don't notify here. It's done in the engine notify handler instead.
 */

GvStreaminfo *
gv_player_get_streaminfo(GvPlayer *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_streaminfo(engine);
}

GvMetadata *
gv_player_get_metadata(GvPlayer *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_metadata(engine);
}

guint
gv_player_get_volume(GvPlayer *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_volume(engine);
}

void
gv_player_set_volume(GvPlayer *self, guint volume)
{
	GvEngine *engine = self->priv->engine;

	gv_engine_set_volume(engine, volume);
}

void
gv_player_lower_volume(GvPlayer *self)
{
	guint volume;
	guint step = 5;

	volume = gv_player_get_volume(self);
	volume = volume > step ? volume - step : 0;

	gv_player_set_volume(self, volume);
}

void
gv_player_raise_volume(GvPlayer *self)
{
	guint volume;
	guint step = 5;

	volume = gv_player_get_volume(self);
	volume = volume < 100 - step ? volume + step : 100;

	gv_player_set_volume(self, volume);
}

gboolean
gv_player_get_mute(GvPlayer *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_mute(engine);
}

void
gv_player_set_mute(GvPlayer *self, gboolean mute)
{
	GvEngine *engine = self->priv->engine;

	gv_engine_set_mute(engine, mute);
}

void
gv_player_toggle_mute(GvPlayer *self)
{
	gboolean mute;

	mute = gv_player_get_mute(self);
	gv_player_set_mute(self, !mute);
}

gboolean
gv_player_get_pipeline_enabled(GvPlayer *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_pipeline_enabled(engine);
}

void
gv_player_set_pipeline_enabled(GvPlayer *self, gboolean enabled)
{
	GvEngine *engine = self->priv->engine;

	gv_engine_set_pipeline_enabled(engine, enabled);
}

const gchar *
gv_player_get_pipeline_string(GvPlayer *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_pipeline_string(engine);
}

void
gv_player_set_pipeline_string(GvPlayer *self, const gchar *pipeline_string)
{
	GvEngine *engine = self->priv->engine;

	gv_engine_set_pipeline_string(engine, pipeline_string);
}

/*
 * Property accessors - player properties
 */

GvPlaybackState
gv_player_get_playback_state(GvPlayer *self)
{
	return self->priv->state;
}

static void
gv_player_set_playback_state(GvPlayer *self, GvPlaybackState state)
{
	GvPlayerPrivate *priv = self->priv;

	if (priv->state == state)
		return;

	priv->state = state;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PLAYBACK_STATE]);
}

gboolean
gv_player_get_repeat(GvPlayer *self)
{
	return self->priv->repeat;
}

void
gv_player_set_repeat(GvPlayer *self, gboolean repeat)
{
	GvPlayerPrivate *priv = self->priv;

	if (priv->repeat == repeat)
		return;

	priv->repeat = repeat;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_REPEAT]);
}

gboolean
gv_player_get_shuffle(GvPlayer *self)
{
	return self->priv->shuffle;
}

void
gv_player_set_shuffle(GvPlayer *self, gboolean shuffle)
{
	GvPlayerPrivate *priv = self->priv;

	if (priv->shuffle == shuffle)
		return;

	priv->shuffle = shuffle;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SHUFFLE]);
}

gboolean
gv_player_get_autoplay(GvPlayer *self)
{
	return self->priv->autoplay;
}

void
gv_player_set_autoplay(GvPlayer *self, gboolean autoplay)
{
	GvPlayerPrivate *priv = self->priv;

	if (priv->autoplay == autoplay)
		return;

	priv->autoplay = autoplay;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AUTOPLAY]);
}

GvStation *
gv_player_get_station(GvPlayer *self)
{
	return self->priv->station;
}

GvStation *
gv_player_get_prev_station(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	return gv_station_list_prev(priv->station_list, priv->station,
				    priv->repeat, priv->shuffle);
}

GvStation *
gv_player_get_next_station(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	return gv_station_list_next(priv->station_list, priv->station,
				    priv->repeat, priv->shuffle);
}

static const gchar *
gv_player_get_station_uri(GvPlayer *self)
{
	GvStation *station;
	const gchar *station_uri;

	station = gv_player_get_station(self);
	if (station == NULL)
		return NULL;

	station_uri = gv_station_get_uri(station);
	return station_uri;
}

void
gv_player_set_station(GvPlayer *self, GvStation *station)
{
	GvPlayerPrivate *priv = self->priv;

	if (station == priv->station)
		return;

	if (priv->station) {
		gv_station_stop(priv->station);
		g_signal_handlers_disconnect_by_data(priv->station, self);
		g_object_unref(priv->station);
		priv->station = NULL;
	}

	if (station) {
		priv->station = g_object_ref_sink(station);
		g_signal_connect_object(station, "notify", G_CALLBACK(on_station_notify), self, 0);
		g_signal_connect_object(station, "ssl-failure", G_CALLBACK(on_station_ssl_failure), self, 0);
	}

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION]);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION_URI]);

	INFO("Station set to '%s'", station ? gv_station_get_name_or_uri(station) : NULL);
}

gboolean
gv_player_set_station_by_name(GvPlayer *self, const gchar *name)
{
	GvPlayerPrivate *priv = self->priv;
	GvStation *station = NULL;

	station = gv_station_list_find_by_name(priv->station_list, name);
	if (station == NULL) {
		DEBUG("Station name '%s' not found in station list", name);
		return FALSE;
	}

	gv_player_set_station(self, station);
	return TRUE;
}

gboolean
gv_player_set_station_by_uri(GvPlayer *self, const gchar *uri)
{
	GvPlayerPrivate *priv = self->priv;
	GvStation *station = NULL;

	/* Look for the station in the station list */
	station = gv_station_list_find_by_uri(priv->station_list, uri);
	if (station == NULL) {
		DEBUG("Station URI '%s' not found in station list", uri);
		return FALSE;
	}

	gv_player_set_station(self, station);
	return TRUE;
}

gboolean
gv_player_set_station_by_guessing(GvPlayer *self, const gchar *string)
{
	GvPlayerPrivate *priv = self->priv;
	GvStation *station = NULL;

	station = gv_station_list_find_by_guessing(priv->station_list, string);
	if (station == NULL) {
		DEBUG("'%s' not found in station list", string);
		return FALSE;
	}

	gv_player_set_station(self, station);
	return TRUE;
}

static void
gv_player_get_property(GObject *object,
		       guint property_id,
		       GValue *value,
		       GParamSpec *pspec)
{
	GvPlayer *self = GV_PLAYER(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_STREAMINFO:
		g_value_set_boxed(value, gv_player_get_streaminfo(self));
		break;
	case PROP_METADATA:
		g_value_set_boxed(value, gv_player_get_metadata(self));
		break;
	case PROP_VOLUME:
		g_value_set_uint(value, gv_player_get_volume(self));
		break;
	case PROP_MUTE:
		g_value_set_boolean(value, gv_player_get_mute(self));
		break;
	case PROP_PIPELINE_ENABLED:
		g_value_set_boolean(value, gv_player_get_pipeline_enabled(self));
		break;
	case PROP_PIPELINE_STRING:
		g_value_set_string(value, gv_player_get_pipeline_string(self));
		break;
	case PROP_PLAYBACK_STATE:
		g_value_set_enum(value, gv_player_get_playback_state(self));
		break;
	case PROP_REPEAT:
		g_value_set_boolean(value, gv_player_get_repeat(self));
		break;
	case PROP_SHUFFLE:
		g_value_set_boolean(value, gv_player_get_shuffle(self));
		break;
	case PROP_AUTOPLAY:
		g_value_set_boolean(value, gv_player_get_autoplay(self));
		break;
	case PROP_STATION:
		g_value_set_object(value, gv_player_get_station(self));
		break;
	case PROP_STATION_URI:
		g_value_set_string(value, gv_player_get_station_uri(self));
		break;
	case PROP_PREV_STATION:
		g_value_set_object(value, gv_player_get_prev_station(self));
		break;
	case PROP_NEXT_STATION:
		g_value_set_object(value, gv_player_get_next_station(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_player_set_property(GObject *object,
		       guint property_id,
		       const GValue *value,
		       GParamSpec *pspec)
{
	GvPlayer *self = GV_PLAYER(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_ENGINE:
		gv_player_set_engine(self, g_value_get_object(value));
		break;
	case PROP_STATION_LIST:
		gv_player_set_station_list(self, g_value_get_object(value));
		break;
	case PROP_VOLUME:
		gv_player_set_volume(self, g_value_get_uint(value));
		break;
	case PROP_MUTE:
		gv_player_set_mute(self, g_value_get_boolean(value));
		break;
	case PROP_PIPELINE_ENABLED:
		gv_player_set_pipeline_enabled(self, g_value_get_boolean(value));
		break;
	case PROP_PIPELINE_STRING:
		gv_player_set_pipeline_string(self, g_value_get_string(value));
		break;
	case PROP_REPEAT:
		gv_player_set_repeat(self, g_value_get_boolean(value));
		break;
	case PROP_SHUFFLE:
		gv_player_set_shuffle(self, g_value_get_boolean(value));
		break;
	case PROP_AUTOPLAY:
		gv_player_set_autoplay(self, g_value_get_boolean(value));
		break;
	case PROP_STATION:
		gv_player_set_station(self, g_value_get_object(value));
		break;
	case PROP_STATION_URI:
		gv_player_set_station_by_uri(self, g_value_get_string(value));
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
gv_player_stop(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	/* To remember what we're doing */
	priv->wish = GV_PLAYER_WISH_TO_STOP;

	/* Stop playing */
	if (priv->station != NULL)
		gv_station_stop(priv->station);
}

void
gv_player_play(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	/* If no station is set yet, take the first from the station list */
	if (priv->station == NULL) {
		GvStation *first_station;

		first_station = gv_station_list_first(priv->station_list);
		gv_player_set_station(self, first_station);
	}

	/* If there's still no station, return */
	if (priv->station == NULL)
		return;

	/* To remember what we're doing */
	priv->wish = GV_PLAYER_WISH_TO_PLAY;

	/* Let's play */
	gv_station_play(priv->station);
}

gboolean
gv_player_next(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;
	GvStation *station;

	station = gv_player_get_next_station(self);

	if (station == NULL)
		return FALSE;

	gv_player_set_station(self, station);

	if (priv->wish == GV_PLAYER_WISH_TO_PLAY)
		gv_player_play(self);

	return TRUE;
}

gboolean
gv_player_prev(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;
	GvStation *station;

	station = gv_player_get_prev_station(self);

	if (station == NULL)
		return FALSE;

	gv_player_set_station(self, station);

	if (priv->wish == GV_PLAYER_WISH_TO_PLAY)
		gv_player_play(self);

	return TRUE;
}

void
gv_player_toggle(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	switch (priv->wish) {
	case GV_PLAYER_WISH_TO_STOP:
		gv_player_play(self);
		break;
	case GV_PLAYER_WISH_TO_PLAY:
		gv_player_stop(self);
		break;
	default:
		ERROR("Invalid wish: %d", priv->wish);
		/* Program execution stops here */
		break;
	}
}

void
gv_player_go(GvPlayer *self, const gchar *string_to_play)
{
	GvPlayerPrivate *priv = self->priv;

	/* If we have no argument, we rely on 'autoplay' */
	if (string_to_play == NULL) {
		if (priv->autoplay) {
			INFO("Autoplay is enabled, let's play");
			gv_player_play(self);
		}
		return;
	}

	/* If we have an argument, it might be anything.
	 * At first, check if we find it in the station list.
	 */
	if (gv_player_set_station_by_guessing(self, string_to_play)) {
		INFO("'%s' found in station list, let's play", string_to_play);
		gv_player_play(self);
		return;
	}

	/* Otherwise, if it's a valid URI, try to play it */
	if (gv_is_uri_scheme_supported(string_to_play)) {
		GvStation *station;

		station = gv_station_new(NULL, string_to_play);
		gv_player_set_station(self, station);

		INFO("'%s' is a valid URI, let's play", string_to_play);
		gv_player_play(self);
		return;
	}

	/* That looks like an invalid string then */
	WARNING("'%s' is neither a known station or a valid URI", string_to_play);
	gv_errorable_emit_error(GV_ERRORABLE(self),
				_("'%s' is neither a known station or a valid URI"),
				string_to_play);
}

GvPlayer *
gv_player_new(GvEngine *engine, GvStationList *station_list)
{
	return g_object_new(GV_TYPE_PLAYER,
			    "engine", engine,
			    "station-list", station_list,
			    NULL);
}

/*
 * GvConfigurable interface
 */

static void
gv_player_configure(GvConfigurable *configurable)
{
	GvPlayer *self = GV_PLAYER(configurable);

	TRACE("%p", self);

	g_assert(gv_core_settings);
	g_settings_bind(gv_core_settings, "pipeline-enabled",
			self, "pipeline-enabled", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "pipeline-string",
			self, "pipeline-string", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "volume",
			self, "volume", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "mute",
			self, "mute", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "repeat",
			self, "repeat", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "shuffle",
			self, "shuffle", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "autoplay",
			self, "autoplay", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_core_settings, "station-uri",
			self, "station-uri", G_SETTINGS_BIND_DEFAULT);
}

static void
gv_player_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_player_configure;
}

/*
 * GObject methods
 */

static void
gv_player_finalize(GObject *object)
{
	GvPlayer *self = GV_PLAYER(object);
	GvPlayerPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Unref the current station */
	if (priv->station)
		g_object_unref(priv->station);

	/* Unref the station list */
	g_object_unref(priv->station_list);

	/* Unref the engine */
	g_object_unref(priv->engine);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_player, object);
}

static void
gv_player_constructed(GObject *object)
{
	GvPlayer *self = GV_PLAYER(object);
	GvPlayerPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Ensure construct-only properties have been set */
	g_assert_nonnull(priv->engine);
	g_assert_nonnull(priv->station_list);

	/* Initialize properties */
	priv->repeat = DEFAULT_REPEAT;
	priv->shuffle = DEFAULT_SHUFFLE;
	priv->autoplay = DEFAULT_AUTOPLAY;
	priv->station = NULL;

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_player, object);
}

static void
gv_player_init(GvPlayer *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_player_get_instance_private(self);
}

static void
gv_player_class_init(GvPlayerClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_player_finalize;
	object_class->constructed = gv_player_constructed;

	/* Properties */
	object_class->get_property = gv_player_get_property;
	object_class->set_property = gv_player_set_property;

	/* Construct-only properties */
	properties[PROP_ENGINE] =
		g_param_spec_object("engine", "Engine", NULL,
				    GV_TYPE_ENGINE,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_STATION_LIST] =
		g_param_spec_object("station-list", "Station list", NULL,
				    GV_TYPE_STATION_LIST,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	/* Engine mirrored properties */
	properties[PROP_STREAMINFO] =
		g_param_spec_boxed("streaminfo", "Stream information", NULL,
				   GV_TYPE_STREAMINFO,
				   GV_PARAM_READABLE);

	properties[PROP_METADATA] =
		g_param_spec_boxed("metadata", "Current metadata", NULL,
				   GV_TYPE_METADATA,
				   GV_PARAM_READABLE);

	properties[PROP_VOLUME] =
		g_param_spec_uint("volume", "Volume in percent", NULL,
				  0, 100, DEFAULT_VOLUME,
				  GV_PARAM_READWRITE);

	properties[PROP_MUTE] =
		g_param_spec_boolean("mute", "Mute", NULL,
				     DEFAULT_MUTE,
				     GV_PARAM_READWRITE);

	properties[PROP_PIPELINE_ENABLED] =
		g_param_spec_boolean("pipeline-enabled", "Enable custom pipeline", NULL,
				     FALSE,
				     GV_PARAM_READWRITE);

	properties[PROP_PIPELINE_STRING] =
		g_param_spec_string("pipeline-string", "Custom pipeline string", NULL,
				    NULL,
				    GV_PARAM_READWRITE);

	/* Player properties */
	properties[PROP_PLAYBACK_STATE] =
		g_param_spec_enum("playback-state", "Playback state", NULL,
				  GV_TYPE_PLAYBACK_STATE,
				  GV_PLAYBACK_STATE_STOPPED,
				  GV_PARAM_READABLE);

	properties[PROP_REPEAT] =
		g_param_spec_boolean("repeat", "Repeat", NULL,
				     DEFAULT_REPEAT,
				     GV_PARAM_READWRITE);

	properties[PROP_SHUFFLE] =
		g_param_spec_boolean("shuffle", "Shuffle", NULL,
				     DEFAULT_SHUFFLE,
				     GV_PARAM_READWRITE);

	properties[PROP_AUTOPLAY] =
		g_param_spec_boolean("autoplay", "Autoplay on startup", NULL,
				     DEFAULT_AUTOPLAY,
				     GV_PARAM_READWRITE);

	properties[PROP_STATION] =
		g_param_spec_object("station", "Current station", NULL,
				    GV_TYPE_STATION,
				    GV_PARAM_READWRITE);

	properties[PROP_STATION_URI] =
		g_param_spec_string("station-uri", "Current station URI",
				    "This is used only to save the current station in conf",
				    NULL,
				    GV_PARAM_READWRITE);

	properties[PROP_PREV_STATION] =
		g_param_spec_object("prev", "Previous station", NULL,
				    GV_TYPE_STATION,
				    GV_PARAM_READABLE);

	properties[PROP_NEXT_STATION] =
		g_param_spec_object("next", "Next station", NULL,
				    GV_TYPE_STATION,
				    GV_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_SSL_FAILURE] =
		g_signal_new("ssl-failure", G_TYPE_FROM_CLASS(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_STRING);
}
