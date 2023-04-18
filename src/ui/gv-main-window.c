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
#include <gtk/gtk.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gv-playlist-view.h"
#include "ui/gv-station-view.h"
#include "ui/gv-ui-enum-types.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"

#include "ui/gv-main-window.h"

#define USER_CSS_FILENAME "style.css"

/*
 * Properties
 */

#define DEFAULT_THEME_VARIANT GV_MAIN_WINDOW_THEME_DEFAULT

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_THEME_VARIANT,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvMainWindowPrivate {
	/* Properties */
	GvMainWindowThemeVariant theme_variant;
	/* Widgets */
	GtkWidget *stack;
	GtkWidget *playlist_view;
	GtkWidget *station_view;
	/* Internal */
	gboolean system_prefer_dark_theme;
};

typedef struct _GvMainWindowPrivate GvMainWindowPrivate;

static void gv_main_window_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GvMainWindow, gv_main_window, GTK_TYPE_APPLICATION_WINDOW,
				 G_ADD_PRIVATE(GvMainWindow)
				 G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
						gv_main_window_configurable_interface_init))

/*
 * Core Player signal handlers
 */

static GtkWidget *
make_security_exception_dialog(GtkWindow *parent, GvStation *station, const gchar *uri)
{
	GtkWidget *dialog, *message_area, *grid, *label;
	const gchar *station_uri = gv_station_get_uri(station);
	guint col;

	/* Create the dialog */
	dialog = gtk_message_dialog_new(parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_NONE,
					_("Add a Security Exception?"));

	gtk_message_dialog_format_secondary_text(
		GTK_MESSAGE_DIALOG(dialog),
		_("The TLS certificate for this station is not valid."
			" The issue is most likely a misconfiguration of the website."));

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Continue"), GTK_RESPONSE_ACCEPT);

	/* Append a separator */
	message_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(message_area), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

	/* Then comes the error details */
	grid = gtk_grid_new();
	g_object_set(grid,
		     "row-spacing", GV_UI_ELEM_SPACING,
		     "column-spacing", GV_UI_COLUMN_SPACING,
		     NULL);

	col = 0;

	label = gtk_label_new(_("Station URL"));
	gtk_label_set_xalign(GTK_LABEL(label), 1);
	gtk_grid_attach(GTK_GRID(grid), label, 0, col, 1, 1);

	label = gtk_label_new(station_uri);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_grid_attach(GTK_GRID(grid), label, 1, col, 1, 1);

	if (uri && g_strcmp0(uri, station_uri) != 0) {
		col += 1;

		label = gtk_label_new(_("Redirection"));
		gtk_label_set_xalign(GTK_LABEL(label), 1);
		gtk_grid_attach(GTK_GRID(grid), label, 0, col, 1, 1);

		label = gtk_label_new(uri);
		gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_label_set_xalign(GTK_LABEL(label), 0);
		gtk_grid_attach(GTK_GRID(grid), label, 1, col, 1, 1);
	}

	/* Pack it */
	gtk_container_add(GTK_CONTAINER(message_area), grid);
	gtk_widget_show_all(message_area);

	return dialog;
}

static gboolean
when_idle_play(gpointer user_data)
{
	GvPlayer *player = GV_PLAYER(user_data);
	gv_player_play(player);
	return FALSE;
}

static void
on_player_ssl_failure(GvPlayer *player,
		      const gchar *uri,
		      GvMainWindow *self)
{
	GvStation *station;
	GtkWidget *dialog;
	int response;

	/* Get the station */
	station = gv_player_get_station(player);
	if (station == NULL)
		return;

	/* Create and run the dialog */
	dialog = make_security_exception_dialog(GTK_WINDOW(self), station, uri);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	/* Handle response */
	if (response == GTK_RESPONSE_ACCEPT) {
		gv_station_set_insecure(station, TRUE);
		// XXX we need to do it async, as we're called from a signal
		// handler, and we want to let it return before playing.
		// However I don't think the caller should know or care about
		// that. Maybe gv_player_play should always call g_idle_add,
		// that would solve the problem once and for all.
		g_idle_add(when_idle_play, player);
	}
}

/*
 * Widget signal handlers
 */

static void
on_go_next_button_clicked(GvPlaylistView *playlist_view G_GNUC_UNUSED, GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	GtkStack *stack = GTK_STACK(priv->stack);

	gtk_stack_set_visible_child(stack, priv->station_view);
}

static void
on_go_back_button_clicked(GvStationView *station_view G_GNUC_UNUSED, GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	GtkStack *stack = GTK_STACK(priv->stack);

	gtk_stack_set_visible_child(stack, priv->playlist_view);
}

/*
 * Property accessors
 */

GvMainWindowThemeVariant
gv_main_window_get_theme_variant(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	return priv->theme_variant;
}

void
gv_main_window_set_theme_variant(GvMainWindow *self, GvMainWindowThemeVariant variant)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	gboolean prefer_dark_theme;

	if (priv->theme_variant == variant)
		return;

	priv->theme_variant = variant;

	switch (variant) {
	case GV_MAIN_WINDOW_THEME_DARK:
		prefer_dark_theme = TRUE;
		break;
	case GV_MAIN_WINDOW_THEME_LIGHT:
		prefer_dark_theme = FALSE;
		break;
	case GV_MAIN_WINDOW_THEME_DEFAULT:
	default:
		prefer_dark_theme = priv->system_prefer_dark_theme;
		break;
	}

	g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme",
		     prefer_dark_theme, NULL);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_THEME_VARIANT]);
}

static void
gv_main_window_get_property(GObject *object,
			    guint property_id,
			    GValue *value,
			    GParamSpec *pspec)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_THEME_VARIANT:
		g_value_set_enum(value, gv_main_window_get_theme_variant(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_main_window_set_property(GObject *object,
			    guint property_id,
			    const GValue *value,
			    GParamSpec *pspec)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_THEME_VARIANT:
		gv_main_window_set_theme_variant(self, g_value_get_enum(value));
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
_gv_main_window_configure(GvConfigurable *configurable)
{
	GvMainWindow *self = GV_MAIN_WINDOW(configurable);
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);

	TRACE("%p", self);

	/* We want to save the value of 'prefer-dark-theme' from GtkSettings
	 * right now, as we might modify it later, and need a way to go back
	 * to its default value.
	 */
	g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme",
		     &priv->system_prefer_dark_theme, NULL);

	g_assert(gv_ui_settings);
	g_settings_bind(gv_ui_settings, "theme-variant",
			self, "theme-variant", G_SETTINGS_BIND_DEFAULT);
}

void
gv_main_window_configure(GvMainWindow *self)
{
	_gv_main_window_configure(GV_CONFIGURABLE(self));
}

static void
gv_main_window_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = _gv_main_window_configure;
}

/*
 * GObject methods
 */

static void
gv_main_window_populate_widgets(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);

	/* Create the views */
	priv->playlist_view = gv_playlist_view_new();
	priv->station_view = gv_station_view_new();

	/* Create a stack and populate it */
	priv->stack = gtk_stack_new();
	gtk_stack_set_transition_type(GTK_STACK(priv->stack),
				      GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
	gtk_stack_add_named(GTK_STACK(priv->stack), priv->playlist_view, "playlist-view");
	gtk_stack_add_named(GTK_STACK(priv->stack), priv->station_view, "station-view");

	/* Add stack to the window */
	gtk_container_add(GTK_CONTAINER(self), priv->stack);
	gtk_widget_show_all(priv->stack);
}

static void
gv_main_window_setup_widgets(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);

	/* Connect buttons to switch between stack children */
	g_signal_connect_object(priv->playlist_view, "go-next-clicked",
				G_CALLBACK(on_go_next_button_clicked), self, 0);
	g_signal_connect_object(priv->station_view, "go-back-clicked",
				G_CALLBACK(on_go_back_button_clicked), self, 0);
}

static void
gv_main_window_setup_css(GvMainWindow *self G_GNUC_UNUSED)
{
	GtkCssProvider *provider;
	const gchar *user_dir;
	gchar *css_path;

	user_dir = gv_get_app_user_data_dir();
	css_path = g_build_filename(user_dir, USER_CSS_FILENAME, NULL);

	if (g_file_test(css_path, G_FILE_TEST_EXISTS) == FALSE) {
		g_free(css_path);
		return;
	}

	INFO("Loading css from file '%s'", css_path);

	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(provider, css_path, NULL);
	gtk_style_context_add_provider_for_screen(
		gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);

	g_object_unref(provider);
	g_free(css_path);
}

static void
gv_main_window_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window, object);
}

static void
gv_main_window_constructed(GObject *object)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);
	GvPlayer *player = gv_core_player;

	TRACE("%p", self);

	/* Build window */
	gv_main_window_populate_widgets(self);
	gv_main_window_setup_widgets(self);
	gv_main_window_setup_css(self);

	g_signal_connect_object(player, "ssl-failure",
				G_CALLBACK(on_player_ssl_failure), self, 0);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window, object);
}

static void
gv_main_window_init(GvMainWindow *self)
{
	TRACE("%p", self);
}

static void
gv_main_window_class_init(GvMainWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_main_window_finalize;
	object_class->constructed = gv_main_window_constructed;

	/* Properties */
	object_class->get_property = gv_main_window_get_property;
	object_class->set_property = gv_main_window_set_property;

	properties[PROP_THEME_VARIANT] =
		g_param_spec_enum("theme-variant", "Theme variant", NULL,
				  GV_TYPE_MAIN_WINDOW_THEME_VARIANT,
				  DEFAULT_THEME_VARIANT,
				  GV_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Register transform function */
	g_value_register_transform_func(GV_TYPE_MAIN_WINDOW_THEME_VARIANT,
					G_TYPE_STRING,
					gv_value_transform_enum_string);
	g_value_register_transform_func(G_TYPE_STRING,
					GV_TYPE_MAIN_WINDOW_THEME_VARIANT,
					gv_value_transform_string_enum);
}
