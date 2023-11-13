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

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gv-ui-enum-types.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"

#include "ui/gv-main-window-standalone.h"
#include "ui/gv-main-window.h"

#define APP_MENU_RESOURCE_PATH GV_APPLICATION_PATH "/Ui/app-menu.ui"

/*
 * Properties
 */

#define DEFAULT_CLOSE_ACTION GV_MAIN_WINDOW_CLOSE_QUIT

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
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
gv_main_window_standalone_update_header_bar(GvMainWindowStandalone *self, GvPlayback *playback)
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

	state = gv_playback_get_state(playback);
	if (state == GV_PLAYBACK_STATE_STOPPED) {
		gtk_header_bar_set_title(header_bar, default_title);
		return;
	}

	metadata = gv_playback_get_metadata(playback);
	if (metadata != NULL) {
		gchar *title;

		title = gv_metadata_make_title_artist(metadata, FALSE);
		if (title != NULL) {
			gtk_header_bar_set_title(header_bar, title);
			g_free(title);
			return;
		}
	}

	station = gv_playback_get_station(playback);
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
gv_main_window_standalone_new(GApplication *application)
{
	GvMainWindowStandalone *self;

	self = g_object_new(GV_TYPE_MAIN_WINDOW_STANDALONE,
			    "application", application,
			    NULL);

	return GV_MAIN_WINDOW(self);
}

/*
 * Signal handlers
 */

static void
on_playback_notify(GvPlayback *playback, GParamSpec *pspec, GvMainWindowStandalone *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", playback, property_name, self);

	if (g_strcmp0(property_name, "state") &&
	    g_strcmp0(property_name, "station") &&
	    g_strcmp0(property_name, "metadata"))
		return;

	gv_main_window_standalone_update_header_bar(self, playback);
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
gv_main_window_standalone_get_property(GObject *object,
				       guint property_id,
				       GValue *value,
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
gv_main_window_standalone_set_property(GObject *object,
				       guint property_id,
				       const GValue *value,
				       GParamSpec *pspec)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
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

static GtkWidget *
make_menu_button(void)
{
	GtkBuilder *builder;
	GMenuModel *menu_model;
	GtkMenuButton *menu_button;

	builder = gtk_builder_new_from_resource(APP_MENU_RESOURCE_PATH);
	menu_model = G_MENU_MODEL(gtk_builder_get_object(builder, "app-menu"));

	menu_button = GTK_MENU_BUTTON(gtk_menu_button_new());
	gtk_menu_button_set_direction(menu_button, GTK_ARROW_NONE);
	gtk_menu_button_set_menu_model(menu_button, menu_model);

	g_object_unref(builder);

	return GTK_WIDGET(menu_button);
}

static void
gv_main_window_standalone_setup_header_bar(GvMainWindowStandalone *self)
{
	GvMainWindowStandalonePrivate *priv = self->priv;
	GtkHeaderBar *header_bar;
	GtkWidget *menu_button;

	g_assert(priv->header_bar == NULL);

	header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
	gtk_header_bar_set_show_close_button(header_bar, TRUE);
	gtk_header_bar_set_title(header_bar, g_get_application_name());

	menu_button = make_menu_button();
	gtk_header_bar_pack_end(header_bar, menu_button);

	gtk_window_set_titlebar(GTK_WINDOW(self), GTK_WIDGET(header_bar));
	gtk_widget_show_all(GTK_WIDGET(header_bar));

	priv->header_bar = header_bar;
}

static void
gv_main_window_standalone_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window_standalone, object);
}

static void
gv_main_window_standalone_constructed(GObject *object)
{
	GvMainWindowStandalone *self = GV_MAIN_WINDOW_STANDALONE(object);
	GvPlayback *playback = gv_core_playback;

	TRACE("%p", self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window_standalone, object);

	/* Create the header bar */
	gv_main_window_standalone_setup_header_bar(self);

	/* Connect main window signal handlers */
	g_signal_connect_object(self, "delete-event",
				G_CALLBACK(on_window_delete_event), NULL, 0);

	/* Connect core signal handlers */
	g_signal_connect_object(playback, "notify",
				G_CALLBACK(on_playback_notify), self, 0);
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
