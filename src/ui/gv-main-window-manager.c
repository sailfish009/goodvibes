/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
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

#include "framework/gv-framework.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-main-window.h"

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
 * - in signal handlers (to resize and save configuration)
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

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_MAIN_WINDOW,
	PROP_STATUS_ICON_MODE,
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
	gboolean      status_icon_mode;
	/* Resizing */
	gboolean autoresize_source_id;
	/* Window configuration */
	guint    save_window_configuration_source_id;
};

typedef struct _GvMainWindowManagerPrivate GvMainWindowManagerPrivate;

struct _GvMainWindowManager {
	/* Parent instance structure */
	GObject                     parent_instance;
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

static gboolean
when_idle_autoresize(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	gv_main_window_autoresize(priv->main_window);

	priv->autoresize_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_manager_autoresize_delayed(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	if (priv->autoresize_source_id > 0)
		g_source_remove(priv->autoresize_source_id);

	priv->autoresize_source_id = g_idle_add((GSourceFunc) when_idle_autoresize, self);
}

static gboolean
when_timeout_save_window_configuration(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	gv_main_window_save_configuration(priv->main_window);

	priv->save_window_configuration_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_manager_save_configuration_delayed(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	if (priv->save_window_configuration_source_id > 0)
		g_source_remove(priv->save_window_configuration_source_id);

	priv->save_window_configuration_source_id =
	        g_timeout_add_seconds(1, (GSourceFunc) when_timeout_save_window_configuration,
	                              self);
}

static gboolean
when_idle_load_window_configuration(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	gv_main_window_load_configuration(priv->main_window);

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_manager_load_configuration_delayed(GvMainWindowManager *self G_GNUC_UNUSED)
{
	/* This is called only once, no need to bother with the source id */
	g_idle_add((GSourceFunc) when_idle_load_window_configuration, self);
}

/*
 * Standalone window signal handlers
 */

static void
on_stations_tree_view_populated(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                                GvMainWindowManager *self)
{
	/* This signal handler is invoked when the station list was modified,
	 * and we want to resize the main window.
	 */
	gv_main_window_manager_autoresize_delayed(self);
}

static gboolean
on_main_window_configure_event(GtkWindow *window G_GNUC_UNUSED,
                               GdkEventConfigure *event G_GNUC_UNUSED,
                               GvMainWindowManager *self)
{
	/* This signal handler is invoked multiple times during resizing (really).
	 * It only means that resizing is in progress, and there is no way to know
	 * that resizing is finished.
	 * The (dumb) strategy to handle that is just to delay our reaction.
	 */
	gv_main_window_manager_save_configuration_delayed(self);

	return FALSE;
}

/*
 * Public functions
 */

GvMainWindowManager *
gv_main_window_manager_new(GvMainWindow *main_window, gboolean status_icon_mode)
{
	return g_object_new(GV_TYPE_MAIN_WINDOW_MANAGER,
	                    "main-window", main_window,
	                    "status-icon-mode", status_icon_mode,
	                    NULL);
}

/*
 * Property accessors
 */

static void
gv_main_window_manager_set_status_icon_mode(GvMainWindowManager *self, gboolean status_icon_mode)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_false(priv->status_icon_mode);
	priv->status_icon_mode = status_icon_mode;
}

static void
gv_main_window_manager_set_main_window(GvMainWindowManager *self, GvMainWindow *main_window)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_null(priv->main_window);
	g_assert_nonnull(main_window);
	g_set_object(&priv->main_window, main_window);
}

static void
gv_main_window_manager_get_property(GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
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
gv_main_window_manager_set_property(GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_MAIN_WINDOW:
		gv_main_window_manager_set_main_window(self, g_value_get_object(value));
		break;
	case PROP_STATUS_ICON_MODE:
		gv_main_window_manager_set_status_icon_mode(self, g_value_get_boolean(value));
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
gv_main_window_manager_configure(GvConfigurable *configurable)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(configurable);
	GvMainWindowManagerPrivate *priv = self->priv;

	TRACE("%p", self);

	/* Window settings are ignored in status icon mode */
	if (priv->status_icon_mode) {
		GtkWidget *stations_tree_view;
		/*
		 * The window height MUST BE handled automatically. The related settings
		 * from gsettings are ignored.
		 *
		 * The trick is that the natural height is not known right now, and will
		 * change each time a row is added or removed. It's a bit complicated to
		 * handle manually, and hopefully this code will go away one day.
		 */

		/* This should be called only once, the first time the window is displayed.
		 * Connecting to the 'realize' signal of the main window doesn't work, as
		 * it's too early to know the size of the stations tree view.
		 */

		/* This will be invoked each time there's a change in the tree view */
		stations_tree_view = gv_main_window_get_stations_tree_view(priv->main_window);
		g_signal_connect_object(stations_tree_view, "populated",
		                        G_CALLBACK(on_stations_tree_view_populated),
		                        self, 0);

	} else {
		/* Settings must be applied with a delay, as it's too early right now */
		gv_main_window_manager_load_configuration_delayed(self);

		/* Connect to the 'configure-event' signal to save window configuration */
		g_signal_connect_object(priv->main_window, "configure-event",
		                        G_CALLBACK(on_main_window_configure_event),
		                        self, 0);
	}
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
	if (priv->autoresize_source_id > 0)
		g_source_remove(priv->autoresize_source_id);

	if (priv->save_window_configuration_source_id > 0)
		when_timeout_save_window_configuration(self);

	/* Release resources */
	g_object_unref(priv->main_window);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window_manager, object);
}

static void
gv_main_window_manager_constructed(GObject *object)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);
	GvMainWindowManagerPrivate *priv =self->priv;

	TRACE("%p", self);

	/* Ensure construct-only properties have been set */
	g_assert_nonnull(priv->main_window);

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
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_CONSTRUCT_ONLY |
	                            G_PARAM_WRITABLE);

	properties[PROP_STATUS_ICON_MODE] =
	        g_param_spec_boolean("status-icon-mode", "Status icon mode", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_CONSTRUCT_ONLY |
	                             G_PARAM_WRITABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
