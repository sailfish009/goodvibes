/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2020 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#include "framework/glib-object-additions.h"
#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gtk-additions.h"
#include "ui/gv-ui-enum-types.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-stations-tree-view.h"

#include "ui/gv-main-window.h"

#define UI_RESOURCE_PATH GV_APPLICATION_PATH "/Ui/main-window.glade"

/*
 * Properties
 */

#define DEFAULT_CLOSE_ACTION  GV_MAIN_WINDOW_CLOSE_QUIT
#define DEFAULT_THEME_VARIANT GV_MAIN_WINDOW_THEME_DEFAULT

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_STATUS_ICON_MODE,
	PROP_PRIMARY_MENU,
	PROP_NATURAL_HEIGHT,
	PROP_CLOSE_ACTION,
	PROP_THEME_VARIANT,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvMainWindowPrivate {
	/*
	 * Properties
	 */

	gboolean status_icon_mode;
	GMenuModel *primary_menu;
	gint     natural_height;
	GvMainWindowCloseAction  close_action;
	GvMainWindowThemeVariant theme_variant;

	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *window_vbox;
	/* Current status */
	GtkWidget *info_vbox;
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
	 * Internal
	 */

	GtkWidget *info_tooltip_grid;
	GBinding  *volume_binding;
	gboolean   system_prefer_dark_theme;
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

/*
 * Info custom tooltip
 */

static void
grid_add_section(GtkGrid *grid, gint row, const gchar *section)
{
	GtkWidget *label;
	PangoAttrList *attrs;
	PangoAttribute *bold;

	attrs = pango_attr_list_new();
	bold = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(attrs, bold);

	label = gtk_label_new(section);
	gtk_label_set_attributes(GTK_LABEL(label), attrs);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(grid, label, 0, row, 2, 1);

	pango_attr_list_unref(attrs);
}

static void
grid_add_field(GtkGrid *grid, gint row, gboolean mandatory,
               const gchar *key, const gchar *value)
{
	GtkWidget *label;

	if (value == NULL && !mandatory)
		return;

	if (key) {
		label = gtk_label_new(key);
		gtk_widget_set_halign(label, GTK_ALIGN_END);
		gtk_grid_attach(grid, label, 0, row, 1, 1);
	}

	if (value) {
		label = gtk_label_new(value);
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(grid, label, 1, row, 1, 1);
	}
}

static GtkWidget *
make_info_tooltip_grid(GvStation *station, GvMetadata *metadata, guint bitrate)
{
	GtkGrid *grid;
	guint n;

	grid = GTK_GRID(gtk_grid_new());
	g_object_set(grid,
	             "row-spacing", 2,
	             "column-spacing", 6,
	             NULL);

	n = 0;

	if (station) {
		const gchar *name = gv_station_get_name(station);
		const gchar *uri = gv_station_get_uri(station);
		const gchar *user_agent = gv_station_get_user_agent(station);
		guint nominal_bitrate = gv_station_get_nominal_bitrate(station);
		GSList *stream_uris = gv_station_get_stream_uris(station);
		GSList *item;

		grid_add_section(grid, n++, _("Station Information"));

		grid_add_field(grid, n++, TRUE, _("Name"), name);
		grid_add_field(grid, n++, TRUE, _("URI"), uri);

		for (item = stream_uris; item; item = item->next) {
			const gchar *stream_uri = item->data;

			if (!g_strcmp0(stream_uri, uri))
				continue;

			grid_add_field(grid, n++, FALSE, "-", stream_uri);
		}

		grid_add_field(grid, n++, FALSE, _("User-agent"), user_agent);

		if (bitrate != 0 || nominal_bitrate != 0) {
			gchar *str = g_strdup_printf("%u kb/s (real: %u kb/s)", nominal_bitrate, bitrate);
			grid_add_field(grid, n++, FALSE, _("Bitrate"), str);
			g_free(str);
		}
	}

	if (metadata) {
		const gchar *artist  = gv_metadata_get_artist(metadata);
		const gchar *title   = gv_metadata_get_title(metadata);
		const gchar *album   = gv_metadata_get_album(metadata);
		const gchar *genre   = gv_metadata_get_genre(metadata);
		const gchar *year    = gv_metadata_get_year(metadata);
		const gchar *comment = gv_metadata_get_comment(metadata);

		grid_add_section(grid, n++, _("Metadata"));

		grid_add_field(grid, n++, FALSE, _("Artist"), artist);
		grid_add_field(grid, n++, FALSE, _("Title"), title);
		grid_add_field(grid, n++, FALSE, _("Album"), album);
		grid_add_field(grid, n++, FALSE, _("Genre"), genre);
		grid_add_field(grid, n++, FALSE, _("Year"), year);
		grid_add_field(grid, n++, FALSE, _("Comment"), comment);
	}

	gtk_widget_show_all(GTK_WIDGET(grid));

	return GTK_WIDGET(grid);
}

static gboolean
on_info_vbox_query_tooltip(GtkWidget    *widget G_GNUC_UNUSED,
                           gint          x G_GNUC_UNUSED,
                           gint          y G_GNUC_UNUSED,
                           gboolean      keyboard_tip G_GNUC_UNUSED,
                           GtkTooltip   *tooltip,
                           GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;

	/* We must take ownership of the grid, otherwise it gets destroyed
	 * as soon as the tooltip window vanishes (aka is finalized).
	 */
	if (priv->info_tooltip_grid == NULL) {
		GvPlayer *player = gv_core_player;
		GvStation *station = gv_player_get_station(player);
		GvMetadata *metadata = gv_player_get_metadata(player);
		guint bitrate = gv_player_get_bitrate(player);

		priv->info_tooltip_grid = make_info_tooltip_grid(station, metadata, bitrate);
		g_object_ref_sink(priv->info_tooltip_grid);
	}

	/* Set the custom widget */
	gtk_tooltip_set_custom(tooltip, priv->info_tooltip_grid);

	/* If the grid is empty, there's nothing to display */
	if (gtk_grid_get_child_at(GTK_GRID(priv->info_tooltip_grid), 0, 0) == NULL)
		return FALSE;

	return TRUE;
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
			state_str = _("Connecting…");
			break;
		case GV_PLAYER_STATE_BUFFERING:
			state_str = _("Buffering…");
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
		g_clear_object(&priv->info_tooltip_grid);

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
		g_clear_object(&priv->info_tooltip_grid);

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
 * Stations tree view signal handlers
 */

static gboolean
when_idle_compute_natural_height(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
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

/*
 * Main window signal handlers (both popup and standalone)
 */

static gboolean
on_window_delete_event(GvMainWindow *self,
		       GdkEvent     *event G_GNUC_UNUSED,
		       gpointer      data G_GNUC_UNUSED)
{
	GvMainWindowPrivate *priv = self->priv;
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

static gboolean
on_window_key_press_event(GvMainWindow *self,
                          GdkEventKey  *event,
                          gpointer      data G_GNUC_UNUSED)
{
	GvMainWindowPrivate *priv = self->priv;

	g_assert(event->type == GDK_KEY_PRESS);

	/* Close window if <Esc> is pressed, only in status icon mode */
	if ((event->keyval == GDK_KEY_Escape) && (priv->status_icon_mode == TRUE))
		gtk_window_close(GTK_WINDOW(self));

	return GDK_EVENT_PROPAGATE;
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
		return GDK_EVENT_PROPAGATE;

	if (!POPUP_WINDOW_CLOSE_ON_FOCUS_OUT)
		return GDK_EVENT_PROPAGATE;

	if (gv_stations_tree_view_has_context_menu(stations_tree_view))
		return GDK_EVENT_PROPAGATE;

	gtk_window_close(window);

	return GDK_EVENT_PROPAGATE;
}

/*
 * Construct helpers
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

void
gv_main_window_play_stop(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	gtk_widget_grab_focus(priv->play_button);
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

GvMainWindow *
gv_main_window_new(GApplication *application, GMenuModel *primary_menu,
		   gboolean status_icon_mode)
{
	return g_object_new(GV_TYPE_MAIN_WINDOW,
	                    "application", application,
			    "primary-menu", primary_menu,
	                    "status-icon-mode", status_icon_mode,
	                    NULL);
}

/*
 * Property accessors
 */

static void
gv_main_window_set_status_icon_mode(GvMainWindow *self, gboolean status_icon_mode)
{
	GvMainWindowPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_false(priv->status_icon_mode);
	priv->status_icon_mode = status_icon_mode;
}

static void
gv_main_window_set_primary_menu(GvMainWindow *self, GMenuModel *primary_menu)
{
	GvMainWindowPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_null(priv->primary_menu);
	priv->primary_menu = g_object_ref_sink(primary_menu);
}

GMenuModel *
gv_main_window_get_primary_menu(GvMainWindow *self)
{
	return self->priv->primary_menu;
}

gint
gv_main_window_get_natural_height(GvMainWindow *self)
{
	return self->priv->natural_height;
}

GvMainWindowCloseAction
gv_main_window_get_close_action(GvMainWindow *self)
{
	return self->priv->close_action;
}

void
gv_main_window_set_close_action(GvMainWindow *self, GvMainWindowCloseAction action)
{
	GvMainWindowPrivate *priv = self->priv;

	/* In status icon mode, this property is forced to CLOSE */
	if (priv->status_icon_mode) {
		DEBUG("Status icon mode: 'close-action' forced to CLOSE");
		action = GV_MAIN_WINDOW_CLOSE_CLOSE;
	}

	if (priv->close_action == action)
		return;

	priv->close_action = action;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_CLOSE_ACTION]);
}

GvMainWindowThemeVariant
gv_main_window_get_theme_variant(GvMainWindow *self)
{
	return self->priv->theme_variant;
}

void
gv_main_window_set_theme_variant(GvMainWindow *self, GvMainWindowThemeVariant variant)
{
	GvMainWindowPrivate *priv = self->priv;
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
	case PROP_PRIMARY_MENU:
		g_value_set_object(value, gv_main_window_get_primary_menu(self));
		break;
	case PROP_NATURAL_HEIGHT:
		g_value_set_int(value, gv_main_window_get_natural_height(self));
		break;
	case PROP_CLOSE_ACTION:
		g_value_set_enum(value, gv_main_window_get_close_action(self));
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
	case PROP_STATUS_ICON_MODE:
		gv_main_window_set_status_icon_mode(self, g_value_get_boolean(value));
		break;
	case PROP_PRIMARY_MENU:
		gv_main_window_set_primary_menu(self, g_value_get_object(value));
		break;
	case PROP_CLOSE_ACTION:
		gv_main_window_set_close_action(self, g_value_get_enum(value));
		break;
	case PROP_THEME_VARIANT:
		gv_main_window_set_theme_variant(self, g_value_get_enum(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Construct helpers
 */

static void
gv_main_window_populate_widgets(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GtkBuilder *builder;

	/* Build the ui */
	builder = gtk_builder_new_from_resource(UI_RESOURCE_PATH);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_vbox);

	/* Current status */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, info_vbox);
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
	g_object_unref(builder);
}

static void
gv_main_window_setup_widgets(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GObject *player_obj = G_OBJECT(gv_core_player);

	/* Setup adjustments - must be done first, before setting widget values */
	setup_adjustment(gtk_scale_button_get_adjustment(GTK_SCALE_BUTTON(priv->volume_button)),
	                 player_obj, "volume");

	/* Setup funky label for info vbox */
	gtk_widget_set_has_tooltip(priv->info_vbox, TRUE);
	g_signal_connect_object(priv->info_vbox, "query-tooltip",
	                        G_CALLBACK(on_info_vbox_query_tooltip), self, 0);

	/* Watch stations tree view */
	g_signal_connect_object(priv->stations_tree_view, "populated",
	                        G_CALLBACK(on_stations_tree_view_populated),
	                        self, 0);
	g_signal_connect_object(priv->stations_tree_view, "realize",
	                        G_CALLBACK(on_stations_tree_view_realize),
	                        self, 0);
	g_signal_connect_object(priv->stations_tree_view, "map-event",
	                        G_CALLBACK(on_stations_tree_view_map_event),
	                        self, 0);

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
gv_main_window_setup_appearance(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;

	g_object_set(priv->window_vbox,
	             "spacing", GV_UI_ELEM_SPACING,
	             NULL);
}

static void
gv_main_window_setup_for_popup(GvMainWindow *self)
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

	/* On close, we only want to close the window, instead of quitting */
	gv_main_window_set_close_action(self, GV_MAIN_WINDOW_CLOSE_CLOSE);

	/* Handle keyboard focus changes, so that we can hide the
	 * window on 'focus-out-event'.
	 */
	g_signal_connect_object(window, "focus-in-event",
	                        G_CALLBACK(on_popup_window_focus_change), NULL, 0);
	g_signal_connect_object(window, "focus-out-event",
	                        G_CALLBACK(on_popup_window_focus_change), NULL, 0);
}

static void
gv_main_window_setup_for_standalone(GvMainWindow *self)
{
	GvMainWindowPrivate *priv = self->priv;
	GtkMenuButton *menu_button;
	GtkHeaderBar *header_bar;

	g_assert_nonnull(priv->primary_menu);

	menu_button = GTK_MENU_BUTTON(gtk_menu_button_new());
	gtk_menu_button_set_direction(menu_button, GTK_ARROW_NONE);
	gtk_menu_button_set_menu_model(menu_button, priv->primary_menu);

	header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
	gtk_header_bar_set_show_close_button(header_bar, TRUE);
	gtk_header_bar_set_title(header_bar, g_get_application_name());
	gtk_header_bar_pack_end(header_bar, GTK_WIDGET(menu_button));

	gtk_window_set_titlebar(GTK_WINDOW(self), GTK_WIDGET(header_bar));
	gtk_widget_show_all(GTK_WIDGET(header_bar));
}

/*
 * GvConfigurable interface
 */

static void
gv_main_window_configure(GvConfigurable *configurable)
{
	GvMainWindow *self = GV_MAIN_WINDOW(configurable);

	TRACE("%p", self);

	/* We want to save the value of 'prefer-dark-theme' from GtkSettings
	 * right now, as we might modify it later, and need a way to go back
	 * to its default value.
	 */
	g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme",
	             &self->priv->system_prefer_dark_theme, NULL);

	g_assert(gv_ui_settings);
	g_settings_bind(gv_ui_settings, "theme-variant",
	                self, "theme-variant", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind(gv_ui_settings, "close-action",
	                self, "close-action", G_SETTINGS_BIND_DEFAULT);
}

static void
gv_main_window_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_main_window_configure;
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

	/* Free resources */
	g_clear_object(&priv->primary_menu);
	g_clear_object(&priv->info_tooltip_grid);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window, object);
}

static void
gv_main_window_constructed(GObject *object)
{
	GvMainWindow *self = GV_MAIN_WINDOW(object);
	GvMainWindowPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	TRACE("%p", self);

	/* Build window */
	gv_main_window_populate_widgets(self);
	gv_main_window_setup_widgets(self);
	gv_main_window_setup_appearance(self);

	/* Additional setup depending on the window mode */
	if (priv->status_icon_mode) {
		DEBUG("Setting up main window for popup mode");
		gv_main_window_setup_for_popup(self);
	} else {
		DEBUG("Setting up main window for standalone mode");
		gv_main_window_setup_for_standalone(self);
	}

	/* Connect main window signal handlers */
	g_signal_connect_object(self, "delete-event",
	                        G_CALLBACK(on_window_delete_event), NULL, 0);
	g_signal_connect_object(self, "key-press-event",
	                        G_CALLBACK(on_window_key_press_event), NULL, 0);

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

	properties[PROP_STATUS_ICON_MODE] =
	        g_param_spec_boolean("status-icon-mode", "Status icon mode", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_WRITABLE |
				     G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_PRIMARY_MENU] =
	        g_param_spec_object("primary-menu", "Primary menu", NULL,
	                            G_TYPE_MENU_MODEL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE |
				    G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_NATURAL_HEIGHT] =
	        g_param_spec_int("natural-height", "Natural height", NULL,
	                         0, G_MAXINT, 0,
	                         GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_CLOSE_ACTION] =
	        g_param_spec_enum("close-action", "Close Action", NULL,
	                          GV_TYPE_MAIN_WINDOW_CLOSE_ACTION,
	                          DEFAULT_CLOSE_ACTION,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_THEME_VARIANT] =
	        g_param_spec_enum("theme-variant", "Theme variant", NULL,
	                          GV_TYPE_MAIN_WINDOW_THEME_VARIANT,
	                          DEFAULT_THEME_VARIANT,
	                          GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Register transform function */
	g_value_register_transform_func(GV_TYPE_MAIN_WINDOW_CLOSE_ACTION,
	                                G_TYPE_STRING,
	                                gv_value_transform_enum_string);
	g_value_register_transform_func(G_TYPE_STRING,
	                                GV_TYPE_MAIN_WINDOW_CLOSE_ACTION,
	                                gv_value_transform_string_enum);
	g_value_register_transform_func(GV_TYPE_MAIN_WINDOW_THEME_VARIANT,
	                                G_TYPE_STRING,
	                                gv_value_transform_enum_string);
	g_value_register_transform_func(G_TYPE_STRING,
	                                GV_TYPE_MAIN_WINDOW_THEME_VARIANT,
	                                gv_value_transform_string_enum);
}
