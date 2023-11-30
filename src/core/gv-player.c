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
#include "core/gv-playback.h"
#include "core/gv-station-list.h"
#include "core/gv-station.h"

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
	PROP_PLAYBACK,
	PROP_STATION_LIST,
	/* Engine mirrored properties */
	PROP_VOLUME,
	PROP_MUTE,
	PROP_PIPELINE_ENABLED,
	PROP_PIPELINE_STRING,
	/* Properties */
	PROP_PLAYING,
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
 * GObject definitions
 */

struct _GvPlayerPrivate {
	/* Construct-only properties */
	GvEngine *engine;
	GvPlayback *playback;
	GvStationList *station_list;
	/* Properties */
	gboolean playing;
	gboolean repeat;
	gboolean shuffle;
	gboolean autoplay;
	GvStation *station;
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
					      gv_player_configurable_interface_init))

/*
 * Signal handlers
 */

typedef struct {
	const gchar *name;
	guint        id;
} PropertyMapping;

static const PropertyMapping engine_mappings[] = {
	{ "volume", PROP_VOLUME },
	{ "mute", PROP_MUTE },
	{ "pipeline-enabled", PROP_PIPELINE_ENABLED },
	{ "pipeline-string", PROP_PIPELINE_STRING },
	{ NULL, 0 },
};

static void
on_engine_notify(GvEngine *engine, GParamSpec *pspec, GvPlayer *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);
	const PropertyMapping *m;

	TRACE("%p, %s, %p", engine, property_name, self);

	for (m = engine_mappings; m->name != NULL; m++) {
		if (g_strcmp0(property_name, m->name) != 0)
			continue;
		g_object_notify_by_pspec(G_OBJECT(self), properties[m->id]);
		break;
	}
}

/*
 * Property accessors - construct-only properties
 */

static void
gv_player_set_engine(GvPlayer *self, GvEngine *engine)
{
	GvPlayerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert(priv->engine == NULL);
	g_assert(engine != NULL);
	priv->engine = g_object_ref(engine);

	/* Some signal handlers */
	g_signal_connect_object(engine, "notify", G_CALLBACK(on_engine_notify), self, 0);
}

static void
gv_player_set_playback(GvPlayer *self, GvPlayback *playback)
{
	GvPlayerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert(priv->playback == NULL);
	g_assert(playback != NULL);
	priv->playback = g_object_ref(playback);
}

static void
gv_player_set_station_list(GvPlayer *self, GvStationList *station_list)
{
	GvPlayerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert(priv->station_list == NULL);
	g_assert(station_list != NULL);
	priv->station_list = g_object_ref(station_list);
}

/*
 * Property accessors - engine mirrored properties
 * We don't notify here. It's done in the engine notify handler instead.
 */

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

gboolean
gv_player_get_playing(GvPlayer *self)
{
	return self->priv->playing;
}

static void
gv_player_set_playing(GvPlayer *self, gboolean playing)
{
	GvPlayerPrivate *priv = self->priv;

	if (priv->playing == playing)
		return;

	priv->playing = playing;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PLAYING]);
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

	g_clear_object(&priv->station);

	/* Station might not belong to the station list and hence be floating,
	 * in this case we just want to sink it, we don't want to increase the
	 * reference counter.
	 */
	if (station)
		priv->station = g_object_ref_sink(station);

	/* Update station for the playback as well */
	gv_playback_set_station(priv->playback, station);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION]);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION_URI]);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PREV_STATION]);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_NEXT_STATION]);

	DEBUG("Station set to '%s'", station ? gv_station_get_name_or_uri(station) : NULL);
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
	case PROP_PLAYING:
		g_value_set_boolean(value, gv_player_get_playing(self));
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
	case PROP_PLAYBACK:
		gv_player_set_playback(self, g_value_get_object(value));
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

	gv_player_set_playing(self, FALSE);
	gv_playback_stop(priv->playback);
}

void
gv_player_play(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	/* If no station is set yet, take the first from the station list */
	if (priv->station == NULL) {
		GvStation *station;

		station = gv_station_list_first(priv->station_list);
		gv_player_set_station(self, station);
	}

	/* If there's still no station, return */
	if (priv->station == NULL)
		return;

	gv_player_set_playing(self, TRUE);

	/* Start playback */
	gv_playback_start(priv->playback);
}

gboolean
gv_player_next(GvPlayer *self)
{
	GvStation *station;

	station = gv_player_get_next_station(self);

	if (station == NULL)
		return FALSE;

	gv_player_set_station(self, station);

	return TRUE;
}

gboolean
gv_player_prev(GvPlayer *self)
{
	GvStation *station;

	station = gv_player_get_prev_station(self);

	if (station == NULL)
		return FALSE;

	gv_player_set_station(self, station);

	return TRUE;
}

void
gv_player_toggle(GvPlayer *self)
{
	GvPlayerPrivate *priv = self->priv;

	if (priv->playing == TRUE)
		gv_player_stop(self);
	else
		gv_player_play(self);
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
	WARNING("Neither a known station nor a valid URI: %s", string_to_play);
}

GvPlayer *
gv_player_new(GvEngine *engine, GvPlayback *playback, GvStationList *station_list)
{
	return g_object_new(GV_TYPE_PLAYER,
			    "engine", engine,
			    "playback", playback,
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

	/* Unref the playback */
	g_object_unref(priv->playback);

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
	g_assert(priv->engine != NULL);
	g_assert(priv->playback != NULL);
	g_assert(priv->station_list != NULL);

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

	properties[PROP_PLAYBACK] =
		g_param_spec_object("playback", "Playback", NULL,
				    GV_TYPE_PLAYBACK,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_STATION_LIST] =
		g_param_spec_object("station-list", "Station list", NULL,
				    GV_TYPE_STATION_LIST,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	/* Engine mirrored properties */
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
	properties[PROP_PLAYING] =
		g_param_spec_boolean("playing", "Playing", NULL,
				     FALSE,
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
}
