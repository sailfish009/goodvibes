/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2021 Arnaud Rebillout
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

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gv-ui-enum-types.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"

#include "ui/gv-main-window.h"
#include "ui/gv-main-window-standalone.h"

/*
 * Properties
 */

#define DEFAULT_CLOSE_ACTION  GV_MAIN_WINDOW_CLOSE_QUIT

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_PRIMARY_MENU,
	PROP_CLOSE_ACTION,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvMainWindowStandalonePrivate {
	/* Properties */
	GMenuModel *primary_menu;
	GvMainWindowCloseAction close_action;
	/* Widgets */
	GtkHeaderBar *header_bar;
};

typedef struct _GvMainWindowStandalonePrivate GvMainWindowStandalonePrivate;

struct _GvMainWindowStandalone {
	/* Parent instance structure */
	GvMainWindow parent_instance;
	/* Private data */
	GvMainWindowStandalonePrivate *priv;
};

static void gv_main_window_standalone_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GvMainWindowStandalone, gv_main_window_standalone, GV_TYPE_MAIN_WINDOW,
				 G_ADD_PRIVATE(GvMainWindowStandalone)
				 G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
						gv_main_window_standalone_configurable_interface_init))

/*
 * Private methods
 */

static void
gv_main_window_standalone_update_header_bar(GvMainWindowStandalone *self, GvPlayer *player)
{
	GvMainWindowStandalonePrivate *priv = self->priv;
	GtkHeaderBar *header_bar = GTK_HEADER_BAR(priv->header_bar);
	const gchar *default_title = g_get_application_name();
	GvPlaybackState state;
	GvMetadata *metadata;
	GvStation *station;

	/* Here's how we set the title bar:
	 * - not playing? -> application name
	 * - playing? -> track name (if any)
	 *   - else fall back to station name (if any)
	 *   - else fall back to application name
	 */

	state = gv_player_get_playback_state(player);
	if (state == GV_PLAYBACK_STATE_STOPPED) {
		gtk_header_bar_set_title(header_bar, default_title);
		return;
	}

	metadata = gv_player_get_metadata(player);
	if (metadata != NULL) {
		gchar *title;

		title = gv_metadata_make_title_artist(metadata, FALSE);
		if (title != NULL) {
			gtk_header_bar_set_title(header_bar, title);
			g_free(title);
			return;
		}
	}

	station = gv_player_get_station(player);
	if (station != NULL) {
		const gchar *title;

		title = gv_station_get_name_or_uri(station);
		if (title) {
			gtk_header_bar_set_title(header_bar, title);
			return;
		}
	}

	gtk_header_bar_set_title(header_bar, default_title);
}

/*
 * Public methods
 */

GvMainWindow *
gv_main_window_standalone_new(GApplication *application, GMenuModel *primary_menu)
{
	GvMainWindowStandalone *self;

	self = g_object_new(GV_TYPE_MAIN_WINDOW_STANDALONE,
			    "application", application,
			    "primary-menu", primary_menu,
			    NULL);

	return GV_MAIN_WINDOW(self);
}

/*
 * Signal handlers
 */

static void
on_player_notify(GvPlayer *player, GParamSpec *pspec, GvMainWindowStandalone *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", player, property_name, self);

	if (g_strcmp0(property_name, "playback-state") &&
	    g_strcmp0(property_name, "station") &&
	    g_strcmp0(property_name, "metadata"))
		return;

	gv_main_window_standalone_update_header_bar(self, player);
}

static gboolean
on_window_delete_event(GvMainWindowStandalone *self, GdkEvent *event G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	GvMainWindowStandalonePrivate *priv = self->priv;
	GvMainWindowCloseAction action = priv->close_action;

	switch (action) {
	case GV_MAIN_WINDOW_CLOSE_CLOSE:
		/* We just close the window (ie. hide) */
		gtk_widget_hide(GTK_WIDGET(self));
		return GDK_EVENT_STOP;
	case GV_MAIN_WINDOW_CLOSE_QUIT:
	default:
		/* We're quitting, and we do it our own way, meaning that we don't want
		 * GTK to destroy the window, so let's stop the event propagation now.
		 *
		 * We don't even want to hide the window, as the window manager might
		 * need to query the window height and position in order to save it.
		 * Such queries would fail if the window was hidden.
		 */
		gv_core_quit();
		return GDK_EVENT_STOP;
	}
}

/*
 * Property accessors
 */

static void
gv_main_window_standalone_set_primary_menu(GvMainWindowStandalone *self, GMenuModel *primary_menu)
{
	GvMainWindowStandalonePrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_null(priv->primary_menu);
	priv->primary_menu = g_object_ref_sink(primary_menu);
}

GvMainWindowCloseAction
gv_main_window_standalone_get_close_action(GvMainWindowStandalone *self)
{
	GvMainWindowStandalonePrivate *priv = self->priv;
	return priv->close_action;
}

void
gv_main_window_standalone_set_close_action(GvMainWindowStandalone *self, GvMainWindowCloseAction action)
{
	GvMainWindowStandalonePrivate *priv = self->priv;

	if (priv->close_action == action)
		return;

	priv->close_action = action;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CLOSE_ACTION]);
}

static void
gv_main_window_standalone_get_property(GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_CLOSE_ACTION:
		g_value_set_enum(value, gv_main_window_standalone_get_close_action(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_main_window_standalone_set_property(GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_PRIMARY_MENU:
		gv_main_window_standalone_set_primary_menu(self, g_value_get_object(value));
		break;
	case PROP_CLOSE_ACTION:
		gv_main_window_standalone_set_close_action(self, g_value_get_enum(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * GvConfigurable interface
 */

static void
gv_main_window_standalone_configure(GvConfigurable *configurable)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(configurable);

	TRACE("%p", self);

	/* Configure parent. Kind of hacky to do that explicitly here,
	 * but there's nothing in place for that to happen automatically,
	 * and this is the only place where we need it, so live with that.
	 * Eventually this will go away, when gv-main-window-status-icon
	 * is retired, and then gv-main-window-standalone can be merged
	 * into gv-main-window.
	 * Note that calling gv_configurable_configure() on the parent
	 * instance doesn't help, as it just calls this very method,
	 * and we're caught in an endless loop.
	 */
	GvMainWindow *parent = &(self->parent_instance);
	gv_main_window_configure(parent);

	g_assert(gv_ui_settings);
	g_settings_bind(gv_ui_settings, "close-action",
	                self, "close-action", G_SETTINGS_BIND_DEFAULT);
}

static void
gv_main_window_standalone_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_main_window_standalone_configure;
}

/*
* GObject methods
*/

static void
gv_main_window_standalone_setup_header_bar(GvMainWindowStandalone *self)
{
	GvMainWindowStandalonePrivate *priv = self->priv;
	GtkMenuButton *menu_button;
	GtkHeaderBar *header_bar;

	g_assert_nonnull(priv->primary_menu);
	g_assert_null(priv->header_bar);

	menu_button = GTK_MENU_BUTTON(gtk_menu_button_new());
	gtk_menu_button_set_direction(menu_button, GTK_ARROW_NONE);
	gtk_menu_button_set_menu_model(menu_button, priv->primary_menu);

	header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
	gtk_header_bar_set_show_close_button(header_bar, TRUE);
	gtk_header_bar_set_title(header_bar, g_get_application_name());
	gtk_header_bar_pack_end(header_bar, GTK_WIDGET(menu_button));

	gtk_window_set_titlebar(GTK_WINDOW(self), GTK_WIDGET(header_bar));
	gtk_widget_show_all(GTK_WIDGET(header_bar));

	priv->header_bar = header_bar;
}

static void
gv_main_window_standalone_finalize(GObject *object)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(object);
	GvMainWindowStandalonePrivate *priv = self->priv;

	TRACE("%p", object);

	/* Free resources */
	g_clear_object(&priv->primary_menu);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window_standalone, object);
}

static void
gv_main_window_standalone_constructed(GObject *object)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(object);
	GvPlayer *player = gv_core_player;

	TRACE("%p", self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window_standalone, object);

	/* Create the header bar */
	gv_main_window_standalone_setup_header_bar(self);

	/* Connect main window signal handlers */
	g_signal_connect_object(self, "delete-event",
	                        G_CALLBACK(on_window_delete_event), NULL, 0);

	/* Connect core signal handlers */
	g_signal_connect_object(player, "notify",
				G_CALLBACK(on_player_notify), self, 0);
}

static void
gv_main_window_standalone_init(GvMainWindowStandalone *self)
{
       TRACE("%p", self);

       /* Initialize private pointer */
       self->priv = gv_main_window_standalone_get_instance_private(self);
}

static void
gv_main_window_standalone_class_init(GvMainWindowStandaloneClass *class)
{
       GObjectClass *object_class = G_OBJECT_CLASS(class);

       TRACE("%p", class);

       /* Override GObject methods */
       object_class->finalize = gv_main_window_standalone_finalize;
       object_class->constructed = gv_main_window_standalone_constructed;

       /* Properties */
       object_class->get_property = gv_main_window_standalone_get_property;
       object_class->set_property = gv_main_window_standalone_set_property;

       properties[PROP_PRIMARY_MENU] =
	       g_param_spec_object("primary-menu", "Primary menu", NULL,
				   G_TYPE_MENU_MODEL,
				   GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

       properties[PROP_CLOSE_ACTION] =
	       g_param_spec_enum("close-action", "Close Action", NULL,
				 GV_TYPE_MAIN_WINDOW_CLOSE_ACTION,
				 DEFAULT_CLOSE_ACTION,
				 GV_PARAM_READWRITE);

       g_object_class_install_properties(object_class, PROP_N, properties);

       /* Register transform function */
       g_value_register_transform_func(GV_TYPE_MAIN_WINDOW_CLOSE_ACTION,
				       G_TYPE_STRING,
				       gv_value_transform_enum_string);
       g_value_register_transform_func(G_TYPE_STRING,
				       GV_TYPE_MAIN_WINDOW_CLOSE_ACTION,
				       gv_value_transform_string_enum);
}
