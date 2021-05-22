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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gv-ui-enum-types.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-playlist-view.h"
#include "ui/gv-station-view.h"

#include "ui/gv-main-window.h"

#define CSS_NAME "goodvibes-main-window"

/*
 * Properties
 */

#define DEFAULT_THEME_VARIANT GV_MAIN_WINDOW_THEME_DEFAULT

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_NATURAL_HEIGHT,
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
	gint natural_height;
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
 * Private methods
 */

#if 0
static gint
gv_main_window_compute_natural_height(GvMainWindow *self)
{
	GtkWindow *window = GTK_WINDOW(self);
	GtkWidget *tree_view = self->priv->stations_tree_view;
	GtkAllocation allocated;
	GtkRequisition natural;
	gint width, height, diff, natural_height;
	gint min_height = 1;
	gint max_height;

	/* Get maximum height */
#if GTK_CHECK_VERSION(3,22,0)
	GdkDisplay *display;
	GdkWindow *gdk_window;
	GdkMonitor *monitor;
	GdkRectangle geometry;

	display = gdk_display_get_default();
	gdk_window = gtk_widget_get_window(GTK_WIDGET(self));
	if (gdk_window) {
		monitor = gdk_display_get_monitor_at_window(display, gdk_window);
	} else {
		/* In status icon mode, the window might not be realized,
		 * and in any case the window is tied to the panel which
		 * lives on the primiary monitor. */
		monitor = gdk_display_get_primary_monitor(display);
	}
	gdk_monitor_get_workarea(monitor, &geometry);
	max_height = geometry.height;
#else
	GdkScreen *screen;

	screen = gdk_screen_get_default();
	max_height = gdk_screen_get_height(screen);
#endif

	/*
	 * Here comes a hacky piece of code!
	 *
	 * Problem: from the moment the station tree view is within a scrolled
	 * window, the height is not handled smartly by GTK anymore. By default,
	 * it's ridiculously small. Then, when the number of rows in the tree view
	 * is changed, the tree view is not resized. So if we want a smart height,
	 * we have to do it manually.
	 *
	 * The success (or failure) of this function highly depends on the moment
	 * it's called.
	 * - too early, get_preferred_size() calls return junk.
	 * - in some signal handlers, get_preferred_size() calls return junk.
	 *
	 * Plus, we resize the window here, is the context safe to do that?
	 *
	 * For these reasons, it's safer to never call this function directly,
	 * but instead always delay the call for an idle moment.
	 */

	/* Determine if the tree view is at its natural height */
	gtk_widget_get_allocation(tree_view, &allocated);
	gtk_widget_get_preferred_size(tree_view, NULL, &natural);
	//DEBUG("allocated height: %d", allocated.height);
	//DEBUG("natural height: %d", natural.height);
	diff = natural.height - allocated.height;

	/* Determine what should be the new height */
	gtk_window_get_size(window, &width, &height);
	natural_height = height + diff;
	if (natural_height < min_height) {
		DEBUG("Clamping natural height %d to minimum height %d",
		      natural_height, min_height);
		natural_height = min_height;
	}
	if (natural_height > max_height) {
		DEBUG("Clamping natural height %d to maximum height %d",
		      natural_height, max_height);
		natural_height = max_height;
	}

	return natural_height;
}
#endif

/*
 * Core Player signal handlers
 */

static void
on_player_ssl_failure(GvPlayer     *player,
		      const gchar  *error,
		      const gchar  *debug,
                      GvMainWindow *self)
{
	GtkWidget *dialog, *message_area, *grid, *label;
	GvStation *station;
	int result;

	/* Get the station */
	station = gv_player_get_station(player);
	if (station == NULL)
		return;

	/* Create the dialog */
	dialog = gtk_message_dialog_new(GTK_WINDOW(self),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_NONE,
		_("Add a security exception?"));

	gtk_message_dialog_format_secondary_text(
		GTK_MESSAGE_DIALOG(dialog),
		_("An error happened while trying to play %s."),
		gv_station_get_name_or_uri(station));

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Add"), GTK_RESPONSE_ACCEPT);

	/* Append a separator */
	message_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(message_area), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

	/* Then comes more details */
	grid = gtk_grid_new();
        g_object_set(grid,
                     "row-spacing", GV_UI_ELEM_SPACING,
                     "column-spacing", GV_UI_COLUMN_SPACING,
                     NULL);

	label = gtk_label_new(_("URL"));
	gtk_label_set_xalign(GTK_LABEL(label), 1);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	label = gtk_label_new(gv_station_get_uri(station));
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);

	label = gtk_label_new(_("Error"));
	gtk_label_set_xalign(GTK_LABEL(label), 1);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	label = gtk_label_new(error);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);

	(void) debug;    // do not display the debug

	gtk_container_add(GTK_CONTAINER(message_area), grid);
	gtk_widget_show_all(message_area);

	/* Show the dialog */
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	/* Handle result */
	if (result == GTK_RESPONSE_ACCEPT) {
		gv_station_set_insecure(station, TRUE);
		gv_player_play(player);
	} else {
		gv_player_stop(player);
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

#if 0
/*
 * Stations tree view signal handlers
 */

static gboolean
when_idle_compute_natural_height(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	gint natural_height;

	natural_height = gv_main_window_compute_natural_height(self);
	if (natural_height != priv->natural_height) {
		priv->natural_height = natural_height;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_NATURAL_HEIGHT]);
	}

	return G_SOURCE_REMOVE;
}

static void
on_stations_tree_view_populated(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                                GvMainWindow *self)
{
	/* If the content of the stations tree view was modified, the natural size
	 * changed also. However it's too early to compute the new size now.
	 */
	g_idle_add((GSourceFunc) when_idle_compute_natural_height, self);
}

static void
on_stations_tree_view_realize(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                              GvMainWindow *self)
{
	/* When the treeview is realized, we need to check AGAIN if the natural
	 * height we have is correct.
	 */
	g_idle_add((GSourceFunc) when_idle_compute_natural_height, self);
}

static gboolean
on_stations_tree_view_map_event(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                                GdkEvent *event G_GNUC_UNUSED,
                                GvMainWindowManager *self)
{
	/* When the treeview is mapped, we need to check AGAIN if the natural
	 * height we have is correct.
	 */
	g_idle_add((GSourceFunc) when_idle_compute_natural_height, self);

	return GDK_EVENT_PROPAGATE;
}
#endif

/*
 * Main window signal handlers (both popup and standalone)
 */

/*
 * Public methods
 */

void
gv_main_window_play_stop(GvMainWindow *self)
{
	GvPlayer *player = gv_core_player;
	(void) self;
#if 0
	// TODO do we still care?
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	gtk_widget_grab_focus(priv->play_button);
#endif
	gv_player_toggle(player);
}

void
gv_main_window_resize_height(GvMainWindow *self, gint height)
{
	GtkWindow *window = GTK_WINDOW(self);
	gint width;

	DEBUG("Resizing height to %d", height);

	gtk_window_get_size(window, &width, NULL);
	gtk_window_resize(window, width, height);
}

/*
 * Property accessors
 */

gint
gv_main_window_get_natural_height(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = gv_main_window_get_instance_private(self);
	return priv->natural_height;
}

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
gv_main_window_get_property(GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NATURAL_HEIGHT:
		g_value_set_int(value, gv_main_window_get_natural_height(self));
		break;
	case PROP_THEME_VARIANT:
		g_value_set_enum(value, gv_main_window_get_theme_variant(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_main_window_set_property(GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
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
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_main_window_finalize;
	object_class->constructed = gv_main_window_constructed;

	/* Set a css name, so that it's possible to theme the main window */
	gtk_widget_class_set_css_name(widget_class, CSS_NAME);

	/* Properties */
	object_class->get_property = gv_main_window_get_property;
	object_class->set_property = gv_main_window_set_property;

	properties[PROP_NATURAL_HEIGHT] =
	        g_param_spec_int("natural-height", "Natural height", NULL,
	                         0, G_MAXINT, 0,
	                         GV_PARAM_READABLE);

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
