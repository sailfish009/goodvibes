/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2017-2020 Arnaud Rebillout
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
#include <gtk/gtk.h>
#include <amtk/amtk.h>

#include "framework/glib-object-additions.h"
#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gv-ui.h"
#include "feat/gv-feat.h"

#include "gv-graphical-application.h"
#include "options.h"

/*
 * GObject definitions
 */

struct _GvGraphicalApplicationPrivate {
	AmtkActionInfoStore *menu_action_info_store;
};

typedef struct _GvGraphicalApplicationPrivate GvGraphicalApplicationPrivate;

struct _GvGraphicalApplication {
	/* Parent instance structure */
	GtkApplication parent_instance;
	/* Private data */
        GvGraphicalApplicationPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvGraphicalApplication, gv_graphical_application, GTK_TYPE_APPLICATION)

/*
 * Public methods
 */

GApplication *
gv_graphical_application_new(const gchar *application_id)
{
	return G_APPLICATION(g_object_new(GV_TYPE_GRAPHICAL_APPLICATION,
	                                  "application-id", application_id,
	                                  "flags", G_APPLICATION_FLAGS_NONE,
	                                  NULL));
}

/*
 * GActions
 */

static void
add_station_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                      GVariant      *parameters G_GNUC_UNUSED,
                      gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_present_add_station();
}

static void
preferences_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                      GVariant      *parameters G_GNUC_UNUSED,
                      gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_present_preferences();
}

static void
help_action_cb(GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameters G_GNUC_UNUSED,
               gpointer       user_data G_GNUC_UNUSED)
{
	g_app_info_launch_default_for_uri(GV_ONLINE_HELP, NULL, NULL);
}

static void
about_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                GVariant      *parameters G_GNUC_UNUSED,
                gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_present_about();
}

static void
close_ui_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                   GVariant      *parameters G_GNUC_UNUSED,
                   gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_hide();
}

static void
quit_action_cb(GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameters G_GNUC_UNUSED,
               gpointer       user_data G_GNUC_UNUSED)
{
	gv_core_quit();
}

static void
add_g_action_entries(GApplication *app, gboolean status_icon_mode)
{
	const GActionEntry entries[] = {
		{ "add-station", add_station_action_cb, NULL, NULL, NULL, {0} },
		{ "preferences", preferences_action_cb, NULL, NULL, NULL, {0} },
		{ "help",        help_action_cb,        NULL, NULL, NULL, {0} },
		{ "about",       about_action_cb,       NULL, NULL, NULL, {0} },
		{ "close-ui",    close_ui_action_cb,    NULL, NULL, NULL, {0} },
		{ "quit",        quit_action_cb,        NULL, NULL, NULL, {0} },
		{ NULL,          NULL,                  NULL, NULL, NULL, {0} }
	};

	/* Add actions to the application */
	amtk_action_map_add_action_entries_check_dups(G_ACTION_MAP(app), entries, -1, NULL);

	/* The 'close-ui' action makes no sense in the status icon mode */
	if (status_icon_mode)
		g_action_map_remove_action(G_ACTION_MAP(app), "close-ui");
}

/*
 * AmtkActions
 */

static void
add_amtk_action_info_entries(GApplication *app)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(app);
	GvGraphicalApplicationPrivate *priv = self->priv;
	AmtkActionInfoStore *store = priv->menu_action_info_store;

	/* Actions related to the whole application */
	const AmtkActionInfoEntry entries[] = {
		{ "app.add-station",      NULL, _("Add Station"), "<Control>a", NULL, {0} },
		{ "app.preferences",      NULL, _("Preferences"), NULL, NULL, {0} },
		{ "app.help",             NULL, _("Online Help"), "F1", NULL, {0} },
		{ "app.about",            NULL, _("About"),       NULL, NULL, {0} },
		{ "app.close-ui",         NULL, _("Close"),       "<Control>c", NULL, {0} },
		{ "app.quit",             NULL, _("Quit"),        "<Control>q", NULL, {0} },
		{ NULL,                   NULL, NULL,             NULL, NULL, {0} }
	};

	amtk_action_info_store_add_entries(store, entries, -1, GETTEXT_PACKAGE);
        amtk_action_info_store_set_all_accels_to_app(store, GTK_APPLICATION(app));

	// TODO when to run that? after widget_show_all()?
        //amtk_action_info_store_check_all_used(store);
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
	gv_framework_cleanup();

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

	/* Add actions to the application */
	add_g_action_entries(app, options.status_icon);
        add_amtk_action_info_entries(app);

	/* Initialization */
	DEBUG_NO_CONTEXT("---- Initializing ----");
	gv_framework_init();
	gv_core_init(app);
	gv_ui_init(app, options.status_icon);
	gv_feat_init();
	gv_framework_init_completed();

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
		 * DO NOT start playing now ! It's too early !
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
gv_graphical_application_finalize(GObject *object)
{
        GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(object);
        GvGraphicalApplicationPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Free resources */
	g_clear_object(&priv->menu_action_info_store);
	amtk_finalize();

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_graphical_application, object);
}

static void
gv_graphical_application_constructed(GObject *object)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(object);
	GvGraphicalApplicationPrivate *priv = self->priv;

	TRACE("%p", self);

	/* Allocate resources */
	amtk_init();
	priv->menu_action_info_store = amtk_action_info_store_new();

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_graphical_application, object);
}

static void
gv_graphical_application_init(GvGraphicalApplication *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_graphical_application_get_instance_private(self);
}

static void
gv_graphical_application_class_init(GvGraphicalApplicationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GApplicationClass *application_class = G_APPLICATION_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_graphical_application_finalize;
	object_class->constructed = gv_graphical_application_constructed;

	/* Override GApplication methods */
	application_class->startup  = gv_graphical_application_startup;
	application_class->shutdown = gv_graphical_application_shutdown;
	application_class->activate = gv_graphical_application_activate;
}
