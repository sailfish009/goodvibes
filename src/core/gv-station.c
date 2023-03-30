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

#include <glib-object.h>
#include <glib.h>
#include <libsoup/soup.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core-internal.h"
#include "core/gv-engine.h"
#include "core/playlist-utils.h"

#include "core/gv-station.h"

/*
 * Properties
 */

#define DEFAULT_INSECURE FALSE

enum {
	/* Reserved */
	PROP_0,
	/* Set at construct-time */
	PROP_UID,
	/* Set by user - station definition */
	PROP_NAME,
	PROP_URI,
	/* Set by user - customization */
	PROP_INSECURE,
	PROP_USER_AGENT,
	/* Learnt along the way */
	PROP_REDIRECTED_URI,
	PROP_STREAM_URIS,
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

struct _GvStationPrivate {
	/*
	 * Properties
	 */

	/* Set at construct-time */
	gchar *uid;
	/* Set by user - station definition */
	gchar *name;
	gchar *uri;
	/* Set by user - customization */
	gboolean insecure;
	gchar *user_agent;
	/* Learnt along the way */
	GvPlaylistFormat playlist_format;
	gchar *redirected_uri;
	GSList *stream_uris;
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
gv_station_set_stream_uris(GvStation *self, GSList *uris)
{
	GvStationPrivate *priv = self->priv;

	if (uris == priv->stream_uris)
		return;

	if (priv->stream_uris) {
		g_slist_free_full(priv->stream_uris, g_free);
		priv->stream_uris = NULL;
	}

	if (uris)
		priv->stream_uris = g_slist_copy_deep(uris, copy_func_strdup, NULL);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAM_URIS]);
}

static void
gv_station_set_stream_uri(GvStation *self, const gchar *uri)
{
	GSList *list;

	if (uri == NULL)
		list = NULL;
	else
		list = g_slist_append(NULL, (gpointer) uri);

	gv_station_set_stream_uris(self, list);
	g_slist_free(list);
}

static void
gv_station_set_redirected_uri(GvStation *self, const gchar *uri)
{
	GvStationPrivate *priv = self->priv;

	if (!g_strcmp0(priv->redirected_uri, uri))
		return;

	g_free(priv->redirected_uri);
	priv->redirected_uri = g_strdup(uri);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_REDIRECTED_URI]);
}

static void
gv_station_update_redirected_uri(GvStation *self, SoupMessage *msg)
{
	GvStationPrivate *priv = self->priv;
	GUri *msg_uri;
	gchar *uri;

	g_assert_nonnull(msg);

	msg_uri = soup_message_get_uri(msg);
	uri = g_uri_to_string(msg_uri);

	/* If the uri from the Soup message differs from the station uri,
	 * then it means we're being redirected, right?
	 */
	if (g_strcmp0(priv->uri, uri) != 0)
		gv_station_set_redirected_uri(self, uri);
	else
		gv_station_set_redirected_uri(self, NULL);

	g_free(uri);
}

/*
 * Signal handlers
 */

static gboolean
on_message_accept_certificate(SoupMessage *msg,
			      GTlsCertificate *tls_peer_certificate,
			      GTlsCertificateFlags tls_peer_errors,
			      gpointer user_data)
{
	GvStation *self = GV_STATION(user_data);
	GvStationPrivate *priv = self->priv;
	const gchar *uri;

	gv_station_update_redirected_uri(self, msg);
	uri = priv->redirected_uri ? priv->redirected_uri : priv->uri;

	INFO("Invalid certificate for uri: %s" uri);

	if (priv->insecure == TRUE) {
		INFO("Accepting certificate anyway, per user config");
		return TRUE;
	} else {
		INFO("Rejecting certificate");
		g_signal_emit(self, signals[SIGNAL_SSL_FAILURE], 0, uri);
		return FALSE;
	}
}

static void
on_message_completed(GObject *source, GAsyncResult *result, gpointer user_data)
{
	GvStation *self = GV_STATION(user_data);
	GvStationPrivate *priv = self->priv;
	SoupSession *session = SOUP_SESSION(source);
	SoupMessage *msg;
	SoupStatus status;
	GError *err = NULL;
	GBytes *body = NULL;
	const gchar *body_data;
	gsize body_length;
	GSList *streams = NULL;

	TRACE("%p, %p, %p", source, result, user_data);

	body = soup_session_send_and_read_finish(session, result, &err);
	if (body == NULL) {
		WARNING("Error in send/receive: %s", err->message);
		g_clear_error(&err);
		goto end;
	}

	msg = soup_session_get_async_result_message(session, result);
	if (msg == NULL) {
		WARNING("Error to get message");
		goto end;
	}

	gv_station_update_redirected_uri(self, msg);

	status = soup_message_get_status(msg);
	if (SOUP_STATUS_IS_SUCCESSFUL(status) == FALSE) {
		const gchar *reason;
		reason = soup_message_get_reason_phrase(msg);
		WARNING("Failed to download playlist (%u): %s", status, reason);
		goto end;
	} else {
		SoupMessageHeaders *headers;
		const gchar *content_type;
		headers = soup_message_get_response_headers(msg);
		if (headers)
			content_type = soup_message_headers_get_content_type(headers, NULL);
		else
			content_type = "unknown";
		DEBUG("Playlist downloaded (Content-Type: %s)", content_type);
	}

	body_data = g_bytes_get_data(body, NULL);
	body_length = g_bytes_get_size(body);
	if (body_length == 0) {
		WARNING("Empty playlist");
		goto end;
	}

	//PRINT("%s", body_data);

	streams = gv_playlist_parse(priv->playlist_format, body_data, body_length);

end:
	g_bytes_unref(body);
	// TODO Is it ok to unref that here ?
	g_object_unref(session);

	gv_station_set_stream_uris(self, streams);

	if (priv->stream_uris != NULL) {
		GvEngine *engine = gv_core_engine;
		gv_engine_play(engine, self);
	}
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

	/* The uri either refers to a playlist, either to an audio stream.
	 * We "guess" it right now:  if it does not seem to be a playlist,
	 * then it's probably an audio stream, and so we save it as such.
	 */
	priv->playlist_format = gv_playlist_get_format(uri);
	if (priv->playlist_format == GV_PLAYLIST_FORMAT_UNKNOWN)
		gv_station_set_stream_uri(self, uri);
	else
		gv_station_set_stream_uri(self, NULL);

	/* Notify */
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_URI]);
}

const gchar *
gv_station_get_name_or_uri(GvStation *self)
{
	GvStationPrivate *priv = self->priv;

	return priv->name ? priv->name : priv->uri;
}

gboolean
gv_station_get_insecure(GvStation *self)
{
	return self->priv->insecure;
}

void
gv_station_set_insecure(GvStation *self, gboolean insecure)
{
	GvStationPrivate *priv = self->priv;

	if (insecure == priv->insecure)
		return;

	priv->insecure = insecure;

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_INSECURE]);
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

const gchar *
gv_station_get_redirected_uri(GvStation *self)
{
	return self->priv->redirected_uri;
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

static void
gv_station_get_property(GObject *object,
			guint property_id,
			GValue *value G_GNUC_UNUSED,
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
	case PROP_INSECURE:
		g_value_set_boolean(value, gv_station_get_insecure(self));
		break;
	case PROP_USER_AGENT:
		g_value_set_string(value, gv_station_get_user_agent(self));
		break;
	case PROP_REDIRECTED_URI:
		g_value_set_string(value, gv_station_get_redirected_uri(self));
		break;
	case PROP_STREAM_URIS:
		g_value_set_pointer(value, gv_station_get_stream_uris(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_station_set_property(GObject *object,
			guint property_id,
			const GValue *value,
			GParamSpec *pspec)
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
	case PROP_INSECURE:
		gv_station_set_insecure(self, g_value_get_boolean(value));
		break;
	case PROP_USER_AGENT:
		gv_station_set_user_agent(self, g_value_get_string(value));
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
gv_station_reset(GvStation *self)
{
	GvStationPrivate *priv = self->priv;

	gv_station_set_redirected_uri(self, NULL);
	if (priv->playlist_format != GV_PLAYLIST_FORMAT_UNKNOWN)
		gv_station_set_stream_uri(self, NULL);
}

static gboolean
gv_station_download_playlist(GvStation *self)
{
	GvStationPrivate *priv = self->priv;
	SoupSession *session;
	SoupMessage *msg;
	const gchar *user_agent;

	if (priv->playlist_format == GV_PLAYLIST_FORMAT_UNKNOWN) {
		WARNING("Uri doesn't seem to be a playlist");
		return FALSE;
	}

	user_agent = priv->user_agent ? priv->user_agent : gv_core_user_agent;

	INFO("Downloading playlist '%s' (user-agent: '%s')", priv->uri, user_agent);

	session = soup_session_new_with_options("user-agent", user_agent, NULL);
	msg = soup_message_new(SOUP_METHOD_GET, priv->uri);
	g_signal_connect_object(msg, "accept-certificate",
				G_CALLBACK(on_message_accept_certificate), self, 0);
	soup_session_send_and_read_async(session, msg, G_PRIORITY_DEFAULT,
			NULL, (GAsyncReadyCallback) on_message_completed, self);

	return TRUE;
}

/*
 * Public methods
 */

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

void
gv_station_stop(GvStation *self)
{
	GvEngine *engine = gv_core_engine;

	// XXX Should cancel any pending playlist download,
	// or maintain an internal state, so that we can discard
	// stuff when download terminates.

	gv_engine_stop(engine);

	gv_station_reset(self);
}

void
gv_station_play(GvStation *self)
{
	GvStationPrivate *priv = self->priv;
	GvEngine *engine = gv_core_engine;

	/* First and before all: stop playback */
	gv_engine_stop(engine);

	/* Shouldn't be needed, but doesn't hurt to be sure */
	gv_station_reset(self);

	/* If we have stream URIs, let's play it */
	if (priv->stream_uris != NULL) {
		gv_engine_play(engine, self);
		return;
	}

	/* If we don't have stream URIs, it probably means that the
	 * station URI points to a playlist, and we didn't download it
	 * yet, so let's do that. The playlist holds the stream URIs.
	 */
	if (gv_station_download_playlist(self) == FALSE)
		WARNING("Can't download playlist");

	/* Nothing else to do: playlist download is async */
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
	g_free(priv->redirected_uri);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station, object);
}

static void
gv_station_constructed(GObject *object)
{
	GvStation *self = GV_STATION(object);
	GvStationPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Initialize properties */
	priv->uid = g_strdup_printf("%p", (void *) self);
	priv->insecure = DEFAULT_INSECURE;

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station, object);
}

static void
gv_station_init(GvStation *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_get_instance_private(self);
}

static void
gv_station_class_init(GvStationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_finalize;
	object_class->constructed = gv_station_constructed;

	/* Properties */
	object_class->get_property = gv_station_get_property;
	object_class->set_property = gv_station_set_property;

	properties[PROP_UID] =
		g_param_spec_string("uid", "UID", NULL, NULL,
				    GV_PARAM_READABLE);

	properties[PROP_NAME] =
		g_param_spec_string("name", "Name", NULL, NULL,
				    GV_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	properties[PROP_URI] =
		g_param_spec_string("uri", "Uri", NULL, NULL,
				    GV_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	properties[PROP_INSECURE] =
		g_param_spec_boolean("insecure", "Insecure", NULL,
				     DEFAULT_INSECURE,
				     GV_PARAM_READWRITE);

	properties[PROP_USER_AGENT] =
		g_param_spec_string("user-agent", "User agent", NULL, NULL,
				    GV_PARAM_READWRITE);

	properties[PROP_REDIRECTED_URI] =
		g_param_spec_string("redirected-uri", "Redirected uri", NULL, NULL,
				    GV_PARAM_READABLE);

	properties[PROP_STREAM_URIS] =
		g_param_spec_pointer("stream-uris", "Stream uris", NULL,
				     GV_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_SSL_FAILURE] =
		g_signal_new("ssl-failure", G_TYPE_FROM_CLASS(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_STRING);
}
