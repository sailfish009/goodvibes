/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2017-2021 Arnaud Rebillout
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
#include <gtk/gtk.h>

#include "base/gv-base.h"
#include "core/gv-core.h"
#include "feat/gv-feat.h"
#include "ui/gv-ui.h"

#include "default-stations.h"
#include "gv-graphical-application.h"
#include "options.h"

#if !GLIB_CHECK_VERSION(2, 74, 0)
#define G_APPLICATION_DEFAULT_FLAGS G_APPLICATION_FLAGS_NONE
#endif

/*
 * GObject definitions
 */

struct _GvGraphicalApplication {
	/* Parent instance structure */
	GtkApplication parent_instance;
};

G_DEFINE_TYPE(GvGraphicalApplication, gv_graphical_application, GTK_TYPE_APPLICATION)

/*
 * Public methods
 */

GApplication *
gv_graphical_application_new(const gchar *application_id)
{
	GvGraphicalApplication *self;

	self = g_object_new(GV_TYPE_GRAPHICAL_APPLICATION,
			    "application-id", application_id,
			    "flags", G_APPLICATION_DEFAULT_FLAGS,
			    NULL);

	return G_APPLICATION(self);
}

/*
 * GActions
 */

static void
play_stop_action_cb(GSimpleAction *action G_GNUC_UNUSED,
		    GVariant *parameters G_GNUC_UNUSED,
		    gpointer user_data G_GNUC_UNUSED)
{
	gv_ui_play_stop();
}

static void
add_station_action_cb(GSimpleAction *action G_GNUC_UNUSED,
		      GVariant *parameters G_GNUC_UNUSED,
		      gpointer user_data G_GNUC_UNUSED)
{
	gv_ui_present_add_station();
}

static void
preferences_action_cb(GSimpleAction *action G_GNUC_UNUSED,
		      GVariant *parameters G_GNUC_UNUSED,
		      gpointer user_data G_GNUC_UNUSED)
{
	gv_ui_present_preferences();
}

static void
keyboard_shortcuts_action_cb(GSimpleAction *action G_GNUC_UNUSED,
			     GVariant *parameters G_GNUC_UNUSED,
			     gpointer user_data G_GNUC_UNUSED)
{
	gv_ui_present_keyboard_shortcuts();
}

static void
help_action_cb(GSimpleAction *action G_GNUC_UNUSED,
	       GVariant *parameters G_GNUC_UNUSED,
	       gpointer user_data G_GNUC_UNUSED)
{
	g_app_info_launch_default_for_uri(GV_ONLINE_HELP, NULL, NULL);
}

static void
about_action_cb(GSimpleAction *action G_GNUC_UNUSED,
		GVariant *parameters G_GNUC_UNUSED,
		gpointer user_data G_GNUC_UNUSED)
{
	gv_ui_present_about();
}

static void
close_ui_action_cb(GSimpleAction *action G_GNUC_UNUSED,
		   GVariant *parameters G_GNUC_UNUSED,
		   gpointer user_data G_GNUC_UNUSED)
{
	gv_ui_hide();
}

static void
quit_action_cb(GSimpleAction *action G_GNUC_UNUSED,
	       GVariant *parameters G_GNUC_UNUSED,
	       gpointer user_data G_GNUC_UNUSED)
{
	gv_core_quit();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static const GActionEntry action_entries[] = {
	{ "play-stop", play_stop_action_cb },
	{ "add-station", add_station_action_cb },
	{ "preferences", preferences_action_cb },
	{ "help", help_action_cb },
	{ "about", about_action_cb },
	{ "quit", quit_action_cb },
	{ NULL }
};
static const GActionEntry standalone_action_entries[] = {
	{ "keyboard-shortcuts", keyboard_shortcuts_action_cb },
	{ "close-ui", close_ui_action_cb },
	{ NULL }
};
#pragma GCC diagnostic pop

static void
add_g_action_entries(GApplication *app, gboolean status_icon_mode)
{
	g_action_map_add_action_entries(G_ACTION_MAP(app), action_entries, -1, NULL);

	if (status_icon_mode == FALSE)
		g_action_map_add_action_entries(G_ACTION_MAP(app), standalone_action_entries, -1, NULL);
}

/*
 * Accelerators
 */

struct _GvActionAccel {
	const gchar *action;
	const gchar *accel;
};

typedef struct _GvActionAccel GvActionAccel;

static const GvActionAccel action_accels[] = {
	{ "app.play-stop", "space" },
	{ "app.add-station", "<Primary>a" },
	{ "app.help", "F1" },
	{ "app.close-ui", "<Primary>c" },
	{ "app.quit", "<Primary>q" },
	{ NULL, NULL }
};

static void
setup_accels(GApplication *gapp, gboolean status_icon_mode)
{
	GtkApplication *app = GTK_APPLICATION(gapp);
	const GvActionAccel *a;

	/* No keyboard shortcuts for status icon mode */
	if (status_icon_mode == TRUE)
		return;

	for (a = action_accels; a->action != NULL; a++) {
		const gchar *accels[] = { a->accel, NULL };
		gtk_application_set_accels_for_action(app, a->action, accels);
	}
}

/*
 * GApplication methods
 */

static void
gv_graphical_application_shutdown(GApplication *app)
{
	DEBUG_NO_CONTEXT(">>>> Main loop terminated <<<<");

	/* Cleanup */
	DEBUG_NO_CONTEXT("---- Cleaning up ----");
	gv_feat_cleanup();
	gv_ui_cleanup();
	gv_core_cleanup();
	gv_base_cleanup();

	/* Mandatory chain-up */
	G_APPLICATION_CLASS(gv_graphical_application_parent_class)->shutdown(app);
}

static void
gv_graphical_application_startup(GApplication *app)
{
	DEBUG_NO_CONTEXT("---- Starting application ----");

	/* Mandatory chain-up, see:
	 * https://developer.gnome.org/gtk3/stable/GtkApplication.html#gtk-application-new
	 */
	G_APPLICATION_CLASS(gv_graphical_application_parent_class)->startup(app);

	/* Setup actions */
	add_g_action_entries(app, options.status_icon);
	setup_accels(app, options.status_icon);

	/* Initialization */
	DEBUG_NO_CONTEXT("---- Initializing ----");
	gv_base_init();
	gv_core_init(app, DEFAULT_STATIONS);
	gv_ui_init(app, options.status_icon);
	gv_feat_init();
	gv_base_init_completed();

	/* Configuration */
	DEBUG_NO_CONTEXT("---- Configuring ----");
	gv_feat_configure_early();
	gv_core_configure();
	gv_ui_configure();
	gv_feat_configure_late();

	/* Hold application */
	g_application_hold(app);
}

static gboolean
when_idle_go_player(gpointer user_data)
{
	const gchar *uri_to_play = user_data;

	gv_player_go(gv_core_player, uri_to_play);

	return G_SOURCE_REMOVE;
}

static void
gv_graphical_application_activate(GApplication *app G_GNUC_UNUSED)
{
	static gboolean first_invocation = TRUE;

	if (first_invocation) {
		first_invocation = FALSE;

		DEBUG_NO_CONTEXT(">>>> Main loop started <<<<");

		/* Schedule a callback to play music.
		 * DO NOT start playing now! It's too early!
		 * There's still some init code pending, and we want to ensure
		 * (as much as possible) that this init code is run before we
		 * start the playback. Therefore we schedule with a low priority.
		 */
		g_idle_add_full(G_PRIORITY_LOW, when_idle_go_player,
				(void *) options.uri_to_play, NULL);

		/* Present the main window, depending on options */
		if (!options.without_ui) {
			DEBUG("Presenting main window");
			gv_ui_present_main();
		} else {
			DEBUG("NOT presenting main window (--without-ui)");
		}

	} else {
		/* Present the main window, unconditionally */
		DEBUG("Presenting main window");
		gv_ui_present_main();
	}
}

/*
 * GObject methods
 */

static void
gv_graphical_application_init(GvGraphicalApplication *self)
{
	TRACE("%p", self);
}

static void
gv_graphical_application_class_init(GvGraphicalApplicationClass *class)
{
	GApplicationClass *application_class = G_APPLICATION_CLASS(class);

	TRACE("%p", class);

	/* Override GApplication methods */
	application_class->startup = gv_graphical_application_startup;
	application_class->shutdown = gv_graphical_application_shutdown;
	application_class->activate = gv_graphical_application_activate;
}
