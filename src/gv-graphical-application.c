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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <amtk/amtk.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gv-ui.h"
#include "feat/gv-feat.h"

#include "gv-graphical-application.h"
#include "default-stations.h"
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
play_stop_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                    GVariant      *parameters G_GNUC_UNUSED,
                    gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_play_stop();
}

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
keyboard_shortcuts_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                             GVariant      *parameters G_GNUC_UNUSED,
                             gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_present_keyboard_shortcuts();
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
 * AmtkActions
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static const AmtkActionInfoEntry action_info_entries[] = {
	{ "app.play-stop", NULL, N_("Play/Stop"), "space" },
	{ "app.add-station", NULL, N_("Add Station"), "<Control>a" },
	{ "app.preferences", NULL, N_("Preferences") },
	{ "app.help", NULL, N_("Online Help"), "F1" },
	{ "app.about", NULL, N_("About") },
	{ "app.quit", NULL, N_("Quit"), "<Control>q" },
	{ NULL }
};
static const AmtkActionInfoEntry standalone_action_info_entries[] = {
	{ "app.keyboard-shortcuts", NULL, N_("_Keyboard Shortcuts") },
	{ "app.close-ui", NULL, N_("Close"), "<Control>c" },
	{ NULL }
};
#pragma GCC diagnostic pop

static void
add_amtk_action_info_entries(GApplication *app, gboolean status_icon_mode)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(app);
	GvGraphicalApplicationPrivate *priv = self->priv;
	AmtkActionInfoStore *store = priv->menu_action_info_store;

	amtk_action_info_store_add_entries(store, action_info_entries, -1, GETTEXT_PACKAGE);

	if (status_icon_mode == FALSE) {
		amtk_action_info_store_add_entries(store, standalone_action_info_entries, -1, GETTEXT_PACKAGE);
		amtk_action_info_store_set_all_accels_to_app(store, GTK_APPLICATION(app));
	}
}

static void
check_amtk_action_info_entries_all_used(GApplication *app)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(app);
	GvGraphicalApplicationPrivate *priv = self->priv;
	AmtkActionInfoStore *store = priv->menu_action_info_store;

	amtk_action_info_store_check_all_used(store);
}

/*
 * Menu model
 */

static GMenuModel *
make_primary_menu(gboolean status_icon_mode)
{
	GMenu *menu;
	GMenu *section;
	GMenuItem *item;
	AmtkFactory *factory;

	factory = amtk_factory_new(NULL);

	menu = g_menu_new();
	g_object_force_floating(G_OBJECT(menu));

	section = g_menu_new();
	item = amtk_factory_create_gmenu_item(factory, "app.play-stop");
	amtk_gmenu_append_item(section, item);
	item = amtk_factory_create_gmenu_item(factory, "app.add-station");
	amtk_gmenu_append_item(section, item);
	amtk_gmenu_append_section(menu, NULL, section);

	section = g_menu_new();
	item = amtk_factory_create_gmenu_item(factory, "app.preferences");
	amtk_gmenu_append_item(section, item);
	amtk_gmenu_append_section(menu, NULL, section);

	section = g_menu_new();
	if (status_icon_mode == FALSE) {
		item = amtk_factory_create_gmenu_item(factory, "app.keyboard-shortcuts");
		amtk_gmenu_append_item(section, item);
	}
	item = amtk_factory_create_gmenu_item(factory, "app.help");
	amtk_gmenu_append_item(section, item);
	item = amtk_factory_create_gmenu_item(factory, "app.about");
	amtk_gmenu_append_item(section, item);
	if (status_icon_mode == FALSE) {
		item = amtk_factory_create_gmenu_item(factory, "app.close-ui");
		amtk_gmenu_append_item(section, item);
	}
	item = amtk_factory_create_gmenu_item(factory, "app.quit");
	amtk_gmenu_append_item(section, item);
	amtk_gmenu_append_section(menu, NULL, section);

	g_menu_freeze(menu);

	g_object_unref(factory);

	return G_MENU_MODEL(menu);
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
	GMenuModel *primary_menu;

	DEBUG_NO_CONTEXT("---- Starting application ----");

	/* Mandatory chain-up, see:
	 * https://developer.gnome.org/gtk3/stable/GtkApplication.html#gtk-application-new
	 */
	G_APPLICATION_CLASS(gv_graphical_application_parent_class)->startup(app);

	/* Setup actions and menus */
	add_g_action_entries(app, options.status_icon);
        add_amtk_action_info_entries(app, options.status_icon);
	primary_menu = make_primary_menu(options.status_icon);

	/* Initialization */
	DEBUG_NO_CONTEXT("---- Initializing ----");
	gv_base_init();
	gv_core_init(app, DEFAULT_STATIONS);
	gv_ui_init(app, primary_menu, options.status_icon);
	gv_feat_init();
	gv_base_init_completed();

	/* Make sure that all menu entries were used */
	check_amtk_action_info_entries_all_used(app);

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
