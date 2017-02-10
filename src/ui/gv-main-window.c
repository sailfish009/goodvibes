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

#include "additions/glib-object.h"
#include "additions/gtk.h"
#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-stations-tree-view.h"

#include "ui/gv-main-window.h"

#define UI_FILE "main-window.glade"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_POPUP,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvMainWindowPrivate {
	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *window_vbox;
	/* Current status */
	GtkWidget *station_label;
	GtkWidget *status_label;
	/* Button box */
	GtkWidget *play_button;
	GtkWidget *prev_button;
	GtkWidget *next_button;
	GtkWidget *repeat_toggle_button;
	GtkWidget *shuffle_toggle_button;
	GtkWidget *volume_button;
	/* Stations */
	GtkWidget *scrolled_window;
	GtkWidget *stations_tree_view;

	/*
	 * Bindings
	 */

	GBinding *volume_binding;

	/* 'popup' is true when the window is in status icon mode */
	gboolean popup;
	gboolean autoresize_source_id;
	/* Window configuration */
	guint    save_window_configuration_timeout_id;
	gboolean maximized;
	gint     width;
	gint     height;
	gint     x;
	gint     y;
};

typedef struct _GvMainWindowPrivate GvMainWindowPrivate;

struct _GvMainWindow {
	/* Parent instance structure */
	GtkApplicationWindow  parent_instance;
	/* Private data */
	GvMainWindowPrivate  *priv;
};

static void gv_main_window_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GvMainWindow, gv_main_window, GTK_TYPE_APPLICATION_WINDOW,
                        G_ADD_PRIVATE(GvMainWindow)
                        G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
                                        gv_main_window_configurable_interface_init))

/*
 * Private methods
 */

static void
gv_main_window_autoresize_now(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GtkAllocation allocated;
	GtkRequisition natural;
	gint width, height, diff, new_height;
	gint min_height = 1;
	gint max_height = 640;

	/*
	 * Here comes a hacky piece of code !
	 *
	 * Problem: from the moment the station tree view is within a scrolled
	 * window, the height is not handled smartly by GTK+ anymore. By default,
	 * it's ridiculously small. Then, when the number of rows in the tree view
	 * is changed, the tree view is not resized. So if we want a smart height,
	 * we have to do it manually.
	 *
	 * The success (or failure) of this function highly depends on the moment
	 * it's called.
	 * - too early, get_preferred_size() calls return junk.
	 * - in some signal handlers, get_preferred_size() calls return junk.
	 *
	 * Plus, we resize the window here, is the context safe to do that ?
	 *
	 * For these reasons, it's safer to never calle this function directly,
	 * but instead always delay the call for an idle moment.
	 */

	/* Determine if the tree view is under its natural height */
	gtk_widget_get_allocation(priv->stations_tree_view, &allocated);
	gtk_widget_get_preferred_size(priv->stations_tree_view, NULL, &natural);
	//DEBUG("allocated height: %d", allocated.height);
	//DEBUG("natural height: %d", natural.height);
	diff = natural.height - allocated.height;

	/* Determine what should be the new height */
	gtk_window_get_size(GTK_WINDOW(self), &width, &height);
	new_height = height + diff;
	if (new_height < 1) {
		DEBUG("Clamping new height %d to minimum height %d",
		      new_height, min_height);
		new_height = min_height;
	}
	if (new_height > max_height) {
		DEBUG("Clamping new height %d to maximum height %d",
		      new_height, max_height);
		new_height = max_height;
	}

	/* Resize the main window */
	DEBUG("Resizing to new height %d", new_height);
	gtk_window_resize(GTK_WINDOW(self), width, new_height);
}

static gboolean
when_idle_autoresize(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;

	gv_main_window_autoresize_now(self);

	priv->autoresize_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_autoresize_delayed(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;

	if (priv->autoresize_source_id > 0)
		g_source_remove(priv->autoresize_source_id);

	priv->autoresize_source_id = g_idle_add((GSourceFunc) when_idle_autoresize, self);
}

static void
gv_main_window_save_configuration(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GSettings *settings = gv_ui_settings;
	gint old_width, old_height;
	gint old_x, old_y;

	TRACE("%p", self);

	/* This function might be invoked during finalization, and therefore it's
	 * too late to invoke any method on the GtkWindow. That's precisely why
	 * we bother to mirror some variables locally.
	 */

	/* Don't save anything when window is maximized */
	if (priv->maximized)
		return;

	/* Save size if changed */
	g_settings_get(settings, "window-size", "(ii)", &old_width, &old_height);
	if ((priv->width != old_width) || (priv->height != old_height))
		g_settings_set(settings, "window-size", "(ii)", priv->width, priv->height);

	/* Save position if changed */
	g_settings_get(settings, "window-position", "(ii)", &old_x, &old_y);
	if ((priv->x != old_x) || (priv->y != old_y))
		g_settings_set(settings, "window-position", "(ii)", priv->x, priv->y);
}

/*
 * Core Player signal handlers
 */

static void
set_station_label(GtkLabel *label, GvStation *station)
{
	const gchar *station_title;

	if (station)
		station_title = gv_station_get_name_or_uri(station);
	else
		station_title = _("No station selected");

	gtk_label_set_text(label, station_title);
}

static void
set_status_label(GtkLabel *label, GvPlayerState state, GvMetadata *metadata)
{
	if (state != GV_PLAYER_STATE_PLAYING || metadata == NULL) {
		const gchar *state_str;

		switch (state) {
		case GV_PLAYER_STATE_PLAYING:
			state_str = _("Playing");
			break;
		case GV_PLAYER_STATE_CONNECTING:
			state_str = _("Connecting...");
			break;
		case GV_PLAYER_STATE_BUFFERING:
			state_str = _("Buffering...");
			break;
		case GV_PLAYER_STATE_STOPPED:
		default:
			state_str = _("Stopped");
			break;
		}

		gtk_label_set_text(label, state_str);
	} else {
		gchar *artist_title;
		gchar *album_year;
		gchar *str;

		artist_title = gv_metadata_make_title_artist(metadata, FALSE);
		album_year = gv_metadata_make_album_year(metadata, FALSE);

		if (artist_title && album_year)
			str = g_strdup_printf("%s/n%s", artist_title, album_year);
		else if (artist_title)
			str = g_strdup(artist_title);
		else
			str = g_strdup(_("Playing"));

		gtk_label_set_text(label, str);

		g_free(str);
		g_free(album_year);
		g_free(artist_title);
	}
}

static void
set_play_button(GtkButton *button, GvPlayerState state)
{
	GtkWidget *image;
	const gchar *icon_name;

	if (state == GV_PLAYER_STATE_STOPPED)
		icon_name = "media-playback-start-symbolic";
	else
		icon_name = "media-playback-stop-symbolic";

	image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(button, image);
}

static void
set_volume_button(GtkVolumeButton *volume_button, guint volume, gboolean mute)
{
	GtkScaleButton *scale_button = GTK_SCALE_BUTTON(volume_button);

	if (mute)
		gtk_scale_button_set_value(scale_button, 0);
	else
		gtk_scale_button_set_value(scale_button, volume);
}

static void
on_player_notify(GvPlayer     *player,
                 GParamSpec    *pspec,
                 GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", player, property_name, self);

	if (!g_strcmp0(property_name, "station")) {
		GtkLabel *label = GTK_LABEL(priv->station_label);
		GvStation *station = gv_player_get_station(player);

		set_station_label(label, station);

	} else if (!g_strcmp0(property_name, "state")) {
		GtkLabel *label = GTK_LABEL(priv->status_label);
		GtkButton *button = GTK_BUTTON(priv->play_button);
		GvPlayerState state = gv_player_get_state(player);
		GvMetadata *metadata = gv_player_get_metadata(player);

		set_status_label(label, state, metadata);
		set_play_button(button, state);

	} else if (!g_strcmp0(property_name, "metadata")) {
		GtkLabel *label = GTK_LABEL(priv->status_label);
		GvPlayerState state = gv_player_get_state(player);
		GvMetadata *metadata = gv_player_get_metadata(player);

		set_status_label(label, state, metadata);

	}  else if (!g_strcmp0(property_name, "mute")) {
		GtkVolumeButton *volume_button = GTK_VOLUME_BUTTON(priv->volume_button);
		guint volume = gv_player_get_volume(player);
		gboolean mute = gv_player_get_mute(player);

		g_binding_unbind(priv->volume_binding);
		set_volume_button(volume_button, volume, mute);
		priv->volume_binding = g_object_bind_property
		                       (player, "volume",
		                        volume_button, "value",
		                        G_BINDING_BIDIRECTIONAL);
	}
}

/*
 * Widget signal handlers
 */

static void
on_button_clicked(GtkButton *button, GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	GtkWidget *widget = GTK_WIDGET(button);

	if (widget == priv->play_button) {
		gv_player_toggle(player);
	} else if (widget == priv->prev_button) {
		gv_player_prev(player);
	} else if (widget == priv->next_button) {
		gv_player_next(player);
	} else {
		CRITICAL("Unhandled button %p", button);
	}
}

/*
 * Stations tree view signal handlers for popup window
 */

static void
on_stations_tree_view_populated(GvStationsTreeView *tree_view G_GNUC_UNUSED,
                                GvMainWindow *self)
{
	gv_main_window_autoresize_delayed(self);
}

static void
on_stations_tree_view_realize(GtkWidget *widget G_GNUC_UNUSED,
                              GvMainWindow *self)
{
	gv_main_window_autoresize_delayed(self);
}

/*
 * Popup window signal handlers
 */

#define POPUP_WINDOW_CLOSE_ON_FOCUS_OUT TRUE

static gboolean
on_popup_window_focus_change(GtkWindow     *window,
                             GdkEventFocus *focus_event,
                             gpointer       data G_GNUC_UNUSED)
{
	GvMainWindow *self = GV_MAIN_WINDOW(window);
	GvMainWindowPrivate *priv = self->priv;
	GvStationsTreeView *stations_tree_view = GV_STATIONS_TREE_VIEW(priv->stations_tree_view);

	g_assert(focus_event->type == GDK_FOCUS_CHANGE);

	//DEBUG("Main window %s focus", focus_event->in ? "gained" : "lost");

	if (focus_event->in)
		return FALSE;

	if (!POPUP_WINDOW_CLOSE_ON_FOCUS_OUT)
		return FALSE;

	if (gv_stations_tree_view_has_context_menu(stations_tree_view))
		return FALSE;

	gtk_window_close(window);

	return FALSE;
}

static gboolean
on_popup_window_key_press_event(GtkWindow   *window G_GNUC_UNUSED,
                                GdkEventKey *event,
                                gpointer     data G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;

	g_assert(event->type == GDK_KEY_PRESS);

	switch (event->keyval) {
	case GDK_KEY_Escape:
		/* Close window if 'Esc' key is pressed */
		gtk_window_close(window);
		break;

	case GDK_KEY_blank:
		/* Play/Stop when space is pressed */
		gv_player_toggle(player);
		break;

	default:
		break;
	}

	return FALSE;
}


/*
 * Standalone window signal handlers
 */

static gboolean
when_save_window_configuration_timeout(gpointer data)
{
	GvMainWindow *self = GV_MAIN_WINDOW(data);
	GvMainWindowPrivate *priv = self->priv;

	gv_main_window_save_configuration(self);

	priv->save_window_configuration_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static gboolean
on_standalone_window_configure_event(GtkWindow *window,
                                     GdkEventConfigure *event G_GNUC_UNUSED,
                                     gpointer user_data G_GNUC_UNUSED)
{
	GvMainWindow *self = GV_MAIN_WINDOW(window);
	GvMainWindowPrivate *priv = self->priv;

	/* This is invoked multiple times during resizing, and we want to act only
	 * when the user is done resizing. Therefore, we need a delay. However, the
	 * new window configuration must be saved locally NOW ! The reason is that
	 * the delayed function might be invoked during object finalization, and at
	 * this moment it is too query the window for these values. So we store the
	 * values locally now, and we delay the storage in gsettings for later.
	 */

	/* Save window configuration */
	priv->maximized = gtk_window_is_maximized(window);
	gtk_window_get_size(window, &priv->width, &priv->height);
	gtk_window_get_position(window, &priv->x, &priv->y);

	/* Delay GSettings storage */
	if (priv->save_window_configuration_timeout_id > 0)
		g_source_remove(priv->save_window_configuration_timeout_id);

	priv->save_window_configuration_timeout_id =
	        g_timeout_add_seconds(1, when_save_window_configuration_timeout, self);

	return FALSE;
}

static gboolean
on_standalone_window_delete_event(GtkWindow *window,
                                  GdkEvent  *event G_GNUC_UNUSED,
                                  gpointer   data G_GNUC_UNUSED)
{
	/* Hide the window immediately */
	gtk_widget_hide(GTK_WIDGET(window));

	/* Now we're about to quit the application */
	gv_core_quit();

	/* We must return TRUE here to prevent the window from being destroyed.
	 * The window will be destroyed later on during the cleanup process.
	 */
	return TRUE;
}

static gboolean
on_standalone_window_key_press_event(GtkWindow   *window G_GNUC_UNUSED,
                                     GdkEventKey *event,
                                     gpointer     data G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;

	g_assert(event->type == GDK_KEY_PRESS);

	switch (event->keyval) {
	case GDK_KEY_Escape:
		/* Iconify window if 'Esc' key is pressed */
		gtk_window_iconify(window);
		break;

	case GDK_KEY_blank:
		/* Play/Stop when space is pressed */
		gv_player_toggle(player);
		break;

	default:
		break;
	}

	return FALSE;
}

/*
 * Construct private methods
 */

static void
setup_adjustment(GtkAdjustment *adjustment, GObject *obj, const gchar *obj_prop)
{
	guint minimum, maximum;
	guint range;

	/* Get property bounds, and assign it to the adjustment */
	g_object_get_property_uint_bounds(obj, obj_prop, &minimum, &maximum);
	range = maximum - minimum;

	gtk_adjustment_set_lower(adjustment, minimum);
	gtk_adjustment_set_upper(adjustment, maximum);
	gtk_adjustment_set_step_increment(adjustment, range / 100);
	gtk_adjustment_set_page_increment(adjustment, range / 10);
}

static void
setup_action(GtkWidget *widget, const gchar *widget_signal,
             GCallback callback, GObject *object)
{
	/* Signal handler */
	g_signal_connect_object(widget, widget_signal, callback, object, 0);
}

static void
setup_setting(GtkWidget *widget, const gchar *widget_prop,
              GObject *obj, const gchar *obj_prop)
{
	/* Binding: obj 'prop' <-> widget 'prop'
	 * Order matters, don't mix up source and target here...
	 */
	g_object_bind_property(obj, obj_prop, widget, widget_prop,
	                       G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

/*
 * Public methods
 */

GtkWidget *
gv_main_window_new(GApplication *application, gboolean popup)
{
	return g_object_new(GV_TYPE_MAIN_WINDOW,
	                    "application", application,
	                    "popup", popup,
	                    NULL);
}

/*
 * Property accessors
 */

static void
gv_main_window_set_popup(GvMainWindow *self, gboolean popup)
{
	GvMainWindowPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_false(priv->popup);
	priv->popup = popup;
}

static void
gv_main_window_get_property(GObject    *object,
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
gv_main_window_set_property(GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_POPUP:
		gv_main_window_set_popup(self, g_value_get_boolean(value));
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
gv_main_window_configure(GvConfigurable *configurable)
{
	GvMainWindow *self = GV_MAIN_WINDOW(configurable);
	GvMainWindowPrivate *priv = self->priv;
	gint width, height;
	gint x, y;

	TRACE("%p", self);

	/* Window settings are ignored in popup mode */
	if (priv->popup)
		return;

	/* Window size */
	g_settings_get(gv_ui_settings, "window-size", "(ii)", &width, &height);
	DEBUG("Restoring window size (%d, %d)", width, height);
	gtk_window_set_default_size(GTK_WINDOW(self), width, height);
	//gtk_window_resize(GTK_WINDOW(self), width, height);

	/* Window position */
	g_settings_get(gv_ui_settings, "window-position", "(ii)", &x, &y);
	DEBUG("Restoring window position (%d, %d)", x, y);
	gtk_window_move(GTK_WINDOW(self), x, y);

	/* Connect to 'configure-event' signal */
	g_signal_connect_object(self, "configure-event",
	                        G_CALLBACK(on_standalone_window_configure_event),
	                        NULL, 0);
}

static void
gv_main_window_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_main_window_configure;
}

/*
 * Construct helpers
 */

static void
gv_main_window_populate_widgets(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GtkBuilder *builder;
	gchar *uifile;

	/* Build the ui */
	gv_builder_load(UI_FILE, &builder, &uifile);
	DEBUG("Built from ui file '%s'", uifile);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_vbox);

	/* Current status */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, status_label);

	/* Button box */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, play_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, prev_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, next_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, repeat_toggle_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, shuffle_toggle_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, volume_button);

	/* Stations tree view */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, scrolled_window);

	/* Now create the stations tree view */
	priv->stations_tree_view = gv_stations_tree_view_new();
	gtk_widget_show_all(priv->stations_tree_view);

	/* Add to scrolled window */
	gtk_container_add(GTK_CONTAINER(priv->scrolled_window), priv->stations_tree_view);

	/* Add vbox to the window */
	gtk_container_add(GTK_CONTAINER(self), priv->window_vbox);

	/* Cleanup */
	g_object_unref(G_OBJECT(builder));
	g_free(uifile);
}

static void
gv_main_window_setup_widgets(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GObject *player_obj = G_OBJECT(gv_core_player);

	/* Setup adjustments - must be done first, before setting widget values */
	setup_adjustment(gtk_scale_button_get_adjustment(GTK_SCALE_BUTTON(priv->volume_button)),
	                 player_obj, "volume");

	/*
	 * Setup settings and actions.
	 * These function calls create the link between the widgets and the program.
	 * - settings: the link is a binding between the gtk widget and an internal object.
	 * - actions : the link is a callback invoked when the widget is clicked. This is a
	 * one-way link from the ui to the application, and additional work would be needed
	 * if the widget was to react to external changes.
	 */
	setup_action(priv->play_button, "clicked",
	             G_CALLBACK(on_button_clicked), G_OBJECT(self));

	setup_action(priv->prev_button, "clicked",
	             G_CALLBACK(on_button_clicked), G_OBJECT(self));

	setup_action(priv->next_button, "clicked",
	             G_CALLBACK(on_button_clicked), G_OBJECT(self));

	setup_setting(priv->repeat_toggle_button, "active",
	              player_obj, "repeat");

	setup_setting(priv->shuffle_toggle_button, "active",
	              player_obj, "shuffle");

	/* Volume button comes with automatic tooltip, so we just need to bind.
	 * Plus, we need to remember the binding because we do strange things with it.
	 */
	priv->volume_binding = g_object_bind_property
	                       (player_obj, "volume",
	                        priv->volume_button, "value",
	                        G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
gv_main_window_setup_layout(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;

	g_object_set(priv->window_vbox,
	             "spacing", GV_UI_ELEM_SPACING,
	             NULL);
}

static void
gv_main_window_configure_for_popup(GvMainWindow *self)
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

	/* We want the window to be hidden instead of destroyed when closed */
	g_signal_connect_object(window, "delete-event",
	                        G_CALLBACK(gtk_widget_hide_on_delete), NULL, 0);

	/* Handle some keys */
	g_signal_connect_object(window, "key-press-event",
	                        G_CALLBACK(on_popup_window_key_press_event), NULL, 0);

	/* Handle keyboard focus changes, so that we can hide the
	 * window on 'focus-out-event'.
	 */
	g_signal_connect_object(window, "focus-in-event",
	                        G_CALLBACK(on_popup_window_focus_change), NULL, 0);
	g_signal_connect_object(window, "focus-out-event",
	                        G_CALLBACK(on_popup_window_focus_change), NULL, 0);

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
	g_signal_connect_object(self->priv->stations_tree_view, "realize",
	                        G_CALLBACK(on_stations_tree_view_realize),
	                        self, 0);

	/* This will be invoked each time there's a change in the tree view */
	g_signal_connect_object(self->priv->stations_tree_view, "populated",
	                        G_CALLBACK(on_stations_tree_view_populated),
	                        self, 0);
}

static void
gv_main_window_configure_for_standalone(GvMainWindow *self)
{
	GtkWindow *window = GTK_WINDOW(self);

	/* We want to quit the application when the window is closed */
	g_signal_connect_object(window, "delete-event",
	                        G_CALLBACK(on_standalone_window_delete_event), NULL, 0);

	/* Handle some keys */
	g_signal_connect_object(window, "key-press-event",
	                        G_CALLBACK(on_standalone_window_key_press_event), NULL, 0);
}

/*
 * GObject methods
 */

static void
gv_main_window_finalize(GObject *object)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);
	GvMainWindowPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Run any pending save operation */
	if (priv->autoresize_source_id > 0)
		g_source_remove(priv->autoresize_source_id);

	if (priv->save_window_configuration_timeout_id > 0)
		when_save_window_configuration_timeout(self);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window, object);
}

static void
gv_main_window_constructed(GObject *object)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);
	GvMainWindowPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	/* Build window */
	gv_main_window_populate_widgets(self);
	gv_main_window_setup_widgets(self);
	gv_main_window_setup_layout(self);

	/* Configure depending on the window mode */
	if (priv->popup) {
		DEBUG("Configuring main window for popup mode");
		gv_main_window_configure_for_popup(self);
	} else {
		DEBUG("Configuring main window for standalone mode");
		gv_main_window_configure_for_standalone(self);
	}

	/* Connect core signal handlers */
	g_signal_connect_object(player, "notify",
	                        G_CALLBACK(on_player_notify), self, 0);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window, object);
}

static void
gv_main_window_init(GvMainWindow *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_main_window_get_instance_private(self);
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

	properties[PROP_POPUP] =
	        g_param_spec_boolean("popup", "Popup", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_CONSTRUCT_ONLY |
	                             G_PARAM_WRITABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
