/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023-2024 Arnaud Rebillout
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
#include "core/gv-playlist.h"
#include "core/gv-station.h"
#include "core/gv-streaminfo.h"
#include "core/playlist-utils.h"

#include "core/gv-playback.h"

#define RETRY_MAX_DELAY 60  // seconds

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Construct-only properties */
	PROP_ENGINE,
	/* Properties mirrored from engine */
	PROP_METADATA,
	PROP_STREAMINFO,
	/* Properties */
	PROP_ERROR,
	PROP_STATE,
	PROP_STATION,
	PROP_PLAYLIST,
	PROP_PLAYLIST_URI,
	PROP_PLAYLIST_REDIRECTION_URI,
	PROP_STREAM_URI,
	PROP_STREAM_REDIRECTION_URI,
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
	/* Playlist, if any */
	GCancellable *cancellable;
	GvPlaylist *playlist;
	gchar *playlist_uri;
	gchar *playlist_redirection_uri;
	/* Stream */
	gchar *stream_uri;
	gchar *stream_redirection_uri;
	gboolean stream_tls_error;
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
 * Helpers
 */

static void start_playback(GvPlayback *self);
static void stop_playback(GvPlayback *self);

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

/*
 * Signal handlers & callbacks
 */

static void gv_playback_set_error(GvPlayback *self, const gchar *message, const gchar *details);
static void gv_playback_set_playlist(GvPlayback *self, GvPlaylist *playlist);
static void gv_playback_set_playlist_uri(GvPlayback *self, const gchar *uri);
static void gv_playback_set_playlist_redirection_uri(GvPlayback *self, const gchar *uri);
static void gv_playback_set_state(GvPlayback *self, GvPlaybackState value);
static void gv_playback_set_stream_uri(GvPlayback *self, const gchar *uri);
static void gv_playback_set_stream_redirection_uri(GvPlayback *self, const gchar *uri);
static void play_stream(GvPlayback *self, const gchar *uri);
static void schedule_retry(GvPlayback *self);
static void reset_playlist(GvPlayback *self);
static void reset_retry(GvPlayback *self);

static void
on_engine_bad_certificate(GvEngine *engine, GTlsCertificateFlags tls_errors, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %d, %p", engine, tls_errors, self);

	/* Remember that it's a TLS error */
	priv->stream_tls_error = TRUE;

	/* Forward the signal */
	g_signal_emit(self, signals[SIGNAL_BAD_CERTIFICATE], 0, tls_errors);

}

static void
on_engine_end_of_stream(GvEngine *engine, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %p", engine, self);

	gv_playback_set_error(self, _("End of stream"), NULL);

	if (priv->playback_on == TRUE)
		schedule_retry(self);
}

static void
on_engine_notify(GvEngine *engine, GParamSpec *pspec, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", engine, property_name, self);

	if (!g_strcmp0(property_name, "streaminfo")) {
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
on_engine_playback_error(GvEngine *engine, GError *error, const gchar *debug,
		GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %p, %s, %p", engine, error, debug, self);

	gv_playback_set_error(self, error->message, debug);

	/* Do not retry if it's a TLS error */
	if (priv->stream_tls_error == TRUE)
		return;

	if (priv->playback_on == TRUE)
		schedule_retry(self);
}

static void
on_engine_redirected(GvEngine *engine, const gchar *uri, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %s, %p", engine, uri, self);

	/* If the uri didn't change, it's not a redirection */
	if (g_strcmp0(uri, priv->stream_uri) == 0)
		return;

	INFO("Redirected to: %s", uri);
	gv_playback_set_stream_redirection_uri(self, uri);
}

static gboolean
on_playlist_accept_certificate(GvPlaylist *playlist, GTlsCertificate *tls_certificate,
		GTlsCertificateFlags tls_errors, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	gboolean insecure;

	TRACE("%p, %p, %d, %p", playlist, tls_certificate, tls_errors, self);

	if (priv->station == NULL) {
		WARNING("Received accept-certificate signal, but no station set");
		return FALSE;
	}

	insecure = gv_station_get_insecure(priv->station);
	if (insecure == TRUE) {
		INFO("Accepting certificate anyway, per user config");
		return TRUE;
	} else {
		INFO("Rejecting certificate");
		g_signal_emit(self, signals[SIGNAL_BAD_CERTIFICATE], 0, tls_errors);
		return FALSE;
	}
}

static void
on_playlist_restarted(GvPlaylist *playlist, const gchar *uri, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p, %s, %p", playlist, uri, self);

	/* If the uri didn't change, it's not a redirection */
	if (g_strcmp0(uri, priv->playlist_uri) == 0)
		return;

	INFO("Redirected to: %s", uri);
	gv_playback_set_playlist_redirection_uri(self, uri);
}

static void
playlist_downloaded_callback(GvPlaylist *playlist, GAsyncResult *result, GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	GError *err = NULL;
	const gchar *station_uri;
	const gchar *stream_uri;

	gv_playlist_download_finish(playlist, result, &err);

	/* Check if we've been cancelled */
	if (g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		DEBUG("%s", err->message);
		gv_playback_set_state(self, GV_PLAYBACK_STATE_STOPPED);
		goto out;
	}

	/* We're done with the cancellable */
	g_clear_object(&priv->cancellable);

	/* Make sure we still have a station, and get the station uri */
	if (priv->station == NULL) {
		DEBUG("Playlist downloaded, but no station");
		goto out;
	}
	station_uri = gv_station_get_uri(priv->station);

	/* Check for errors we can handle */
	if (g_error_matches(err, GV_PLAYLIST_ERROR, GV_PLAYLIST_ERROR_EXTENSION) ||
	    g_error_matches(err, GV_PLAYLIST_ERROR, GV_PLAYLIST_ERROR_CONTENT_TYPE)) {
		/* It's not a playlist, let's assume it's a stream */
		reset_playlist(self);
		play_stream(self, station_uri);
		goto out;
	}

	/* Bail out for other errors */
	if (err != NULL) {
		INFO("Failed to download playlist: %s", err->message);
		gv_playback_set_state(self, GV_PLAYBACK_STATE_STOPPED);
		gv_playback_set_error(self, _("Failed to download playlist"),
				err->message);
		goto out;
	}

	/* Parse the playlist */
	gv_playlist_parse(playlist, &err);
	if (err != NULL) {
		INFO("Failed to parse playlist: %s", err->message);
		gv_playback_set_state(self, GV_PLAYBACK_STATE_STOPPED);
		gv_playback_set_error(self, _("Failed to parse playlist"),
				err->message);
		goto out;
	}

	/* Get the first stream, bail out if there's no such */
	stream_uri = gv_playlist_get_first_stream(playlist);
	if (stream_uri == NULL) {
		INFO("No stream found in playlist");
		gv_playback_set_state(self, GV_PLAYBACK_STATE_STOPPED);
		gv_playback_set_error(self, _("Failed to parse playlist"),
				"No stream");
		goto out;
	}

	/* At this point, we know the uri of the stream to play */
	play_stream(self, stream_uri);

out:
	g_clear_error(&err);
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
	g_signal_connect_object(engine, "bad-certificate",
			G_CALLBACK(on_engine_bad_certificate), self, 0);
	g_signal_connect_object(engine, "end-of-stream",
			G_CALLBACK(on_engine_end_of_stream), self, 0);
	g_signal_connect_object(engine, "notify",
			G_CALLBACK(on_engine_notify), self, 0);
	g_signal_connect_object(engine, "playback-error",
			G_CALLBACK(on_engine_playback_error), self, 0);
	g_signal_connect_object(engine, "redirected",
			G_CALLBACK(on_engine_redirected), self, 0);
}

/*
 * Property accessors - properties mirrored from engine
 * We don't notify here. It's done in the engine notify handler instead.
 */

GvMetadata *
gv_playback_get_metadata(GvPlayback *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_metadata(engine);
}

GvStreaminfo *
gv_playback_get_streaminfo(GvPlayback *self)
{
	GvEngine *engine = self->priv->engine;

	return gv_engine_get_streaminfo(engine);
}

/*
 * Property accessors
 */

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

GvStation *
gv_playback_get_station(GvPlayback *self)
{
	return self->priv->station;
}

void
gv_playback_set_station(GvPlayback *self, GvStation *station)
{
	GvPlaybackPrivate *priv = self->priv;

	if (station == priv->station)
		return;

	stop_playback(self);
	reset_retry(self);

	g_clear_object(&priv->station);

	if (station)
		priv->station = g_object_ref(station);

	if (priv->playback_on == TRUE)
		start_playback(self);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATION]);

	DEBUG("Station set to '%s'", station ? gv_station_get_name_or_uri(station) : NULL);
}

GvPlaylist *
gv_playback_get_playlist(GvPlayback *self)
{
	return self->priv->playlist;
}

static void
gv_playback_set_playlist(GvPlayback *self, GvPlaylist *playlist)
{
	GvPlaybackPrivate *priv = self->priv;

	if (g_set_object(&priv->playlist, playlist) == FALSE)
		return;

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PLAYLIST]);
}

const gchar *
gv_playback_get_playlist_uri(GvPlayback *self)
{
	return self->priv->playlist_uri;
}

static void
gv_playback_set_playlist_uri(GvPlayback *self, const gchar *uri)
{
	GvPlaybackPrivate *priv = self->priv;

	if (!g_strcmp0(uri, ""))
		uri = NULL;

	if (!g_strcmp0(priv->playlist_uri, uri))
		return;

	g_free(priv->playlist_uri);
	priv->playlist_uri = g_strdup(uri);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PLAYLIST_URI]);
}

const gchar *
gv_playback_get_playlist_redirection_uri(GvPlayback *self)
{
	return self->priv->playlist_redirection_uri;
}

static void
gv_playback_set_playlist_redirection_uri(GvPlayback *self, const gchar *uri)
{
	GvPlaybackPrivate *priv = self->priv;

	if (!g_strcmp0(uri, ""))
		uri = NULL;

	if (!g_strcmp0(priv->playlist_redirection_uri, uri))
		return;

	g_free(priv->playlist_redirection_uri);
	priv->playlist_redirection_uri = g_strdup(uri);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PLAYLIST_REDIRECTION_URI]);
}

const gchar *
gv_playback_get_stream_uri(GvPlayback *self)
{
	return self->priv->stream_uri;
}

static void
gv_playback_set_stream_uri(GvPlayback *self, const gchar *uri)
{
	GvPlaybackPrivate *priv = self->priv;

	if (!g_strcmp0(uri, ""))
		uri = NULL;

	if (!g_strcmp0(priv->stream_uri, uri))
		return;

	g_free(priv->stream_uri);
	priv->stream_uri = g_strdup(uri);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAM_URI]);
}

const gchar *
gv_playback_get_stream_redirection_uri(GvPlayback *self)
{
	return self->priv->stream_redirection_uri;
}

static void
gv_playback_set_stream_redirection_uri(GvPlayback *self, const gchar *uri)
{
	GvPlaybackPrivate *priv = self->priv;

	if (!g_strcmp0(uri, ""))
		uri = NULL;

	if (!g_strcmp0(priv->stream_redirection_uri, uri))
		return;

	g_free(priv->stream_redirection_uri);
	priv->stream_redirection_uri = g_strdup(uri);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAM_REDIRECTION_URI]);
}

static void
gv_playback_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GvPlayback *self = GV_PLAYBACK(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	/* Properties mirrored from engine */
	case PROP_METADATA:
		g_value_set_boxed(value, gv_playback_get_metadata(self));
		break;
	case PROP_STREAMINFO:
		g_value_set_boxed(value, gv_playback_get_streaminfo(self));
		break;
	/* Properties */
	case PROP_ERROR:
		g_value_set_object(value, gv_playback_get_error(self));
		break;
	case PROP_STATE:
		g_value_set_enum(value, gv_playback_get_state(self));
		break;
	case PROP_STATION:
		g_value_set_object(value, gv_playback_get_station(self));
		break;
	case PROP_PLAYLIST:
		g_value_set_object(value, gv_playback_get_playlist(self));
		break;
	case PROP_PLAYLIST_URI:
		g_value_set_string(value, gv_playback_get_playlist_uri(self));
		break;
	case PROP_PLAYLIST_REDIRECTION_URI:
		g_value_set_string(value, gv_playback_get_playlist_redirection_uri(self));
		break;
	case PROP_STREAM_URI:
		g_value_set_string(value, gv_playback_get_stream_uri(self));
		break;
	case PROP_STREAM_REDIRECTION_URI:
		g_value_set_string(value, gv_playback_get_stream_redirection_uri(self));
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
 * Helpers
 */

static gboolean
when_idle_play(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	GvEngine *engine = priv->engine;
	GvStation *station = priv->station;
	const gchar *user_agent;
	gboolean ssl_strict;

	if (priv->stream_uri == NULL) {
		DEBUG("No stream uri");
		goto out;
	}

	if (priv->station == NULL) {
		DEBUG("No station");
		goto out;
	}

	user_agent = gv_station_get_user_agent(station);
	ssl_strict = gv_station_get_insecure(station) ? FALSE : TRUE;

	gv_engine_play(engine, priv->stream_uri, user_agent, ssl_strict);

out:
	return G_SOURCE_REMOVE;
}

static void
reset_stream(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	gv_playback_set_stream_uri(self, NULL);
	gv_playback_set_stream_redirection_uri(self, NULL);

	priv->stream_tls_error = FALSE;
}

static void
play_stream(GvPlayback *self, const gchar *uri)
{
	GvPlaybackPrivate *priv = self->priv;

	g_assert(priv->stream_uri == NULL);
	gv_playback_set_stream_uri(self, uri);

	g_assert(priv->stream_redirection_uri == NULL);

	g_idle_add(G_SOURCE_FUNC(when_idle_play), self);
}

static void
reset_playlist(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	if (priv->cancellable != NULL)
		g_cancellable_cancel(priv->cancellable);
	g_clear_object(&priv->cancellable);

	gv_playback_set_playlist(self, NULL);
	gv_playback_set_playlist_uri(self, NULL);
	gv_playback_set_playlist_redirection_uri(self, NULL);
}

static void
download_playlist(GvPlayback *self, const gchar *uri, const gchar *user_agent)
{
	GvPlaybackPrivate *priv = self->priv;
	GvPlaylist *playlist;

	g_assert(priv->cancellable == NULL);
	priv->cancellable = g_cancellable_new();

	g_assert(priv->playlist == NULL);
	playlist = gv_playlist_new();
	gv_playback_set_playlist(self, playlist);
	g_object_unref(playlist);

	g_assert(priv->playlist_uri == NULL);
	gv_playback_set_playlist_uri(self, uri);

	g_assert(priv->playlist_redirection_uri == NULL);

	g_signal_connect_object(playlist, "accept-certificate",
			G_CALLBACK(on_playlist_accept_certificate), self, 0);
	g_signal_connect_object(playlist, "restarted",
			G_CALLBACK(on_playlist_restarted), self, 0);

	gv_playlist_download_async(playlist, uri, user_agent, priv->cancellable,
			(GAsyncReadyCallback) playlist_downloaded_callback, self);
}

static void
reset_retry(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	g_clear_handle_id(&priv->retry_timeout_id, g_source_remove);
	priv->retry_count = 0;
}

static void
schedule_retry(GvPlayback *self)
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

	/* Increase the retry counter */
	priv->retry_count++;
	delay = priv->retry_count;
	if (delay > RETRY_MAX_DELAY)
		delay = RETRY_MAX_DELAY;

	INFO("Restarting playback in %u seconds", delay);
	priv->retry_timeout_id = g_timeout_add_seconds(delay, when_timeout_retry, self);

	gv_playback_set_state(self, GV_PLAYBACK_STATE_WAITING_RETRY);
}

static void
stop_playback(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	/* Stop engine (if ever it was started) */
	gv_engine_stop(priv->engine);

	/* Reset stream and playlist */
	reset_stream(self);
	reset_playlist(self);

	/* Reset state and error */
	gv_playback_set_error(self, NULL, NULL);
	gv_playback_set_state(self, GV_PLAYBACK_STATE_STOPPED);

	/* Do NOT reset retry */
}

static void
start_playback(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;
	GvStation *station = priv->station;
	GvPlaylistFormat format;
	GError *err = NULL;
	const gchar *station_uri;
	const gchar *user_agent;
	gboolean ret;

	/* Stop playback first */
	stop_playback(self);

	/* Bail out if no station is set */
	if (station == NULL)
		return;

	/* Get station details */
	station_uri = gv_station_get_uri(station);
	user_agent = gv_station_get_user_agent(station);

	INFO("Station uri: %s", station_uri);

	/* Try to guess whether it's a playlist or an audio stream */
	ret = gv_playlist_format_from_uri(station_uri, &format, &err);
	if (ret == FALSE) {
		/* Not a playlist */
		INFO("Can't get playlist format from uri: %s", err->message);
		g_clear_error(&err);

		/* Assume it's an audio stream, play it */
		play_stream(self, station_uri);

		/* No need to set the state, we'll get notified by the engine */
	} else {
		/* Probably a playlist */
		INFO("Looks like a playlist: format=%s",
				gv_playlist_format_to_string(format));

		/* Download it */
		download_playlist(self, station_uri, user_agent);
		gv_playback_set_state(self, GV_PLAYBACK_STATE_DOWNLOADING_PLAYLIST);
	}
}

/*
 * Public methods
 */

void
gv_playback_stop(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	INFO("Stopping playback ...");

	/* Remember what we're doing */
	priv->playback_on = FALSE;

	/* Stop playback */
	stop_playback(self);

	/* Reset retry */
	reset_retry(self);
}

void
gv_playback_start(GvPlayback *self)
{
	GvPlaybackPrivate *priv = self->priv;

	INFO("Starting playback ...");

	if (priv->station == NULL) {
		WARNING("No station set!");
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

	/* Free the stream details */
	g_free(priv->stream_uri);
	g_free(priv->stream_redirection_uri);

	/* Cancel playlist download */
	if (priv->cancellable)
		g_cancellable_cancel(priv->cancellable);
	g_clear_object(&priv->cancellable);

	/* Free the playlist details */
	g_clear_object(&priv->playlist);
	g_free(priv->playlist_uri);
	g_free(priv->playlist_redirection_uri);

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

	/* Properties mirrored from engine */
	properties[PROP_METADATA] =
		g_param_spec_boxed("metadata", "Current metadata", NULL,
				   GV_TYPE_METADATA,
				   GV_PARAM_READABLE);

	properties[PROP_STREAMINFO] =
		g_param_spec_boxed("streaminfo", "Stream information", NULL,
				   GV_TYPE_STREAMINFO,
				   GV_PARAM_READABLE);

	/* Properties */
	properties[PROP_ERROR] =
		g_param_spec_boxed("error", "Playback error", NULL,
				   GV_TYPE_PLAYBACK_ERROR,
				   GV_PARAM_READABLE);

	properties[PROP_STATE] =
		g_param_spec_enum("state", "State", NULL,
				  GV_TYPE_PLAYBACK_STATE,
				  GV_PLAYBACK_STATE_STOPPED,
				  GV_PARAM_READABLE);

	properties[PROP_STATION] =
		g_param_spec_object("station", "Current station", NULL,
				    GV_TYPE_STATION,
				    GV_PARAM_READWRITE);

	properties[PROP_PLAYLIST] =
		g_param_spec_object("playlist", "Playlist", NULL,
				    GV_TYPE_STATION,
				    GV_PARAM_READABLE);

	properties[PROP_PLAYLIST_URI] =
		g_param_spec_string("playlist-uri", "Playlist URI", NULL, NULL,
				    GV_PARAM_READABLE);

	properties[PROP_PLAYLIST_REDIRECTION_URI] =
		g_param_spec_string("playlist-redirection-uri", "Playlist Redirection URI", NULL, NULL,
				    GV_PARAM_READABLE);

	properties[PROP_STREAM_URI] =
		g_param_spec_string("stream-uri", "Stream URI", NULL, NULL,
				    GV_PARAM_READABLE);

	properties[PROP_STREAM_REDIRECTION_URI] =
		g_param_spec_string("stream-redirection-uri", "Stream Redirection URI", NULL, NULL,
				    GV_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_BAD_CERTIFICATE] =
		g_signal_new("bad-certificate", G_OBJECT_CLASS_TYPE(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_TLS_CERTIFICATE_FLAGS);
}
