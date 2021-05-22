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
#include "ui/gtk-additions.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-stations-tree-view.h"

#include "ui/gv-playlist-view.h"

#define UI_RESOURCE_PATH GV_APPLICATION_PATH "/Ui/playlist-view.glade"

/*
 * Signal
 */

enum {
	SIGNAL_GO_NEXT_CLICKED,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvPlaylistViewPrivate {

	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *window_vbox;
	/* Current status */
	GtkWidget *info_grid;
	GtkWidget *station_label;
	GtkWidget *status_label;
	GtkWidget *go_next_button;
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

	GBinding *repeat_binding;
	GBinding *shuffle_binding;
	GBinding *volume_binding;
};

typedef struct _GvPlaylistViewPrivate GvPlaylistViewPrivate;

struct _GvPlaylistView {
	/* Parent instance structure */
	GtkApplicationWindow  parent_instance;
	/* Private data */
	GvPlaylistViewPrivate  *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvPlaylistView, gv_playlist_view, GTK_TYPE_BOX)

/*
 * Private methods
 */

#if 0
static gint
gv_playlist_view_compute_natural_height(GvPlaylistView *self)
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

/*
 * Private methods
 */

static void
gv_playlist_view_update_station_label(GvPlaylistView *self, GvPlayer *player)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkLabel *label = GTK_LABEL(priv->station_label);
	GvStation *station = gv_player_get_station(player);

	set_station_label(label, station);
}

static void
gv_playlist_view_update_status_label(GvPlaylistView *self, GvPlayer *player)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkLabel *label = GTK_LABEL(priv->status_label);
	GvPlayerState state = gv_player_get_state(player);
	GvMetadata *metadata = gv_player_get_metadata(player);

	set_status_label(label, state, metadata);
}

static void
gv_playlist_view_update_play_button(GvPlaylistView *self, GvPlayer *player)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkButton *button = GTK_BUTTON(priv->play_button);
	GvPlayerState state = gv_player_get_state(player);

	set_play_button(button, state);
}

static void
gv_playlist_view_update_volume_button(GvPlaylistView *self, GvPlayer *player)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkVolumeButton *volume_button = GTK_VOLUME_BUTTON(priv->volume_button);
	guint volume = gv_player_get_volume(player);
	gboolean mute = gv_player_get_mute(player);

	g_binding_unbind(priv->volume_binding);
	set_volume_button(volume_button, volume, mute);
	priv->volume_binding = g_object_bind_property(
			player, "volume", volume_button, "value",
			G_BINDING_BIDIRECTIONAL);
}

/*
 * Signal handlers
 */

static void
on_player_notify(GvPlayer     *player,
                 GParamSpec    *pspec,
                 GvPlaylistView *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", player, property_name, self);

	if (!g_strcmp0(property_name, "station")) {
		gv_playlist_view_update_station_label(self, player);
	} else if (!g_strcmp0(property_name, "state")) {
		gv_playlist_view_update_status_label(self, player);
		gv_playlist_view_update_play_button(self, player);
	} else if (!g_strcmp0(property_name, "metadata")) {
		gv_playlist_view_update_status_label(self, player);
	}  else if (!g_strcmp0(property_name, "mute")) {
		gv_playlist_view_update_volume_button(self, player);
	}
}

static void
on_control_button_clicked(GtkButton *button, GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	GtkWidget *widget = GTK_WIDGET(button);

	if (widget == priv->play_button) {
		gv_player_toggle(player);
	} else if (widget == priv->prev_button) {
		gv_player_prev(player);
	} else if (widget == priv->next_button) {
		gv_player_next(player);
	} else if (widget == priv->go_next_button) {
		CRITICAL("Unhandled button %p", button);
	}
}

static void
on_go_next_button_clicked(GtkButton *button G_GNUC_UNUSED, GvPlaylistView *self)
{
	g_signal_emit(self, signals[SIGNAL_GO_NEXT_CLICKED], 0);
}

static void
on_map(GvPlaylistView *self, gpointer user_data)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	TRACE("%p, %p", self, user_data);

	g_signal_connect_object(player, "notify",
				G_CALLBACK(on_player_notify), self, 0);

	/* Order matters, don't mix up source and target here */
	priv->repeat_binding = g_object_bind_property(
			player, "repeat", priv->repeat_toggle_button, "active",
			G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	priv->shuffle_binding = g_object_bind_property(
			player, "shuffle", priv->shuffle_toggle_button, "active",
			G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	priv->volume_binding = g_object_bind_property(
			player, "volume", priv->volume_button, "value",
			G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	gv_playlist_view_update_station_label(self, player);
	gv_playlist_view_update_status_label(self, player);
	gv_playlist_view_update_play_button(self, player);
	gv_playlist_view_update_volume_button(self, player);
}

static void
on_unmap(GvPlaylistView *self, gpointer user_data G_GNUC_UNUSED)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	TRACE("%p, %p", self, user_data);

	g_signal_handlers_disconnect_by_data(player, self);

	g_binding_unbind(priv->repeat_binding);
	priv->repeat_binding = NULL;
	g_binding_unbind(priv->shuffle_binding);
	priv->shuffle_binding = NULL;
	g_binding_unbind(priv->volume_binding);
	priv->volume_binding = NULL;
}

#if 0
/*
 * Stations tree view signal handlers
 */

static gboolean
when_idle_compute_natural_height(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;
	gint natural_height;

	natural_height = gv_playlist_view_compute_natural_height(self);
	if (natural_height != priv->natural_height) {
		priv->natural_height = natural_height;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_NATURAL_HEIGHT]);
	}

	return G_SOURCE_REMOVE;
}

static void
on_stations_tree_view_populated(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                                GvPlaylistView *self)
{
	/* If the content of the stations tree view was modified, the natural size
	 * changed also. However it's too early to compute the new size now.
	 */
	g_idle_add((GSourceFunc) when_idle_compute_natural_height, self);
}

static void
on_stations_tree_view_realize(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                              GvPlaylistView *self)
{
	/* When the treeview is realized, we need to check AGAIN if the natural
	 * height we have is correct.
	 */
	g_idle_add((GSourceFunc) when_idle_compute_natural_height, self);
}

static gboolean
on_stations_tree_view_map_event(GtkWidget *stations_tree_view G_GNUC_UNUSED,
                                GdkEvent *event G_GNUC_UNUSED,
                                GvPlaylistViewManager *self)
{
	/* When the treeview is mapped, we need to check AGAIN if the natural
	 * height we have is correct.
	 */
	g_idle_add((GSourceFunc) when_idle_compute_natural_height, self);

	return GDK_EVENT_PROPAGATE;
}
#endif

/*
 * Public methods
 */

GtkWidget *
gv_playlist_view_new(void)
{
	return g_object_new(GV_TYPE_PLAYLIST_VIEW, NULL);
}

/*
 * Construct helpers
 */

static void
setup_adjustment(GtkScaleButton *scale_button, GObject *obj, const gchar *obj_prop)
{
	GtkAdjustment *adjustment;
	guint minimum, maximum;
	guint range;

	g_object_get_property_uint_bounds(obj, obj_prop, &minimum, &maximum);
	range = maximum - minimum;

	adjustment = gtk_scale_button_get_adjustment(scale_button);
	gtk_adjustment_set_lower(adjustment, minimum);
	gtk_adjustment_set_upper(adjustment, maximum);
	gtk_adjustment_set_step_increment(adjustment, range / 100);
	gtk_adjustment_set_page_increment(adjustment, range / 10);
}

static void
gv_playlist_view_populate_widgets(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkBuilder *builder;

	/* Build the ui */
	builder = gtk_builder_new_from_resource(UI_RESOURCE_PATH);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_vbox);

	/* Current status */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, info_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, status_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, go_next_button);

	/* Button box */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, play_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, prev_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, next_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, repeat_toggle_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, shuffle_toggle_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, volume_button);

	/* Stations tree view */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, scrolled_window);

	/* Create the stations tree view, add it to the scrolled window */
	priv->stations_tree_view = gv_stations_tree_view_new();
	//gtk_widget_show_all(priv->stations_tree_view);
	gtk_container_add(GTK_CONTAINER(priv->scrolled_window), priv->stations_tree_view);

	/* Pack that within the box */
	gtk_container_add(GTK_CONTAINER(self), priv->window_vbox);

	/* Cleanup */
	g_object_unref(builder);
}

static void
gv_playlist_view_setup_appearance(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;

	g_object_set(priv->window_vbox,
		     "margin", GV_UI_MAIN_WINDOW_MARGIN,
	             "spacing", GV_UI_ELEM_SPACING,
	             NULL);
	g_object_set(priv->info_grid,
		     "column-spacing", GV_UI_ELEM_SPACING,
		     NULL);
}

static void
gv_playlist_view_setup_widgets(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GObject *player_obj = G_OBJECT(gv_core_player);

	/* Setup adjustment for the volume button */
	setup_adjustment(GTK_SCALE_BUTTON(priv->volume_button), player_obj, "volume");

	/* Connect next button */
	g_signal_connect_object(priv->go_next_button, "clicked",
				G_CALLBACK(on_go_next_button_clicked), self, 0);

	/* Connect controls */
	g_signal_connect_object(priv->play_button, "clicked",
			        G_CALLBACK(on_control_button_clicked), self, 0);
	g_signal_connect_object(priv->prev_button, "clicked",
			        G_CALLBACK(on_control_button_clicked), self, 0);
	g_signal_connect_object(priv->next_button, "clicked",
			        G_CALLBACK(on_control_button_clicked), self, 0);

#if 0
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
#endif

	/* Connect map and unmap */
	g_signal_connect_object(self, "map", G_CALLBACK(on_map), NULL, 0);
	g_signal_connect_object(self, "unmap", G_CALLBACK(on_unmap), NULL, 0);
}


/*
 * GObject methods
 */

static void
gv_playlist_view_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_playlist_view, object);
}

static void
gv_playlist_view_constructed(GObject *object)
{
	GvPlaylistView *self = GV_PLAYLIST_VIEW(object);

	TRACE("%p", self);

	/* Build widget */
	gv_playlist_view_populate_widgets(self);
	gv_playlist_view_setup_appearance(self);
	gv_playlist_view_setup_widgets(self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_playlist_view, object);
}

static void
gv_playlist_view_init(GvPlaylistView *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_playlist_view_get_instance_private(self);
}

static void
gv_playlist_view_class_init(GvPlaylistViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_playlist_view_finalize;
	object_class->constructed = gv_playlist_view_constructed;

	/* Signals */
	signals[SIGNAL_GO_NEXT_CLICKED] =
	        g_signal_new("go-next-clicked", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
