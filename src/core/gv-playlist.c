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
#include <libsoup/soup.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core-internal.h"
#include "core/playlist-utils.h"

#include "core/gv-playlist.h"

#define PLAYLIST_MAX_SIZE (1024 * 128) // 128 kB

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Set at construct-time */
	PROP_URI,
	PROP_REDIRECTION_URI,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

enum {
	SIGNAL_ACCEPT_CERTIFICATE,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvPlaylistPrivate {
	/* Construct-only properties */
	gchar *uri;
	/* Properties */
	gchar *redirection_uri;
	/* The rest */
	GvPlaylistFormat format;
	gchar *buffer;
	gsize  buffer_size;
	GSList *streams;
};

typedef struct _GvPlaylistPrivate GvPlaylistPrivate;

struct _GvPlaylist {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvPlaylistPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvPlaylist, gv_playlist, G_TYPE_OBJECT)

/*
 * Helpers
 */

static gboolean
content_type_is_likely_audio(const gchar *content_type)
{
	/* audio/mpegurl is said to be a valid value for HLS streams,
	 * however it's also sometimes used for m3u playlists, so we
	 * don't list it here.
	 *
	 * Also, content-types such as application/octect-stream or
	 * binary/octet-stream don't mean a thing, we ignore it.
	 */
	const gchar *audio_types[] = {
		"audio/aac",	// aac
		"audio/aacp",	// aac
		"audio/flac",	// flac
		"audio/mpeg",	// mp3
		"audio/ogg",	// ogg
		NULL,
	};
	const gchar *application_types[] = {
		"application/dash+xml",	// dash
		"application/ogg",	// ogg
		"application/vnd.apple.mpegurl",// hls
		NULL,
	};
	const gchar **types;
	const gchar **ptr;
	guint offset;

	g_return_val_if_fail(content_type != NULL, FALSE);

	if (g_str_has_prefix(content_type, "audio/")) {
		types = audio_types;
		offset = 6;
	} else if (g_str_has_prefix(content_type, "application/")) {
		types = application_types;
		offset = 12;
	} else {
		return FALSE;
	}

	for (ptr = types; *ptr != NULL; ptr++) {
		const gchar *str1 = content_type + offset;
		const gchar *str2 = (*ptr) + offset;
		if (g_strcmp0(str1, str2) == 0)
			return TRUE;
	}

	return FALSE;
}

/*
 * Property accessors
 */

const gchar *
gv_playlist_get_uri(GvPlaylist *self)
{
	return self->priv->uri;
}

static void
gv_playlist_set_uri(GvPlaylist *self, const gchar *uri)
{
	GvPlaylistPrivate *priv = self->priv;

	g_assert(priv->uri == NULL);
	g_assert(uri != NULL);

	priv->uri = g_strdup(uri);
}

const gchar *
gv_playlist_get_redirection_uri(GvPlaylist *self)
{
	return self->priv->redirection_uri;
}

static void
gv_playlist_set_redirection_uri(GvPlaylist *self, const gchar *uri)
{
	GvPlaylistPrivate *priv = self->priv;

	if (!g_strcmp0(priv->redirection_uri, uri))
		return;

	g_free(priv->redirection_uri);
	priv->redirection_uri = g_strdup(uri);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_REDIRECTION_URI]);
}

static void
gv_playlist_get_property(GObject *object,
			guint property_id,
			GValue *value G_GNUC_UNUSED,
			GParamSpec *pspec)
{
	GvPlaylist *self = GV_PLAYLIST(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_URI:
		g_value_set_string(value, gv_playlist_get_uri(self));
		break;
	case PROP_REDIRECTION_URI:
		g_value_set_string(value, gv_playlist_get_redirection_uri(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_playlist_set_property(GObject *object,
			guint property_id,
			const GValue *value,
			GParamSpec *pspec)
{
	GvPlaylist *self = GV_PLAYLIST(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_URI:
		gv_playlist_set_uri(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Signal handlers
 */

static gboolean
on_soup_message_accept_certificate(SoupMessage *msg G_GNUC_UNUSED,
                                   GTlsCertificate *tls_certificate,
                                   GTlsCertificateFlags tls_errors,
				   gpointer user_data)
{
	GTask *task = G_TASK(user_data);
	GvPlaylist *self = GV_PLAYLIST(g_task_get_source_object(task));
        gboolean accept = FALSE;

	g_signal_emit(self, signals[SIGNAL_ACCEPT_CERTIFICATE], 0,
		      tls_certificate, tls_errors, &accept);

        return accept;
}

static void
on_soup_message_restarted(SoupMessage *msg, gpointer user_data)
{
	GTask *task = G_TASK(user_data);
	GvPlaylist *self = GV_PLAYLIST(g_task_get_source_object(task));
	GvPlaylistPrivate *priv = self->priv;
	GUri *msg_uri = soup_message_get_uri(msg);
	gchar *uri;

	/* We must compare this uri with the original uri to know if we're
	 * being redirected. If it's the same, it's not a redirection.
	 */
	uri = g_uri_to_string(msg_uri);
	if (g_strcmp0(uri, priv->uri) == 0)
		g_clear_pointer(&uri, g_free);
	else
		INFO("Redirected to: %s", uri);

	gv_playlist_set_redirection_uri(self, uri);
	g_free(uri);
}

/*
 * Private methods
 */

static void
input_stream_read_callback(GObject *source, GAsyncResult *result, gpointer user_data)
{
	GTask *task = G_TASK(user_data);
	GvPlaylist *self = GV_PLAYLIST(g_task_get_source_object(task));
	GvPlaylistPrivate *priv = self->priv;
	GInputStream* input_stream = G_INPUT_STREAM(source);
	GError *err = NULL;
	gsize bytes_read;
	gboolean ret;

	TRACE("%p, %p, %p", source, result, user_data);

	ret = g_input_stream_read_all_finish(input_stream, result, &bytes_read, &err);
	if (ret == FALSE) {
		g_task_return_error(task, err);
		goto out;
	}

	DEBUG("Read %" G_GSIZE_FORMAT " bytes from http input", bytes_read);

	g_assert(bytes_read <= priv->buffer_size);
	if (bytes_read == priv->buffer_size) {
		g_task_return_new_error(task, GV_PLAYLIST_ERROR,
				GV_PLAYLIST_ERROR_TOO_BIG,
				"Playlist too big (> %u kB)",
				PLAYLIST_MAX_SIZE / 1024);
		goto out;
	}

	/* Make sure the string is null-terminated and realloc */
	priv->buffer = g_realloc(priv->buffer, bytes_read + 1);
	priv->buffer[bytes_read] = '\0';
	priv->buffer_size = bytes_read;

	g_task_return_boolean(task, TRUE);

out:
	g_object_unref(input_stream);
	g_object_unref(task);
}

static void
message_sent_callback(GObject *source, GAsyncResult *result, gpointer user_data)
{
	GTask *task = G_TASK(user_data);
	GvPlaylist *self = GV_PLAYLIST(g_task_get_source_object(task));
	GvPlaylistPrivate *priv = self->priv;
	SoupSession *session = SOUP_SESSION(source);
	SoupMessage *msg;
	SoupMessageHeaders *headers;
	SoupStatus status;
	GInputStream* input_stream;
	GError *err = NULL;
	const gchar *content_type;

	TRACE("%p, %p, %p", source, result, user_data);

	/* Bail out now if the user has already cancelled */
	if (g_task_return_error_if_cancelled(task))
		goto error;

	/* Get the soup message */
	msg = soup_session_get_async_result_message(session, result);
	g_assert(msg != NULL);

	/* Complete the request and get an input stream.
	 *
	 * NB: send_finish() must be called before get_status() and
	 * SOUP_STATUS_IS_SUCCESSFUL(). For example, if URL can't be resolved,
	 * we'll get a meaningful error message here and now by calling
	 * send_finish(), while get_status() would give zero, and
	 * SOUP_STATUS_IS_SUCCESSFUL() would then fail without information.
	 */
	input_stream = soup_session_send_finish(session, result, &err);
	if (input_stream == NULL) {
		DEBUG("Error in send/receive: %s", err->message);
		g_task_return_error(task, err);
		goto error;
	}

	/* Check if the request succeeded */
	status = soup_message_get_status(msg);
	if (SOUP_STATUS_IS_SUCCESSFUL(status) == FALSE) {
		const gchar *reason = soup_message_get_reason_phrase(msg);
		DEBUG("HTTP request failed: %u: %s", status, reason);
		g_task_return_new_error(task, GV_PLAYLIST_ERROR,
				GV_PLAYLIST_ERROR_DOWNLOAD,
				"HTTP status: %u: %s",
				status, reason);
		goto error;
	}

	/* Check the headers and the content-type */
	headers = soup_message_get_response_headers(msg);
	content_type = soup_message_headers_get_content_type(headers, NULL);
	if (content_type != NULL) {
		DEBUG("Got Content-Type header: %s", content_type);

		/* The idea here is to check if the content-type is indicative
		 * of an audio stream.  If that's the case, we abort right now
		 * with ERROR_CONTENT_TYPE. The uri will then be handed over to
		 * GStreamer, it will play it if it can.
		 */
		if (content_type_is_likely_audio(content_type)) {
			DEBUG("Not a playlist, according to Content-Type");
			g_task_return_new_error(task, GV_PLAYLIST_ERROR,
					GV_PLAYLIST_ERROR_CONTENT_TYPE,
					"Content-Type indicates an audio stream");
			goto error;
		}
	}

	DEBUG("Playlist download in progress...");

	/* Allocate a buffer and read data */
	priv->buffer_size = PLAYLIST_MAX_SIZE + 1;
	priv->buffer = g_new(gchar, priv->buffer_size);
	g_input_stream_read_all_async(input_stream, priv->buffer, priv->buffer_size,
			G_PRIORITY_DEFAULT, g_task_get_cancellable(task),
			input_stream_read_callback, task);

	return;

error:
	g_object_unref(task);
}

/*
 * Public methods
 */

GSList *
gv_playlist_get_stream_uris(GvPlaylist *self)
{
	return self->priv->streams;
}

const gchar *
gv_playlist_get_first_stream(GvPlaylist *self)
{
	GvPlaylistPrivate *priv = self->priv;
	GSList *streams = priv->streams;

	if (streams == NULL)
		return NULL;

	return (const gchar *) streams->data;
}

gboolean
gv_playlist_parse(GvPlaylist *self, GError **error)
{
	GvPlaylistPrivate *priv = self->priv;
	GvPlaylistParser parser;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
	g_assert(priv->buffer != NULL);
	g_assert(priv->streams == NULL);

	switch (priv->format) {
	case GV_PLAYLIST_FORMAT_ASX:
		parser = gv_parse_asx_playlist;
		break;
	case GV_PLAYLIST_FORMAT_M3U:
		parser = gv_parse_m3u_playlist;
		break;
	case GV_PLAYLIST_FORMAT_PLS:
		parser = gv_parse_pls_playlist;
		break;
	case GV_PLAYLIST_FORMAT_XSPF:
		parser = gv_parse_xspf_playlist;
		break;
	default:
		ERROR("No parser for playlist format: %d", priv->format);
		/* Program execution stops here */
	}

	priv->streams = parser(priv->buffer, priv->buffer_size);
	if (priv->streams == NULL) {
		g_set_error(error, GV_PLAYLIST_ERROR,
				GV_PLAYLIST_ERROR_CONTENT,
				"Invalid content, or empty");
		return FALSE;
	}

	GSList *item;
	DEBUG("%d streams found:", g_slist_length(priv->streams));
	for (item = priv->streams; item; item = item->next)
		DEBUG(". %s", item->data);

	return TRUE;
}

gboolean
gv_playlist_download_finish(GvPlaylist *self,
			    GAsyncResult *result,
			    GError **error)
{
	g_return_val_if_fail(g_task_is_valid(result, self), FALSE);

	return g_task_propagate_boolean(G_TASK(result), error);
}

void
gv_playlist_download_async(GvPlaylist *self,
			   const gchar *user_agent,
			   GCancellable *cancellable,
			   GAsyncReadyCallback callback,
			   gpointer user_data)
{
	GvPlaylistPrivate *priv = self->priv;
	GTask *task;
	SoupSession *session;
	SoupMessage *msg;
	GError *err = NULL;

	/* User can call this method only once */
	g_return_if_fail(priv->format == GV_PLAYLIST_FORMAT_UNKNOWN);

	/* Guess the playlist format according to the extension */
	priv->format = gv_playlist_guess_format(priv->uri, &err);
	if (priv->format == GV_PLAYLIST_FORMAT_UNKNOWN) {
		g_task_report_error(self, callback, user_data,
				gv_playlist_download_async, err);
		return;
	}

	/* If no user-agent was given, fall back to a default */
	if (user_agent == NULL)
		user_agent = gv_core_user_agent;

	INFO("Downloading playlist: %s", priv->uri);
	INFO("with user-agent: %s", user_agent);

	/* Send the request using libsoup */
	session = soup_session_new_with_options("user-agent", user_agent, NULL);

	task = g_task_new(self, cancellable, callback, user_data);
        g_task_set_task_data(task, session, g_object_unref);

	msg = soup_message_new(SOUP_METHOD_GET, priv->uri);
	g_signal_connect_object(msg, "accept-certificate",
			G_CALLBACK(on_soup_message_accept_certificate), task, 0);
	g_signal_connect_object(msg, "restarted",
			G_CALLBACK(on_soup_message_restarted), task, 0);

	soup_session_send_async(session, msg, G_PRIORITY_DEFAULT, cancellable,
			message_sent_callback, task);
}

GvPlaylist *
gv_playlist_new(const gchar *uri)
{
	return g_object_new(GV_TYPE_PLAYLIST, "uri", uri, NULL);
}

/*
 * GObject methods
 */

static void
gv_playlist_finalize(GObject *object)
{
	GvPlaylistPrivate *priv = GV_PLAYLIST(object)->priv;

	TRACE("%p", object);

	g_slist_free_full(priv->streams, g_free);
	g_free(priv->buffer);
	g_free(priv->redirection_uri);
	g_free(priv->uri);

	G_OBJECT_CHAINUP_FINALIZE(gv_playlist, object);
}

static void
gv_playlist_constructed(GObject *object)
{
	TRACE("%p", object);

	G_OBJECT_CHAINUP_CONSTRUCTED(gv_playlist, object);
}

static void
gv_playlist_init(GvPlaylist *self)
{
	TRACE("%p", self);

	self->priv = gv_playlist_get_instance_private(self);
}

static void
gv_playlist_class_init(GvPlaylistClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_playlist_finalize;
	object_class->constructed = gv_playlist_constructed;

	/* Properties */
	object_class->get_property = gv_playlist_get_property;
	object_class->set_property = gv_playlist_set_property;

	properties[PROP_URI] =
		g_param_spec_string("uri", "Uri", NULL, NULL,
				    GV_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_REDIRECTION_URI] =
		g_param_spec_string("redirection-uri", "Redirection Uri", NULL, NULL,
				    GV_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_ACCEPT_CERTIFICATE] =
		g_signal_new("accept-certificate", G_OBJECT_CLASS_TYPE(class),
			     G_SIGNAL_RUN_LAST, 0,
			     g_signal_accumulator_true_handled, NULL, NULL,
			     G_TYPE_BOOLEAN, 2,
			     G_TYPE_TLS_CERTIFICATE,
			     G_TYPE_TLS_CERTIFICATE_FLAGS);
}

/*
 * Public functions
 */

GvPlaylistFormat
gv_playlist_guess_format(const gchar *playlist_uri, GError **error)
{
	GvPlaylistFormat fmt = GV_PLAYLIST_FORMAT_UNKNOWN;
	gchar *ext = NULL;
	gboolean res;

	g_return_val_if_fail(error == NULL || *error == NULL, fmt);

	res = gv_get_uri_extension_lowercase(playlist_uri, &ext, error);
	if (res == FALSE)
		return fmt;
	if (ext == NULL) {
		g_set_error(error, GV_PLAYLIST_ERROR,
				GV_PLAYLIST_ERROR_EXTENSION,
				"No extension");
		return fmt;
	}

	if (!g_ascii_strcasecmp(ext, "m3u"))
		fmt = GV_PLAYLIST_FORMAT_M3U;
	else if (!g_ascii_strcasecmp(ext, "ram"))
		fmt = GV_PLAYLIST_FORMAT_M3U;
	else if (!g_ascii_strcasecmp(ext, "pls"))
		fmt = GV_PLAYLIST_FORMAT_PLS;
	else if (!g_ascii_strcasecmp(ext, "asx"))
		fmt = GV_PLAYLIST_FORMAT_ASX;
	else if (!g_ascii_strcasecmp(ext, "xspf"))
		fmt = GV_PLAYLIST_FORMAT_XSPF;

	if (fmt == GV_PLAYLIST_FORMAT_UNKNOWN)
		g_set_error(error, GV_PLAYLIST_ERROR,
				GV_PLAYLIST_ERROR_EXTENSION,
				"Unsupported extension: %s", ext);

	g_free(ext);

	return fmt;
}
