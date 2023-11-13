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

#include <stdio.h>
#include <unistd.h>

#include <glib-object.h>
#include <glib.h>

#include "base/glib-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"

#include "feat/gv-console-output.h"

/*
 * GObject definitions
 */

struct _GvConsoleOutput {
	/* Parent instance structure */
	GvFeature parent_instance;
};

G_DEFINE_TYPE(GvConsoleOutput, gv_console_output, GV_TYPE_FEATURE)

/*
 * Helpers
 */

#define PRINT(fmt, ...) g_print(fmt "\n", ##__VA_ARGS__)

static const gchar *
time_now(void)
{
	static gchar *now_str;
	GDateTime *now;

	g_free(now_str);

	now = g_date_time_new_now_local();
	now_str = g_date_time_format(now, "%T");
	g_date_time_unref(now);

	return now_str;
}

static void
print_hello_line(void)
{
	PRINT("---- " GV_NAME_CAPITAL " " PACKAGE_VERSION " ----");
	PRINT("Hit Ctrl+C to quit...");
}

static void
print_goodbye_line(void)
{
	PRINT("---- Bye ----");
}

static void
print_error(const gchar *message, const gchar *details)
{
	PRINT(VT_BOLD("Error!") " %s", message);
	if (details != NULL)
		PRINT("       %s", details);
}

static void
print_station(GvStation *station)
{
	const gchar *str;

	if (station == NULL)
		return;

	str = gv_station_get_name(station);
	if (str) {
		PRINT(VT_BOLD("> %s Playing %s"), time_now(), str);
	} else {
		str = gv_station_get_uri(station);
		PRINT(VT_BOLD("> %s Playing <%s>"), time_now(), str);
	}
}

static void
print_metadata(GvMetadata *metadata)
{
	const gchar *artist;
	const gchar *title;
	const gchar *album;
	const gchar *year;
	const gchar *genre;

	if (metadata == NULL)
		return;

	artist = gv_metadata_get_artist(metadata);
	title = gv_metadata_get_title(metadata);
	album = gv_metadata_get_album(metadata);
	year = gv_metadata_get_year(metadata);
	genre = gv_metadata_get_genre(metadata);

	/* Ensure this first line is printed, with a timestamp */
	if (title == NULL)
		title = "(Unknown title)";

	PRINT(". %s %s", time_now(), title);

	/* Other fields are optional */
	if (artist)
		PRINT("           %s", artist);

	if (album && year)
		PRINT("           %s (%s)", album, year);
	else if (album)
		PRINT("           %s", album);
	else if (year)
		PRINT("           (%s)", year);

	if (genre)
		PRINT("           %s", genre);
}

/*
 * Signal handlers
 */

static void
on_playback_notify(GvPlayback *playback, GParamSpec *pspec, GvConsoleOutput *self G_GNUC_UNUSED)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	if (!g_strcmp0(property_name, "state")) {
		GvPlaybackState state;

		state = gv_playback_get_state(playback);

		if (state == GV_PLAYBACK_STATE_PLAYING) {
			GvStation *station;

			station = gv_playback_get_station(playback);
			print_station(station);
		}
	} else if (!g_strcmp0(property_name, "metadata")) {
		GvMetadata *metadata;

		metadata = gv_playback_get_metadata(playback);
		print_metadata(metadata);
	}
}

static void
on_errorable_error(GvErrorable *errorable G_GNUC_UNUSED, const gchar *message,
		   const gchar *details, GvConsoleOutput *self G_GNUC_UNUSED)
{
	print_error(message, details);
}

/*
 * GvFeature methods
 */

static void
gv_console_output_disable(GvFeature *feature)
{
	GvPlayback *playback = gv_core_playback;
	GList *item;

	/* Disconnect error signal handlers */
	for (item = gv_base_get_objects(); item; item = item->next) {
		GObject *object = G_OBJECT(item->data);

		if (GV_IS_ERRORABLE(object) == FALSE)
			continue;

		g_signal_handlers_disconnect_by_data(object, feature);
	}

	/* Disconnect playback signal handlers */
	g_signal_handlers_disconnect_by_data(playback, feature);

	/* Say good-bye */
	print_goodbye_line();

	/* Chain up */
	GV_FEATURE_CHAINUP_DISABLE(gv_console_output, feature);
}

static void
gv_console_output_enable(GvFeature *feature)
{
	GvPlayback *playback = gv_core_playback;
	GList *item;

	/* Chain up */
	GV_FEATURE_CHAINUP_ENABLE(gv_console_output, feature);

	/* Say hello */
	print_hello_line();

	/* Connect playback signal handlers */
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
gv_console_output_new(void)
{
	return gv_feature_new(GV_TYPE_CONSOLE_OUTPUT, "ConsoleOutput", GV_FEATURE_EARLY);
}

/*
 * GObject methods
 */

static void
gv_console_output_init(GvConsoleOutput *self)
{
	TRACE("%p", self);
}

static void
gv_console_output_class_init(GvConsoleOutputClass *class)
{
	GvFeatureClass *feature_class = GV_FEATURE_CLASS(class);

	TRACE("%p", class);

	/* Override GvFeature methods */
	feature_class->enable = gv_console_output_enable;
	feature_class->disable = gv_console_output_disable;
}
