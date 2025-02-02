/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2021-2024 Arnaud Rebillout
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
#include "ui/gtk-additions.h"
#include "ui/gv-stations-tree-view.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-volume-control.h"

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
	GtkWidget *playlist_view_box;
	/* Current station */
	GtkWidget *station_grid;
	GtkWidget *station_name_label;
	GtkWidget *playback_status_label;
	GtkWidget *go_next_button;
	/* Controls */
	GtkWidget *button_box;
	GtkWidget *play_button;
	GtkWidget *prev_button;
	GtkWidget *next_button;
	GtkWidget *volume_control;
	GtkWidget *repeat_toggle_button;
	GtkWidget *shuffle_toggle_button;
	/* Station list */
	GtkWidget *scrolled_window;
	GtkWidget *stations_tree_view;

	/*
	 * Internal
	 */

	GBinding *repeat_binding;
	GBinding *shuffle_binding;
};

typedef struct _GvPlaylistViewPrivate GvPlaylistViewPrivate;

struct _GvPlaylistView {
	/* Parent instance structure */
	GtkApplicationWindow parent_instance;
	/* Private data */
	GvPlaylistViewPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvPlaylistView, gv_playlist_view, GTK_TYPE_BOX)

/*
 * Core signal handlers
 */

static void
set_station_name_label(GtkLabel *label, GvStation *station)
{
	const gchar *station_title;

	if (station)
		station_title = gv_station_get_name_or_uri(station);
	else
		station_title = _("No station selected");

	gtk_label_set_text(label, station_title);
}

static void
set_playback_status_label(GtkLabel *label, GvPlaybackState state,
		GvMetadata *metadata)
{
	const gchar *playback_state_str;

	playback_state_str = gv_playback_state_to_string(state);

	if (state != GV_PLAYBACK_STATE_PLAYING) {
		gtk_label_set_text(label, playback_state_str);
		return;
	}

	if (metadata == NULL) {
		gtk_label_set_text(label, playback_state_str);
	} else {
		gchar *artist_title;
		gchar *album_year;
		gchar *str;

		artist_title = gv_metadata_make_title_artist(metadata, FALSE);
		album_year = gv_metadata_make_album_year(metadata, FALSE);

		if (artist_title && album_year)
			str = g_strdup_printf("%s\n%s", artist_title, album_year);
		else if (artist_title)
			str = g_strdup(artist_title);
		else
			str = g_strdup(playback_state_str);

		gtk_label_set_text(label, str);

		g_free(str);
		g_free(album_year);
		g_free(artist_title);
	}
}

static void
set_play_button(GtkButton *button, gboolean playing)
{
	GtkWidget *image;
	const gchar *icon_name;

	if (playing == TRUE)
		icon_name = "media-playback-stop-symbolic";
	else
		icon_name = "media-playback-start-symbolic";

	image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(button, image);
}

/*
 * Private methods
 */

static void
gv_playlist_view_update_station_name_label(GvPlaylistView *self, GvPlayback *playback)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkLabel *label = GTK_LABEL(priv->station_name_label);
	GvStation *station = gv_playback_get_station(playback);

	set_station_name_label(label, station);
}

static void
gv_playlist_view_update_playback_status_label(GvPlaylistView *self, GvPlayback *playback)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkLabel *label = GTK_LABEL(priv->playback_status_label);
	GvPlaybackState state = gv_playback_get_state(playback);
	GvMetadata *metadata = gv_playback_get_metadata(playback);

	set_playback_status_label(label, state, metadata);
}

static void
gv_playlist_view_update_play_button(GvPlaylistView *self, GvPlayer *player)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkButton *button = GTK_BUTTON(priv->play_button);
	gboolean playing = gv_player_get_playing(player);

	set_play_button(button, playing);
}

/*
 * Signal handlers
 */

static void
on_player_notify(GvPlayer *player, GParamSpec *pspec, GvPlaylistView *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", player, property_name, self);

	if (!g_strcmp0(property_name, "playing")) {
		gv_playlist_view_update_play_button(self, player);
	}
}

static void
on_playback_notify(GvPlayback *playback, GParamSpec *pspec, GvPlaylistView *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", playback, property_name, self);

	if (!g_strcmp0(property_name, "station")) {
		gv_playlist_view_update_station_name_label(self, playback);
	} else if (!g_strcmp0(property_name, "state")) {
		gv_playlist_view_update_playback_status_label(self, playback);
	} else if (!g_strcmp0(property_name, "error")) {
		gv_playlist_view_update_playback_status_label(self, playback);
	} else if (!g_strcmp0(property_name, "metadata")) {
		gv_playlist_view_update_playback_status_label(self, playback);
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
	GvPlayback *playback = gv_core_playback;

	TRACE("%p, %p", self, user_data);

	/* Connect core signal handlers */
	g_signal_connect_object(player, "notify",
				G_CALLBACK(on_player_notify), self, 0);
	g_signal_connect_object(playback, "notify",
				G_CALLBACK(on_playback_notify), self, 0);

	/* Create bindings between widgets properties and player properties.
	 * Order matters, don't mix up source and target here.
	 */
	priv->repeat_binding = g_object_bind_property(
		player, "repeat", priv->repeat_toggle_button, "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	priv->shuffle_binding = g_object_bind_property(
		player, "shuffle", priv->shuffle_toggle_button, "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	/* Update widgets */
	gv_playlist_view_update_station_name_label(self, playback);
	gv_playlist_view_update_playback_status_label(self, playback);
	gv_playlist_view_update_play_button(self, player);
}

static void
on_unmap(GvPlaylistView *self, gpointer user_data G_GNUC_UNUSED)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	GvPlayback *playback = gv_core_playback;

	TRACE("%p, %p", self, user_data);

	/* Disconnect core signal handlers */
	g_signal_handlers_disconnect_by_data(playback, self);
	g_signal_handlers_disconnect_by_data(player, self);

	/* Release bindings */
	g_binding_unbind(priv->repeat_binding);
	priv->repeat_binding = NULL;
	g_binding_unbind(priv->shuffle_binding);
	priv->shuffle_binding = NULL;
}

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
gv_playlist_view_populate_widgets(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;
	GtkBuilder *builder;

	/* Build the ui */
	builder = gtk_builder_new_from_resource(UI_RESOURCE_PATH);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, playlist_view_box);

	/* Current status */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_name_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, playback_status_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, go_next_button);

	/* Button box */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, button_box);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, play_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, prev_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, next_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, repeat_toggle_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, shuffle_toggle_button);

	/* Create the volume control, add it to the button box */
	priv->volume_control = gv_volume_control_new();
	gtk_container_add(GTK_CONTAINER(priv->button_box), priv->volume_control);

	/* Stations tree view */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, scrolled_window);

	/* Create the stations tree view, add it to the scrolled window */
	priv->stations_tree_view = gv_stations_tree_view_new();
	//gtk_widget_show_all(priv->stations_tree_view);
	gtk_container_add(GTK_CONTAINER(priv->scrolled_window), priv->stations_tree_view);

	/* Pack that within the box */
	gtk_container_add(GTK_CONTAINER(self), priv->playlist_view_box);

	/* Cleanup */
	g_object_unref(builder);
}

static void
gv_playlist_view_setup_appearance(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;

	g_object_set(priv->playlist_view_box,
		     "margin", GV_UI_MAIN_WINDOW_MARGIN,
		     "spacing", GV_UI_ELEM_SPACING,
		     NULL);
	g_object_set(priv->station_grid,
		     "column-spacing", GV_UI_ELEM_SPACING,
		     NULL);
}

static void
gv_playlist_view_setup_widgets(GvPlaylistView *self)
{
	GvPlaylistViewPrivate *priv = self->priv;

	/* Give a name to some widgets, for the status icon window */
	gtk_widget_set_name(priv->go_next_button, "go_next_button");
	gtk_widget_set_name(priv->station_name_label, "station_name_label");
	gtk_widget_set_name(priv->stations_tree_view, "stations_tree_view");

	/* Connect widgets signal handlers */
	g_signal_connect_object(priv->play_button, "clicked",
				G_CALLBACK(on_control_button_clicked), self, 0);
	g_signal_connect_object(priv->prev_button, "clicked",
				G_CALLBACK(on_control_button_clicked), self, 0);
	g_signal_connect_object(priv->next_button, "clicked",
				G_CALLBACK(on_control_button_clicked), self, 0);
	g_signal_connect_object(priv->go_next_button, "clicked",
				G_CALLBACK(on_go_next_button_clicked), self, 0);

	/* Connect self signal handlers */
	g_signal_connect_object(self, "map", G_CALLBACK(on_map), NULL, 0);
	g_signal_connect_object(self, "unmap", G_CALLBACK(on_unmap), NULL, 0);

	/* Make sure that the play/pause button has focus */
	gtk_widget_grab_focus(priv->play_button);
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
		g_signal_new("go-next-clicked", G_OBJECT_CLASS_TYPE(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
