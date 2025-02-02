/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2024 Arnaud Rebillout
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

#include "base/glib-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"

#include "feat/gv-notifications.h"

#define NOTIF_ID_ERROR	 "error"
#define NOTIF_ID_PLAYING "playing"

/*
 * GObject definitions
 */

struct _GvNotifications {
	/* Parent instance structure */
	GvFeature parent_instance;
};

G_DEFINE_TYPE(GvNotifications, gv_notifications, GV_TYPE_FEATURE)

/*
 * Helpers
 */

static GNotification *
make_station_notification(GvStation *station)
{
	GNotification *notif;
	const gchar *str;
	gchar *body;

	if (station == NULL)
		return NULL;

	str = gv_station_get_name(station);
	if (str) {
		body = g_strdup_printf(_("Playing %s"), str);
	} else {
		str = gv_station_get_uri(station);
		body = g_strdup_printf(_("Playing <%s>"), str);
	}

	notif = g_notification_new(_("Playing"));
	g_notification_set_body(notif, body);
	g_free(body);

	return notif;
}

static GNotification *
make_metadata_notification(GvMetadata *metadata)
{
	GNotification *notif;
	const gchar *artist;
	const gchar *title;
	const gchar *album;
	const gchar *year;
	const gchar *genre;
	GString *body;

	if (metadata == NULL)
		return NULL;

	artist = gv_metadata_get_artist(metadata);
	title = gv_metadata_get_title(metadata);
	album = gv_metadata_get_album(metadata);
	year = gv_metadata_get_year(metadata);
	genre = gv_metadata_get_genre(metadata);

	if (title && !artist && !album && !year && !genre) {
		/* If there's only the 'title' field, don't bother being smart,
		 * just display it as it. Actually, most radios fill only this field,
		 * and put everything in it (title + artist + some more stuff).
		 */
		body = g_string_new(title);
	} else {
		/* Otherwise, each existing field is displayed on a line */
		gchar *album_year;

		album_year = gv_metadata_make_album_year(metadata, FALSE);

		if (title == NULL)
			title = _("(Unknown title)");

		body = g_string_new(title);

		if (artist != NULL)
			g_string_append_printf(body, "\n%s", artist);
		if (album_year != NULL)
			g_string_append_printf(body, "\n%s", album_year);
		if (genre != NULL)
			g_string_append_printf(body, "\n%s", genre);

		g_free(album_year);
	}

	notif = g_notification_new(_("Playing"));
	g_notification_set_body(notif, body->str);
	g_string_free(body, TRUE);

	return notif;
}

static GNotification *
make_error_notification(const gchar *message, const gchar *details)
{
	GNotification *notif;
	gchar *body;

	if (details != NULL)
		body = g_strdup_printf("%s\n%s", message, details);
	else
		body = g_strdup(message);

	notif = g_notification_new(_("Error"));
	g_notification_set_body(notif, body);

	g_free(body);

	return notif;
}

/*
 * Signal handlers & callbacks
 */

static void
on_player_notify(GvPlayer *player, GParamSpec *pspec, GvNotifications *self G_GNUC_UNUSED)
{
	GApplication *app = gv_core_application;
	const gchar *property_name = g_param_spec_get_name(pspec);

	if (!g_strcmp0(property_name, "playing")) {
		GNotification *notif;
		GvStation *station;
		gboolean playing;

		playing = gv_player_get_playing(player);

		if (playing == FALSE) {
			g_application_withdraw_notification(app, NOTIF_ID_PLAYING);
			return;
		}

		station = gv_player_get_station(player);
		notif = make_station_notification(station);
		if (notif == NULL)
			return;

		g_application_send_notification(app, NOTIF_ID_PLAYING, notif);
		g_object_unref(notif);
	}
}

static void
on_playback_notify(GvPlayback *playback, GParamSpec *pspec, GvNotifications *self G_GNUC_UNUSED)
{
	GApplication *app = gv_core_application;
	const gchar *property_name = g_param_spec_get_name(pspec);

	if (!g_strcmp0(property_name, "metadata")) {
		GNotification *notif;
		GvMetadata *metadata;

		metadata = gv_playback_get_metadata(playback);
		notif = make_metadata_notification(metadata);
		if (notif == NULL)
			return;

		g_application_send_notification(app, NOTIF_ID_PLAYING, notif);
		g_object_unref(notif);
	}
}

static void
on_errorable_error(GvErrorable *errorable,
		   const gchar *message,
		   const gchar *details,
		   GvNotifications *self)
{
	GApplication *app = gv_core_application;
	GNotification *notif;

	TRACE("%p, %s, %s, %p", errorable, message, details, self);

	notif = make_error_notification(message, details);
	g_application_send_notification(app, NOTIF_ID_ERROR, notif);
	g_object_unref(notif);
}

/*
 * Feature methods
 */

static void
gv_notifications_disable(GvFeature *feature)
{
	GApplication *app = gv_core_application;
	GvPlayer *player = gv_core_player;
	GvPlayback *playback = gv_core_playback;
	GList *item;

	/* Withdraw notifications */
	g_application_withdraw_notification(app, NOTIF_ID_ERROR);
	g_application_withdraw_notification(app, NOTIF_ID_PLAYING);

	/* Disconnect error signal handlers */
	for (item = gv_base_get_objects(); item; item = item->next) {
		GObject *object = G_OBJECT(item->data);

		if (GV_IS_ERRORABLE(object) == FALSE)
			continue;

		g_signal_handlers_disconnect_by_data(object, feature);
	}

	/* Disconnect signal handlers */
	g_signal_handlers_disconnect_by_data(playback, feature);
	g_signal_handlers_disconnect_by_data(player, feature);

	/* Chain up */
	GV_FEATURE_CHAINUP_DISABLE(gv_notifications, feature);
}

static void
gv_notifications_enable(GvFeature *feature)
{
	GvPlayer *player = gv_core_player;
	GvPlayback *playback = gv_core_playback;
	GList *item;

	/* Chain up */
	GV_FEATURE_CHAINUP_ENABLE(gv_notifications, feature);

	/* Connect signal handlers */
	g_signal_connect_object(player, "notify",
			G_CALLBACK(on_player_notify), feature, 0);
	g_signal_connect_object(playback, "notify",
			G_CALLBACK(on_playback_notify), feature, 0);

	/* Connect error signal handlers */
	for (item = gv_base_get_objects(); item; item = item->next) {
		GObject *object = G_OBJECT(item->data);

		if (GV_IS_ERRORABLE(object) == FALSE)
			continue;

		g_signal_connect_object(object, "error",
				G_CALLBACK(on_errorable_error), feature, 0);
	}
}

/*
 * Public methods
 */

GvFeature *
gv_notifications_new(void)
{
	return gv_feature_new(GV_TYPE_NOTIFICATIONS, "Notifications", GV_FEATURE_EARLY);
}

/*
 * GObject methods
 */

static void
gv_notifications_init(GvNotifications *self)
{
	TRACE("%p", self);
}

static void
gv_notifications_class_init(GvNotificationsClass *class)
{
	GvFeatureClass *feature_class = GV_FEATURE_CLASS(class);

	TRACE("%p", class);

	/* Override GvFeature methods */
	feature_class->enable = gv_notifications_enable;
	feature_class->disable = gv_notifications_disable;
}
