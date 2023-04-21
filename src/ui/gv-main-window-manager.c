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
#include "ui/gv-main-window.h"
#include "ui/gv-ui-internal.h"

#include "ui/gv-main-window-manager.h"

/*
 * Reasons to have a separate object to deal with the main window configuration,
 * aka size, position, maximized.
 *
 * Delayed method calls
 * --------------------
 *
 * We invoke methods on the main window in two contexts:
 * - at init time (to load configuration)
 * - in signal handlers (to save configuration)
 *
 * Both of these contexts are not suitable to query the window size, and because
 * of that we need to delay all the method calls. This logic is better taken out
 * of the main window, and handled here in the window manager.
 *
 * Window finalization
 * -------------------
 *
 * Saving the window configuration is an operation that is delayed by a timeout,
 * and therefore there's a possibility that the main window is finalized before
 * the timeout is reached. If this 'save timeout' is handled by the main window,
 * then we have to try to save the window configuration from within the finalize
 * method of the window itself, and it doesn't work. It's too late to query the
 * window at this moment.
 *
 * The window manager solves this problem elegantly, since the 'save timeout' is
 * taken out of the main window, and handled by the manager. The manager is sure
 * to be finalized before the main window, and therefore can safely query it at
 * this moment.
 */

#define SAVE_DELAY 1 // how long to wait before writing changes to disk

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_MAIN_WINDOW,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvMainWindowManagerPrivate {
	/* Properties */
	GvMainWindow *main_window;
	/* New values, waiting to be saved */
	gint new_x;
	gint new_y;
	gint new_width;
	gint new_height;
	/* Window configuration */
	guint save_configuration_timeout_id;
};

typedef struct _GvMainWindowManagerPrivate GvMainWindowManagerPrivate;

struct _GvMainWindowManager {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvMainWindowManagerPrivate *priv;
};

static void gv_main_window_manager_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GvMainWindowManager, gv_main_window_manager, G_TYPE_OBJECT,
			G_ADD_PRIVATE(GvMainWindowManager)
			G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
					      gv_main_window_manager_configurable_interface_init))

/*
 * Private methods
 */

static void
gv_main_window_manager_save_configuration_now(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;
	GSettings *settings = gv_ui_settings;
	gint width, height;
	gint x, y;

	TRACE("%p", self);

	/* Save size if changed */
	g_settings_get(settings, "window-size", "(ii)", &width, &height);
	if ((width != priv->new_width) || (height != priv->new_height))
		g_settings_set(settings, "window-size", "(ii)", priv->new_width, priv->new_height);
	priv->new_width = priv->new_height = 0;

	/* Save position if changed */
	g_settings_get(settings, "window-position", "(ii)", &x, &y);
	if ((x != priv->new_x) || (y != priv->new_y))
		g_settings_set(settings, "window-position", "(ii)", priv->new_x, priv->new_y);
	priv->new_x = priv->new_y = 0;
}

static gboolean
when_timeout_save_configuration(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	gv_main_window_manager_save_configuration_now(self);

	priv->save_configuration_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_manager_save_configuration_delayed(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	g_clear_handle_id(&priv->save_configuration_timeout_id, g_source_remove);
	priv->save_configuration_timeout_id =
		g_timeout_add_seconds(SAVE_DELAY, (GSourceFunc) when_timeout_save_configuration,
				      self);
}

/*
 * Window signal handlers
 */

static gboolean
on_main_window_configure_event(GtkWindow *window,
			       GdkEventConfigure *event,
			       GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	TRACE("%p, %d, %p", window, event->type, self);

	g_assert(event->type == GDK_CONFIGURE);

	/* Don't save anything when window is maximized */
	if (gtk_window_is_maximized(window))
		return GDK_EVENT_PROPAGATE;

	/* We can either use the values available in the GdkEventConfigure struct,
	 * or use GtkWindow getters. After a few tests (KDE/X11 and GNOME/Wayland),
	 * it turns out that the GdkEventConfigure values can't be used. For
	 * KDE/X11, the y value has an offset. For GNOME/Wayland, both x and y are
	 * zero, and both width and height are too big. So let's use GtkWindow
	 * getters.
	 */
	gtk_window_get_position(window, &priv->new_x, &priv->new_y);
	gtk_window_get_size(window, &priv->new_width, &priv->new_height);
	//DEBUG("GdkEventConfigure: x=%d, y=%d, widht=%d, height=%d",
	//		event->x, event->y, event->width, event->height);
	//DEBUG("GtkWindow getters: x=%d, y=%d, widht=%d, height=%d",
	//		priv->new_x, priv->new_y, priv->new_width, priv->new_height);

	/* Since this signal handler is invoked multiple times during resizing,
	 * we prefer to delay the actual save operation.
	 */
	gv_main_window_manager_save_configuration_delayed(self);

	return GDK_EVENT_PROPAGATE;
}

/*
 * Public functions
 */

GvMainWindowManager *
gv_main_window_manager_new(GvMainWindow *main_window)
{
	return g_object_new(GV_TYPE_MAIN_WINDOW_MANAGER,
			    "main-window", main_window,
			    NULL);
}

/*
 * Property accessors
 */

static void
gv_main_window_manager_set_main_window(GvMainWindowManager *self, GvMainWindow *main_window)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert(priv->main_window == NULL);
	g_assert(main_window != NULL);
	g_set_object(&priv->main_window, main_window);
}

static void
gv_main_window_manager_get_property(GObject *object,
				    guint property_id,
				    GValue *value,
				    GParamSpec *pspec)
{
	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_main_window_manager_set_property(GObject *object,
				    guint property_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_MAIN_WINDOW:
		gv_main_window_manager_set_main_window(self, g_value_get_object(value));
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
gv_main_window_manager_load_configuration(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;
	GSettings *settings = gv_ui_settings;
	gint width, height;
	gint x, y;

	TRACE("%p", self);

	/* Get settings */
	g_settings_get(settings, "window-size", "(ii)", &width, &height);
	g_settings_get(settings, "window-position", "(ii)", &x, &y);

	/* Set initial window size */
	if (width != -1 && height != -1) {
		GtkWindow *window = GTK_WINDOW(priv->main_window);

		DEBUG("Restoring window size (%d, %d)", width, height);
		gtk_window_resize(window, width, height);

	} else {
		GtkWindow *window = GTK_WINDOW(priv->main_window);
		gint win_height;

		/*
		 * Now is the tricky part. I wish to resize the main window to its
		 * 'natural size', so that it can accomodate the station tree view
		 * (whatever the number of stations), without showing the scroll bar.
		 * I don't think I'm wishing anything fancy here.
		 *
		 * However, it seems impossible to find a reliable way to do that.
		 *
		 * Querying the station tree view for its natural size RIGHT NOW is
		 * too early. Ok, that's fine to me GTK, so when is the right moment ?
		 *
		 * Well, the problem is that there doesn't seem to be a right moment to
		 * speak of. I tried delaying to the latest, that's to say connecting to
		 * the 'map-event' signal of the station tree view, then delaying with a
		 * 'g_idle_add()', then at last querying the tree view for its natural size.
		 * Even at this moment, it might fail and report a size that is too small.
		 * After that I have no other signal to connect to, I'm left with delaying
		 * with a timeout to query AGAIN the tree view for its natural size, and
		 * discover, o surprise, that it reports a different natural size.
		 *
		 * And as far as I know, there was no signal that I could have used to
		 * get notified of this change.
		 *
		 * So, screw it, no more autosize, it's way too much hassle for such a
		 * trivial result.
		 */

		gtk_window_get_size(window, NULL, &win_height);
		win_height += 240;

		DEBUG("Setting default window size (%d, %d)", 1, win_height);
		gtk_window_resize(window, 1, win_height);
	}

	/* Set initial window position */
	if (x != -1 || y != -1) {
		GtkWindow *window = GTK_WINDOW(priv->main_window);

		DEBUG("Restoring window position (%d, %d)", x, y);
		gtk_window_move(window, x, y);
	}

	/* Connect to the 'configure-event' signal to save changes when the
	 * window size or position is modified.
	 */
	g_signal_connect_object(priv->main_window, "configure-event",
				G_CALLBACK(on_main_window_configure_event),
				self, 0);
}

static void
gv_main_window_manager_configure(GvConfigurable *configurable)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(configurable);

	gv_main_window_manager_load_configuration(self);
}

static void
gv_main_window_manager_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_main_window_manager_configure;
}

/*
 * GObject methods
 */

static void
gv_main_window_manager_finalize(GObject *object)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);
	GvMainWindowManagerPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Run any pending save operation */
	if (priv->save_configuration_timeout_id > 0)
		when_timeout_save_configuration(self);

	/* Release resources */
	g_object_unref(priv->main_window);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window_manager, object);
}

static void
gv_main_window_manager_constructed(GObject *object)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);
	GvMainWindowManagerPrivate *priv = self->priv;

	TRACE("%p", self);

	/* Ensure construct-only properties have been set */
	g_assert(priv->main_window != NULL);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window_manager, object);
}

static void
gv_main_window_manager_init(GvMainWindowManager *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_main_window_manager_get_instance_private(self);
}

static void
gv_main_window_manager_class_init(GvMainWindowManagerClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_main_window_manager_finalize;
	object_class->constructed = gv_main_window_manager_constructed;

	/* Properties */
	object_class->get_property = gv_main_window_manager_get_property;
	object_class->set_property = gv_main_window_manager_set_property;

	properties[PROP_MAIN_WINDOW] =
		g_param_spec_object("main-window", "Main window", NULL,
				    GV_TYPE_MAIN_WINDOW,
				    GV_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
