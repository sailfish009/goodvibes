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
#include "ui/gv-ui-helpers.h"
#if 0
#include "ui/gv-stations-tree-view.h"
#endif

#include "ui/gv-main-window-status-icon.h"
#include "ui/gv-main-window.h"

/*
 * GObject definitions
 */

struct _GvMainWindowStatusIconPrivate {
	GtkWidget *stations_tree_view;
};

typedef struct _GvMainWindowStatusIconPrivate GvMainWindowStatusIconPrivate;

struct _GvMainWindowStatusIcon {
	/* Parent instance structure */
	GvMainWindow parent_instance;
	/* Private data */
	GvMainWindowStatusIconPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvMainWindowStatusIcon, gv_main_window_status_icon, GV_TYPE_MAIN_WINDOW)

/*
 * Helpers
 */

/* https://stackoverflow.com/a/23497087 */
static GtkWidget *
find_child(GtkWidget *parent, const gchar *name)
{
	const gchar *widget_name;

	widget_name = gtk_widget_get_name(parent);
	if (g_strcmp0(widget_name, name) == 0) {
		return parent;
	}

	if (GTK_IS_BIN(parent)) {
		GtkWidget *child;

		child = gtk_bin_get_child(GTK_BIN(parent));
		if (child)
			return find_child(child, name);
		else
			return NULL;
	}

	if (GTK_IS_CONTAINER(parent)) {
		GList *children, *child;
		GtkWidget *found = NULL;

		children = gtk_container_get_children(GTK_CONTAINER(parent));
		for (child = children; child != NULL; child = g_list_next(child)) {
			GtkWidget *w;
			w = find_child(child->data, name);
			if (w != NULL) {
				found = w;
				break;
			}
		}
		g_list_free(children);
		if (found != NULL)
			return found;
	}

	return NULL;
}

/*
 * All the mess to automatically resize the window
 */

static gint
get_screen_max_height(GtkWindow *self_window)
{
	gint max_height;

#if GTK_CHECK_VERSION(3, 22, 0)
	GdkDisplay *display;
	GdkWindow *gdk_window;
	GdkMonitor *monitor;
	GdkRectangle geometry;

	display = gdk_display_get_default();
	gdk_window = gtk_widget_get_window(GTK_WIDGET(self_window));
	if (gdk_window) {
		monitor = gdk_display_get_monitor_at_window(display, gdk_window);
	} else {
		/* In status icon mode, the window might not be realized,
		 * and in any case the window is tied to the panel which
		 * lives on the primary monitor. */
		monitor = gdk_display_get_primary_monitor(display);
	}
	gdk_monitor_get_workarea(monitor, &geometry);
	max_height = geometry.height;
#else
	GdkScreen *screen;

	screen = gdk_screen_get_default();
	max_height = gdk_screen_get_height(screen);
#endif

	return max_height;
}

static gint
compute_natural_height(GtkWindow *window, GtkWidget *tree_view)
{
	GtkAllocation allocated;
	GtkRequisition natural;
	gint width, height, diff, natural_height;
	gint min_height;
	gint max_height;

	min_height = 1;
	max_height = get_screen_max_height(window);

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

static void
gv_main_window_status_icon_resize(GvMainWindowStatusIcon *self)
{
	GvMainWindowStatusIconPrivate *priv = self->priv;
	GtkWindow *window = GTK_WINDOW(self);
	gint new_height;
	gint height;
	gint width;

	new_height = compute_natural_height(window, priv->stations_tree_view);
	gtk_window_get_size(window, &width, &height);
	DEBUG("Resizing window height: %d -> %d", height, new_height);
	gtk_window_resize(window, width, new_height);
}

static gboolean
when_idle_resize_window(GvMainWindowStatusIcon *self)
{
	gv_main_window_status_icon_resize(self);
	return G_SOURCE_REMOVE;
}

static void
on_stations_tree_view_populated(GtkWidget *stations_tree_view G_GNUC_UNUSED,
				GvMainWindowStatusIcon *self)
{
	/* If the content of the stations tree view was modified, the natural size
	 * changed also. However it's too early to compute the new size now.
	 */
	g_idle_add(G_SOURCE_FUNC(when_idle_resize_window), self);
}

static void
on_stations_tree_view_realize(GtkWidget *stations_tree_view G_GNUC_UNUSED,
			      GvMainWindowStatusIcon *self)
{
	/* When the treeview is realized, we need to check AGAIN if the natural
	 * height we have is correct.
	 */
	g_idle_add(G_SOURCE_FUNC(when_idle_resize_window), self);
}

static gboolean
on_stations_tree_view_map_event(GtkWidget *stations_tree_view G_GNUC_UNUSED,
				GdkEvent *event G_GNUC_UNUSED,
				GvMainWindowStatusIcon *self)
{
	/* When the treeview is mapped, we need to check AGAIN if the natural
	 * height we have is correct.
	 */
	g_idle_add(G_SOURCE_FUNC(when_idle_resize_window), self);

	return GDK_EVENT_PROPAGATE;
}

/*
 * Public methods
 */

GvMainWindow *
gv_main_window_status_icon_new(GApplication *application)
{
	GvMainWindowStatusIcon *self;

	self = g_object_new(GV_TYPE_MAIN_WINDOW_STATUS_ICON,
			    "application", application,
			    NULL);

	return GV_MAIN_WINDOW(self);
}

/*
 * Signal handlers
 */

#define CLOSE_WINDOW_ON_FOCUS_OUT TRUE

static gboolean
on_focus_change(GtkWindow *window, GdkEventFocus *focus_event, gpointer data G_GNUC_UNUSED)
{
	g_assert(focus_event->type == GDK_FOCUS_CHANGE);

	DEBUG("Main window %s focus", focus_event->in ? "gained" : "lost");

	if (focus_event->in)
		return GDK_EVENT_PROPAGATE;

	if (CLOSE_WINDOW_ON_FOCUS_OUT == FALSE)
		return GDK_EVENT_PROPAGATE;

#if 0
	/* This code does not seem to be needed, as the call to
	 * gtk_window_close() below has no effect anyway when the
	 * context menu is on.
	 */
	GvMainWindowStatusIcon *self = GV_MAIN_WINDOW_STATUS_ICON(window);
	GvMainWindowStatusIconPrivate *priv = self->priv;
	GvStationsTreeView *stations_tree_view = GV_STATIONS_TREE_VIEW(priv->stations_tree_view);

	if (gv_stations_tree_view_has_context_menu(stations_tree_view))
		return GDK_EVENT_PROPAGATE;
#endif

	DEBUG("Closing window");
	gtk_window_close(window);

	return GDK_EVENT_PROPAGATE;
}

static gboolean
on_key_press_event(GtkWindow *window, GdkEventKey *event, gpointer data G_GNUC_UNUSED)
{
	g_assert(event->type == GDK_KEY_PRESS);

	/* When <Esc> is pressed, close the window */
	if (event->keyval == GDK_KEY_Escape)
		gtk_window_close(window);

	return GDK_EVENT_PROPAGATE;
}

static gboolean
on_window_delete_event(GtkWindow *self, GdkEvent *event G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	/* On delete event, we just close the window (ie. hide) */
	gtk_widget_hide(GTK_WIDGET(self));
	return GDK_EVENT_STOP;
}

/*
 * GObject methods
 */

static void
gv_main_window_status_icon_setup(GvMainWindowStatusIcon *self)
{
	GtkApplicationWindow *application_window = GTK_APPLICATION_WINDOW(self);
	GtkWindow *window = GTK_WINDOW(self);

	/* Basically, we want the window to appear and behave as a popup window */

	/* Hide the menu bar in the main window */
	gtk_application_window_set_show_menubar(application_window, FALSE);

	/* Window appearance */
	gtk_window_set_decorated(window, FALSE);
	gtk_window_set_position(window, GTK_WIN_POS_MOUSE);

	/* We don't want the window to appear in pager or taskbar.
	 * This has an undesired effect though: the window may not
	 * have the focus when it's shown by the window manager.
	 * But read on...
	 */
	gtk_window_set_skip_pager_hint(window, TRUE);
	gtk_window_set_skip_taskbar_hint(window, TRUE);

	/* Setting the window modal seems to ensure that the window
	 * receives focus when shown by the window manager.
	 */
	gtk_window_set_modal(window, TRUE);

	/* Handle keyboard focus changes, so that we can hide the
	 * window on 'focus-out-event'.
	 */
	g_signal_connect_object(window, "focus-in-event",
				G_CALLBACK(on_focus_change), NULL, 0);
	g_signal_connect_object(window, "focus-out-event",
				G_CALLBACK(on_focus_change), NULL, 0);

	/* Catch the <Esc> keystroke to close the window */
	g_signal_connect_object(window, "key-press-event",
				G_CALLBACK(on_key_press_event), NULL, 0);

	/* Don't quit when the window is closed, just hide */
	g_signal_connect_object(window, "delete-event",
				G_CALLBACK(on_window_delete_event), NULL, 0);
}

static void
gv_main_window_status_icon_setup_autosize(GvMainWindowStatusIcon *self)
{
	GvMainWindowStatusIconPrivate *priv = self->priv;
	GtkWidget *stations_tree_view;
	GtkWidget *go_next_button;
	GtkWidget *station_name_label;

	/* Get the stations tree view, and connect all the signal handlers so
	 * that we can resize the window vertically whenever the number of
	 * stations change. This is also what sets the initial vertical size
	 * of the window. */
	stations_tree_view = find_child(GTK_WIDGET(self), "stations_tree_view");
	g_assert(stations_tree_view != NULL);
	g_signal_connect_object(stations_tree_view, "populated",
				G_CALLBACK(on_stations_tree_view_populated), self, 0);
	g_signal_connect_object(stations_tree_view, "realize",
				G_CALLBACK(on_stations_tree_view_realize), self, 0);
	g_signal_connect_object(stations_tree_view, "map-event",
				G_CALLBACK(on_stations_tree_view_map_event), self, 0);
	priv->stations_tree_view = stations_tree_view;

	/* Get the next button and hide it, no station view in status icon mode,
	 * as it's too difficult too get a useful width, without impacting
	 * the width of the playlist view (tried to set width-request on map
	 * and unmap of the station-view, didn't work)
	 */
	go_next_button = find_child(GTK_WIDGET(self), "go_next_button");
	g_assert(go_next_button != NULL);
	gtk_widget_set_visible(go_next_button, FALSE);

	/* Get the station label, make sure it does not wrap. Let it define the
	 * minimun width for the window, if ever it's longer than the control bar.
	 */
	station_name_label = find_child(GTK_WIDGET(self), "station_name_label");
	g_assert(station_name_label != NULL);
	gtk_label_set_line_wrap(GTK_LABEL(station_name_label), FALSE);
	gtk_label_set_ellipsize(GTK_LABEL(station_name_label), PANGO_ELLIPSIZE_NONE);
}

static void
gv_main_window_status_icon_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window_status_icon, object);
}

static void
gv_main_window_status_icon_constructed(GObject *object)
{
	GvMainWindowStatusIcon *self = GV_MAIN_WINDOW_STATUS_ICON(object);

	TRACE("%p", self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window_status_icon, object);

	gv_main_window_status_icon_setup(self);
	gv_main_window_status_icon_setup_autosize(self);
}

static void
gv_main_window_status_icon_init(GvMainWindowStatusIcon *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_main_window_status_icon_get_instance_private(self);
}

static void
gv_main_window_status_icon_class_init(GvMainWindowStatusIconClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_main_window_status_icon_finalize;
	object_class->constructed = gv_main_window_status_icon_constructed;
}
