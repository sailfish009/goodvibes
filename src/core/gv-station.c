/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2020 Arnaud Rebillout
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

#include "framework/glib-object-additions.h"
#include "framework/gv-framework.h"
#include "core/gv-core-internal.h"
#include "core/gv-playlist.h"

#include "core/gv-station.h"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Set at construct-time */
	PROP_UID,
	/* Set by user - station definition */
	PROP_NAME,
	PROP_URI,
	/* Set by user - customization */
	PROP_USER_AGENT,
	/* Learnt along the way */
	PROP_STREAM_URIS,
	PROP_CHANNELS,
	PROP_CODEC,
	PROP_MAXIMUM_BITRATE,
	PROP_MINIMUM_BITRATE,
	PROP_NOMINAL_BITRATE,
	PROP_SAMPLE_RATE,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

enum {
	SIGNAL_PLAYLIST_DOWNLOADED,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvStationPrivate {
	/*
	 * Properties
	 */

	/* Set at construct-time */
	gchar  *uid;
	/* Set by user - station definition */
	gchar  *name;
	gchar  *uri;
	/* Set by user - customization */
	gchar  *user_agent;
	/* Learnt along the way */
	GSList *stream_uris;
	guint   channels;
	gchar  *codec;
	guint   maximum_bitrate;
	guint   minimum_bitrate;
	guint   nominal_bitrate;
	guint   sample_rate;
};

typedef struct _GvStationPrivate GvStationPrivate;

struct _GvStation {
	/* Parent instance structure */
	GInitiallyUnowned parent_instance;
	/* Private data */
	GvStationPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStation, gv_station, G_TYPE_INITIALLY_UNOWNED)

/*
 * Helpers
 */

static gpointer
copy_func_strdup(gconstpointer src, gpointer data G_GNUC_UNUSED)
{
	return g_strdup(src);
}

static void
gv_station_set_stream_uris(GvStation *self, GSList *list)
{
	GvStationPrivate *priv = self->priv;

	if (priv->stream_uris)
		g_slist_free_full(priv->stream_uris, g_free);

	priv->stream_uris = g_slist_copy_deep(list, copy_func_strdup, NULL);

	g_object_notify(G_OBJECT(self), "stream-uris");
}

static void
gv_station_set_stream_uri(GvStation *self, const gchar *uri)
{
	GSList *list = NULL;
	list = g_slist_append(list, g_strdup(uri));
	gv_station_set_stream_uris(self, list);
	g_slist_free_full(list, g_free);
}

/*
 * Signal handlers
 */

static void
on_playlist_downloaded(GvPlaylist *playlist,
                       GvStation  *self)
{
	GSList *streams;

	streams = gv_playlist_get_stream_list(playlist);
	gv_station_set_stream_uris(self, streams);

	g_object_unref(playlist);
}

/*
 * Property accessors
 */

const gchar *
gv_station_get_uid(GvStation *self)
{
	return self->priv->uid;
}

const gchar *
gv_station_get_name(GvStation *self)
{
	return self->priv->name;
}

void
gv_station_set_name(GvStation *self, const gchar *name)
{
	GvStationPrivate *priv = self->priv;

	/* What should be used when there's no name for the station ?
	 * NULL or empty string ? Let's settle the question here.
	 */
	if (name && *name == '\0')
		name = NULL;

	if (!g_strcmp0(priv->name, name))
		return;

	g_free(priv->name);
	priv->name = g_strdup(name);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_NAME]);
}

const gchar *
gv_station_get_uri(GvStation *self)
{
	return self->priv->uri;
}

void
gv_station_set_uri(GvStation *self, const gchar *uri)
{
	GvStationPrivate *priv = self->priv;

	/* Setting the uri to NULL is forbidden */
	if (uri == NULL) {
		if (priv->uri == NULL) {
			/* Construct-time set. Setting the uri to NULL
			 * at this moment is an error in the code.
			 */
			ERROR("Creating station with an empty uri");
			/* Program execution stops here */
		} else {
			/* User is trying to set the uri to null, we just
			 * silently discard the request.
			 */
			DEBUG("Trying to set station uri to null. Ignoring.");
			return;
		}
	}

	/* Set uri */
	if (!g_strcmp0(priv->uri, uri))
		return;

	g_free(priv->uri);
	priv->uri = g_strdup(uri);

	/* If this is not a playlist uri, then it's a stream uri */
	if (gv_playlist_get_format(uri) == GV_PLAYLIST_FORMAT_UNKNOWN)
		gv_station_set_stream_uri(self, uri);

	/* Notify */
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_URI]);
}

const gchar *
gv_station_get_name_or_uri(GvStation *self)
{
	GvStationPrivate *priv = self->priv;

	return priv->name ? priv->name : priv->uri;
}

const gchar *
gv_station_get_user_agent(GvStation *self)
{
	return self->priv->user_agent;
}

void
gv_station_set_user_agent(GvStation *self, const gchar *user_agent)
{
	GvStationPrivate *priv = self->priv;

	if (user_agent && user_agent[0] == '\0')
		user_agent = NULL;

	if (!g_strcmp0(priv->user_agent, user_agent))
		return;

	g_free(priv->user_agent);
	priv->user_agent = g_strdup(user_agent);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_USER_AGENT]);
}

GSList *
gv_station_get_stream_uris(GvStation *self)
{
	return self->priv->stream_uris;
}

const gchar *
gv_station_get_first_stream_uri(GvStation *self)
{
	GSList *uris;

	uris = self->priv->stream_uris;
	if (uris == NULL)
		return NULL;

	return (const gchar *) uris->data;
}

guint
gv_station_get_channels(GvStation *self)
{
	return self->priv->channels;
}

void
gv_station_set_channels(GvStation *self, guint channels)
{
	GvStationPrivate *priv = self->priv;

	if (priv->channels == channels)
		return;

	priv->channels = channels;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CHANNELS]);
}

const gchar *
gv_station_get_codec(GvStation *self)
{
	return self->priv->codec;
}

void
gv_station_set_codec(GvStation *self, const gchar *codec)
{
	GvStationPrivate *priv = self->priv;

	if (codec && codec[0] == '\0')
		codec = NULL;

	if (!g_strcmp0(priv->codec, codec))
		return;

	g_free(priv->codec);
	priv->codec = g_strdup(codec);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CODEC]);
}

guint
gv_station_get_maximum_bitrate(GvStation *self)
{
	return self->priv->maximum_bitrate;
}

void
gv_station_set_maximum_bitrate(GvStation *self, guint bitrate)
{
	GvStationPrivate *priv = self->priv;

	if (priv->maximum_bitrate == bitrate)
		return;

	priv->maximum_bitrate = bitrate;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MAXIMUM_BITRATE]);
}

guint
gv_station_get_minimum_bitrate(GvStation *self)
{
	return self->priv->minimum_bitrate;
}

void
gv_station_set_minimum_bitrate(GvStation *self, guint bitrate)
{
	GvStationPrivate *priv = self->priv;

	if (priv->minimum_bitrate == bitrate)
		return;

	priv->minimum_bitrate = bitrate;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MINIMUM_BITRATE]);
}

guint
gv_station_get_nominal_bitrate(GvStation *self)
{
	return self->priv->nominal_bitrate;
}

void
gv_station_set_nominal_bitrate(GvStation *self, guint bitrate)
{
	GvStationPrivate *priv = self->priv;

	if (priv->nominal_bitrate == bitrate)
		return;

	priv->nominal_bitrate = bitrate;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_NOMINAL_BITRATE]);
}

guint
gv_station_get_sample_rate(GvStation *self)
{
	return self->priv->sample_rate;
}

void
gv_station_set_sample_rate(GvStation *self, guint sample_rate)
{
	GvStationPrivate *priv = self->priv;

	if (priv->sample_rate == sample_rate)
		return;

	priv->sample_rate = sample_rate;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SAMPLE_RATE]);
}

static void
gv_station_get_property(GObject    *object,
                        guint       property_id,
                        GValue     *value G_GNUC_UNUSED,
                        GParamSpec *pspec)
{
	GvStation *self = GV_STATION(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_UID:
		g_value_set_string(value, gv_station_get_uid(self));
		break;
	case PROP_NAME:
		g_value_set_string(value, gv_station_get_name(self));
		break;
	case PROP_URI:
		g_value_set_string(value, gv_station_get_uri(self));
		break;
	case PROP_USER_AGENT:
		g_value_set_string(value, gv_station_get_user_agent(self));
		break;
	case PROP_STREAM_URIS:
		g_value_set_pointer(value, gv_station_get_stream_uris(self));
		break;
	case PROP_CHANNELS:
		g_value_set_uint(value, gv_station_get_channels(self));
		break;
	case PROP_CODEC:
		g_value_set_string(value, gv_station_get_codec(self));
		break;
	case PROP_MAXIMUM_BITRATE:
		g_value_set_uint(value, gv_station_get_maximum_bitrate(self));
		break;
	case PROP_MINIMUM_BITRATE:
		g_value_set_uint(value, gv_station_get_minimum_bitrate(self));
		break;
	case PROP_NOMINAL_BITRATE:
		g_value_set_uint(value, gv_station_get_nominal_bitrate(self));
		break;
	case PROP_SAMPLE_RATE:
		g_value_set_uint(value, gv_station_get_sample_rate(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_station_set_property(GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	GvStation *self = GV_STATION(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NAME:
		gv_station_set_name(self, g_value_get_string(value));
		break;
	case PROP_URI:
		gv_station_set_uri(self, g_value_get_string(value));
		break;
	case PROP_USER_AGENT:
		gv_station_set_user_agent(self, g_value_get_string(value));
		break;
	case PROP_CHANNELS:
		gv_station_set_channels(self, g_value_get_uint(value));
		break;
	case PROP_CODEC:
		gv_station_set_codec(self, g_value_get_string(value));
		break;
	case PROP_MAXIMUM_BITRATE:
		gv_station_set_maximum_bitrate(self, g_value_get_uint(value));
		break;
	case PROP_MINIMUM_BITRATE:
		gv_station_set_minimum_bitrate(self, g_value_get_uint(value));
		break;
	case PROP_NOMINAL_BITRATE:
		gv_station_set_nominal_bitrate(self, g_value_get_uint(value));
		break;
	case PROP_SAMPLE_RATE:
		gv_station_set_sample_rate(self, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

gboolean
gv_station_download_playlist(GvStation *self)
{
	GvStationPrivate *priv = self->priv;
	GvPlaylist *playlist;

	if (priv->uri == NULL) {
		WARNING("No uri to download");
		return FALSE;
	}

	if (gv_playlist_get_format(priv->uri) == GV_PLAYLIST_FORMAT_UNKNOWN) {
		WARNING("Uri doesn't seem to be a playlist");
		return FALSE;
	}

	/* No need to keep track of that, it's unreferenced in the callback */
	playlist = gv_playlist_new(priv->uri);
	g_signal_connect_object(playlist, "downloaded", G_CALLBACK(on_playlist_downloaded), self, 0);
	gv_playlist_download(playlist, priv->user_agent ? priv->user_agent : gv_core_user_agent);

	return TRUE;
}

gchar *
gv_station_make_name(GvStation *self, gboolean escape)
{
	GvStationPrivate *priv = self->priv;
	gchar *str;

	str = priv->name ? priv->name : priv->uri;

	if (escape)
		str = g_markup_escape_text(str, -1);
	else
		str = g_strdup(str);

	return str;
}

GvStation *
gv_station_new(const gchar *name, const gchar *uri)
{
	return g_object_new(GV_TYPE_STATION,
	                    "name", name,
	                    "uri", uri,
	                    NULL);
}

/*
 * GObject methods
 */

static void
gv_station_finalize(GObject *object)
{
	GvStationPrivate *priv = GV_STATION(object)->priv;

	TRACE("%p", object);

	/* Free any allocated resources */
	if (priv->stream_uris)
		g_slist_free_full(priv->stream_uris, g_free);

	g_free(priv->uid);
	g_free(priv->name);
	g_free(priv->uri);
	g_free(priv->user_agent);
	g_free(priv->codec);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station, object);
}

static void
gv_station_init(GvStation *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_get_instance_private(self);

	/* Initialize properties */
	self->priv->uid = g_strdup_printf("%p", self);
}

static void
gv_station_class_init(GvStationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_finalize;

	/* Properties */
	object_class->get_property = gv_station_get_property;
	object_class->set_property = gv_station_set_property;

	properties[PROP_UID] =
	        g_param_spec_string("uid", "UID", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_NAME] =
	        g_param_spec_string("name", "Name", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE |
	                            G_PARAM_CONSTRUCT);

	properties[PROP_URI] =
	        g_param_spec_string("uri", "Uri", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE |
	                            G_PARAM_CONSTRUCT);

	properties[PROP_USER_AGENT] =
	        g_param_spec_string("user-agent", "User agent", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_STREAM_URIS] =
	        g_param_spec_pointer("stream-uris", "Stream uris", NULL,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_CHANNELS] =
	        g_param_spec_uint("channels", "Number of channels", NULL,
				  0, G_MAXUINT, 0,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_CODEC] =
	        g_param_spec_string("codec", "Codec", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_MAXIMUM_BITRATE] =
	        g_param_spec_uint("maximum-bitrate", "Maximum bitrate (bits per second)", NULL,
	                          0, G_MAXUINT, 0,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_MINIMUM_BITRATE] =
	        g_param_spec_uint("minimum-bitrate", "Minimum bitrate (bits per second)", NULL,
	                          0, G_MAXUINT, 0,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_NOMINAL_BITRATE] =
	        g_param_spec_uint("nominal-bitrate", "Nominal bitrate (bits per second)", NULL,
	                          0, G_MAXUINT, 0,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_SAMPLE_RATE] =
	        g_param_spec_uint("sample-rate", "Sample rate (Hz)", NULL,
	                          0, G_MAXUINT, 0,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_PLAYLIST_DOWNLOADED] =
	        g_signal_new("playlist-downloaded", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
