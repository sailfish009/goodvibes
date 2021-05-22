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
#include "ui/gv-ui-helpers.h"

#include "ui/gv-main-window.h"
#include "ui/gv-main-window-status-icon.h"

/*
 * GObject definitions
 */

struct _GvMainWindowStatusIconPrivate {
	gint natural_height;
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
 * Popup window signal handlers
 */

#define POPUP_WINDOW_CLOSE_ON_FOCUS_OUT TRUE

static gboolean
on_focus_change(GtkWindow *window, GdkEventFocus *focus_event, gpointer data G_GNUC_UNUSED)
{
	g_assert(focus_event->type == GDK_FOCUS_CHANGE);

	DEBUG("Main window %s focus", focus_event->in ? "gained" : "lost");

	if (focus_event->in)
		return GDK_EVENT_PROPAGATE;

	if (POPUP_WINDOW_CLOSE_ON_FOCUS_OUT == FALSE)
		return GDK_EVENT_PROPAGATE;

	// TODO this code might not be needed anyway
#if 0
	GvMainWindow *self = GV_MAIN_WINDOW(window);
	GvMainWindowPrivate *priv = self->priv;
	GvStationsTreeView *stations_tree_view = GV_STATIONS_TREE_VIEW(priv->stations_tree_view);

	if (gv_stations_tree_view_has_context_menu(stations_tree_view))
		return GDK_EVENT_PROPAGATE;
#endif

	gtk_window_close(window);

	return GDK_EVENT_PROPAGATE;
}

static gboolean
on_key_press_event(GtkWindow *window, GdkEventKey *event, gpointer data G_GNUC_UNUSED)
{
	g_assert(event->type == GDK_KEY_PRESS);

	if (event->keyval == GDK_KEY_Escape)
		gtk_window_close(window);

	return GDK_EVENT_PROPAGATE;
}

static gboolean
on_window_delete_event(GtkWindow *self, GdkEvent *event G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	/* We just close the window (ie. hide) */
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

	/* Connect main window signal handlers */
	g_signal_connect_object(window, "delete-event",
	                        G_CALLBACK(on_window_delete_event), NULL, 0);
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
