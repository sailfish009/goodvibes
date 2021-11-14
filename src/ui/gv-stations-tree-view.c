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
#include "ui/gv-station-context-menu.h"

#include "ui/gv-stations-tree-view.h"

/*
 * Signals
 */

enum {
	SIGNAL_POPULATED,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvStationsTreeViewPrivate {
	/* Current context menu */
	GtkWidget *context_menu;
	/* Dragging operation in progress */
	gboolean is_dragging;
	GvStation *station_dragged;
	gint station_new_pos;
};

typedef struct _GvStationsTreeViewPrivate GvStationsTreeViewPrivate;

struct _GvStationsTreeView {
	/* Parent instance structure */
	GtkTreeView parent_instance;
	/* Private data */
	GvStationsTreeViewPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStationsTreeView, gv_stations_tree_view, GTK_TYPE_TREE_VIEW)

/*
 * Columns
 */

enum {
	STATION_COLUMN,
	STATION_NAME_COLUMN,
	STATION_WEIGHT_COLUMN,
	STATION_STYLE_COLUMN,
	N_COLUMNS
};

/*
 * Player signal handlers
 */

static void
on_player_notify_station(GvPlayer *player,
			 GParamSpec *pspec G_GNUC_UNUSED,
			 GvStationsTreeView *self)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GvStation *station = gv_player_get_station(player);
	GtkTreeIter iter;
	gboolean can_iter;

	can_iter = gtk_tree_model_get_iter_first(tree_model, &iter);

	while (can_iter) {
		GvStation *iter_station;

		/* Get station from model */
		gtk_tree_model_get(tree_model, &iter,
				   STATION_COLUMN, &iter_station,
				   -1);

		/* Make the current station bold */
		if (station == iter_station)
			gtk_list_store_set(GTK_LIST_STORE(tree_model), &iter,
					   STATION_WEIGHT_COLUMN, PANGO_WEIGHT_BOLD,
					   -1);
		else
			gtk_list_store_set(GTK_LIST_STORE(tree_model), &iter,
					   STATION_WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
					   -1);

		/* Unref the station */
		if (iter_station)
			g_object_unref(iter_station);

		/* Next! */
		can_iter = gtk_tree_model_iter_next(tree_model, &iter);
	}
}

/*
 * Station List signal handlers
 * Needed to update the internal list store when the station list is modified.
 * (remember the station list might be updated through the D-Bus API).
 */

static void
on_station_list_loaded(GvStationList *station_list,
		       GvStationsTreeView *self)
{
	TRACE("%p, %p", station_list, self);

	gv_stations_tree_view_populate(self);
}

static void
on_station_list_station_event(GvStationList *station_list,
			      GvStation *station,
			      GvStationsTreeView *self)
{
	TRACE("%p, %p, %p", station_list, station, self);

	gv_stations_tree_view_populate(self);
}

static GSignalHandler station_list_handlers[] = {
	// clang-format off
	{ "loaded",           G_CALLBACK(on_station_list_loaded)        },
	{ "station-added",    G_CALLBACK(on_station_list_station_event) },
	{ "station-removed",  G_CALLBACK(on_station_list_station_event) },
	{ "station-modified", G_CALLBACK(on_station_list_station_event) },
	{ "station-moved",    G_CALLBACK(on_station_list_station_event) },
	{ NULL,               NULL                                      }
	// clang-format on
};

/*
 * Stations tree view row activated
 * Might be caused by mouse action (single click on the row),
 * or by keyboard action (Enter or similar key pressed).
 */

static void
on_tree_view_row_activated(GvStationsTreeView *self,
			   GtkTreePath         *path,
			   GtkTreeViewColumn   *column G_GNUC_UNUSED,
			   gpointer             data G_GNUC_UNUSED)
{
	GvStationsTreeViewPrivate *priv = self->priv;
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter;
	GvStation *station;

	/* Get station from model */
	gtk_tree_model_get_iter(tree_model, &iter, path);
	gtk_tree_model_get(tree_model, &iter,
			   STATION_COLUMN, &station,
			   -1);

	/* Play station */
	if (station) {
		GvPlayer *player = gv_core_player;

		gv_player_set_station(player, station);
		gv_player_play(player);
		g_object_unref(station);
	}

	DEBUG("Row activated");
}

/*
 * Stations Tree View button-press-event, for context menu on right-click
 */

static void
on_context_menu_hide(GtkWidget *widget, GvStationsTreeView *self)
{
	GvStationsTreeViewPrivate *priv = self->priv;

	/* Sanity check */
	if (widget != priv->context_menu)
		CRITICAL("'hide' signal from unknown context menu");

	/* Context menu can be destroyed */
	g_object_unref(widget);
	priv->context_menu = NULL;
}

static gboolean
on_tree_view_button_press_event(GvStationsTreeView *self,
				GdkEventButton *event,
				gpointer data G_GNUC_UNUSED)

{
	GvStationsTreeViewPrivate *priv = self->priv;
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GtkTreePath *path;
	GvStation *station;
	GtkWidget *context_menu;

	DEBUG("Button pressed: %d", event->button);

	/* Ensure the event happened on the expected window.
	 * According from the doc, we MUST check that.
	 */
	if (event->window != gtk_tree_view_get_bin_window(tree_view))
		return GDK_EVENT_PROPAGATE;

	/* Handle only single-click */
	if (event->type != GDK_BUTTON_PRESS)
		return GDK_EVENT_PROPAGATE;

	/* Handle only right-click */
	if (event->button != 3)
		return GDK_EVENT_PROPAGATE;

	/* Do nothing if there's already a context menu (this case shouldn't happen) */
	if (priv->context_menu) {
		WARNING("Context menu already exists!");
		return GDK_EVENT_PROPAGATE;
	}

	/* Get row at this position */
	path = NULL;
	gtk_tree_view_get_path_at_pos(tree_view, event->x, event->y,
				      &path, NULL, NULL, NULL);

	/* Get corresponding station */
	station = NULL;
	if (path != NULL) {
		GtkTreeIter iter;
		gtk_tree_model_get_iter(tree_model, &iter, path);
		gtk_tree_model_get(tree_model, &iter,
				   STATION_COLUMN, &station,
				   -1);
	}

	/* Create the context menu */
	if (station) {
		context_menu = gv_station_context_menu_new_with_station(station);
		g_object_unref(station);
	} else {
		context_menu = gv_station_context_menu_new();
	}

	/* Pop it up */
#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_pointer(GTK_MENU(context_menu), NULL);
#else
	gtk_menu_popup(GTK_MENU(context_menu),
		       NULL,
		       NULL,
		       NULL,
		       NULL,
		       event->button,
		       event->time);
#endif

	/* Save it for later use, handle destruction in callback */
	priv->context_menu = g_object_ref_sink(context_menu);
	g_signal_connect_object(context_menu, "hide", G_CALLBACK(on_context_menu_hide), self, 0);

	/* Free at last */
	gtk_tree_path_free(path);

	return GDK_EVENT_PROPAGATE;
}

/*
 * Stations Tree View drag-and-drop source signal handlers.
 * We watch these signals to know when a dragging operation is in progress.
 */

static void
on_tree_view_drag_begin(GvStationsTreeView *self,
			GdkDragContext *context G_GNUC_UNUSED,
			gpointer data G_GNUC_UNUSED)
{
	GvStationsTreeViewPrivate *priv = self->priv;

	priv->is_dragging = TRUE;
}

static void
on_tree_view_drag_end(GvStationsTreeView *self,
		      GdkDragContext *context G_GNUC_UNUSED,
		      gpointer data G_GNUC_UNUSED)
{
	GvStationsTreeViewPrivate *priv = self->priv;

	priv->is_dragging = FALSE;
}

static gboolean
on_tree_view_drag_failed(GvStationsTreeView *self,
			 GdkDragContext *context G_GNUC_UNUSED,
			 GtkDragResult result,
			 gpointer data G_GNUC_UNUSED)
{
	GvStationsTreeViewPrivate *priv = self->priv;

	DEBUG("Drag failed with result: %d", result);
	priv->is_dragging = FALSE;

	return TRUE;
}

static GSignalHandler tree_view_drag_handlers[] = {
	// clang-format off
	{ "drag-begin",  G_CALLBACK(on_tree_view_drag_begin)  },
	{ "drag-end",    G_CALLBACK(on_tree_view_drag_end)    },
	{ "drag-failed", G_CALLBACK(on_tree_view_drag_failed) },
	{ NULL,          NULL                                 }
	// clang-format on
};

/*
 * Stations List Store signal handlers.
 *
 * We watch these signals to be notified when a station is moved
 * in the list (this is done when user drag'n'drop).
 * After a station has been moved, we need to forward this change
 * to the core station list.
 *
 * The signal sequence when a station is moved is as follow:
 * - row-inserted: a new empty row is created
 * - row-changed: the new row has been populated
 * - row-deleted: the old row has been deleted
 */

static void
on_list_store_row_inserted(GtkTreeModel *tree_model G_GNUC_UNUSED,
			   GtkTreePath *path,
			   GtkTreeIter *iter G_GNUC_UNUSED,
			   GvStationsTreeView *self)
{
	GvStationsTreeViewPrivate *priv = self->priv;
	gint *indices;
	guint position;

	/* This should only happen when there's a drag-n-drop */
	if (priv->is_dragging == FALSE) {
		WARNING("Not dragging at the moment, ignoring");
		return;
	}

	/* We expect a clean status */
	if (priv->station_dragged != NULL || priv->station_new_pos != -1) {
		WARNING("Current state is not clean, ignoring");
		return;
	}

	/* Get position of the new row */
	indices = gtk_tree_path_get_indices(path);
	position = indices[0];
	priv->station_new_pos = position;

	DEBUG("Row inserted at %d", position);
}

static void
on_list_store_row_changed(GtkTreeModel *tree_model,
			  GtkTreePath *path,
			  GtkTreeIter *iter,
			  GvStationsTreeView *self)
{
	GvStationsTreeViewPrivate *priv = self->priv;
	GvStation *station;
	gint *indices;
	gint position;

	/* We only care if it's caused by a drag'n'drop */
	if (priv->is_dragging == FALSE)
		return;

	/* We expect the indice to be the one registered previously
	 * in 'row-inserted' signal.
	 */
	indices = gtk_tree_path_get_indices(path);
	position = indices[0];
	if (position != priv->station_new_pos) {
		WARNING("Unexpected position %d, doesn't match %d",
			position, priv->station_new_pos);
		return;
	}

	/* Get station */
	gtk_tree_model_get(tree_model, iter,
			   STATION_COLUMN, &station,
			   -1);

	/* Save it */
	priv->station_dragged = station;

	/* Freedom for the braves */
	g_object_unref(station);

	DEBUG("Row changed at %d", position);
}

static void
on_list_store_row_deleted(GtkTreeModel *tree_model G_GNUC_UNUSED,
			  GtkTreePath *path G_GNUC_UNUSED,
			  GvStationsTreeView *self)
{
	GvStationsTreeViewPrivate *priv = self->priv;
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	GvStationList *station_list = gv_core_station_list;
	GtkTreeSelection *select;
	GvStation *station;
	guint indice_inserted;

	/* End of drag operation, let's commit that to station list */
	station = priv->station_dragged;
	if (station == NULL) {
		WARNING("Station dragged is null, wtf?");
		return;
	}

	/* Move station in the station list */
	indice_inserted = priv->station_new_pos;
	g_signal_handlers_block(station_list, station_list_handlers, self);
	gv_station_list_move(station_list, station, indice_inserted);
	g_signal_handlers_unblock(station_list, station_list_handlers, self);
	DEBUG("Row deleted, station moved at %d", indice_inserted);

	/* Clean status */
	priv->station_dragged = NULL;
	priv->station_new_pos = -1;

	/* Reset selection, as GTK doesn't do it itself, hence the column that
	 * was selected (ie. highlighted as we're in hover selection mode)
	 * before the drag'n'drop is highligted again after the drag'n'drop.
	 */
	select = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_unselect_all(select);
}

static GSignalHandler list_store_handlers[] = {
	// clang-format off
	{ "row-inserted", G_CALLBACK(on_list_store_row_inserted) },
	{ "row-changed",  G_CALLBACK(on_list_store_row_changed)  },
	{ "row-deleted",  G_CALLBACK(on_list_store_row_deleted)  },
	{ NULL,           NULL                                   }
	// clang-format on
};

/*
 * Helpers
 */

static void
station_cell_data_func(GtkTreeViewColumn *tree_column G_GNUC_UNUSED,
		       GtkCellRenderer *cell,
		       GtkTreeModel *tree_model,
		       GtkTreeIter *iter,
		       gpointer data G_GNUC_UNUSED)
{
	gchar *station_name;
	PangoWeight station_weight;
	PangoStyle station_style;

	/* According to the doc, there should be nothing heavy in this function,
	 * since it's called intensively. No UTF-8 conversion, for example.
	 */

	gtk_tree_model_get(tree_model, iter,
			   STATION_NAME_COLUMN, &station_name,
			   STATION_WEIGHT_COLUMN, &station_weight,
			   STATION_STYLE_COLUMN, &station_style,
			   -1);

	g_object_set(cell,
		     "text", station_name,
		     "weight", station_weight,
		     "style", station_style,
		     NULL);

	g_free(station_name);
}

/*
 * Public methods
 */

void
gv_stations_tree_view_populate(GvStationsTreeView *self)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GtkListStore *list_store = GTK_LIST_STORE(tree_model);

	GvStationList *station_list = gv_core_station_list;
	GvPlayer *player = gv_core_player;

	TRACE("%p", self);

	/* Block list store handlers */
	g_signal_handlers_block(list_store, list_store_handlers, self);

	/* Make station list empty */
	gtk_list_store_clear(list_store);

	/* Handle the special-case: empty station list */
	if (gv_station_list_length(station_list) == 0) {
		GtkTreeIter tree_iter;

		/* Populate */
		gtk_list_store_append(list_store, &tree_iter);
		gtk_list_store_set(list_store, &tree_iter,
				   STATION_COLUMN, NULL,
				   STATION_NAME_COLUMN, "Right click to add station",
				   STATION_WEIGHT_COLUMN, PANGO_WEIGHT_NORMAL,
				   STATION_STYLE_COLUMN, PANGO_STYLE_ITALIC,
				   -1);

		/* Configure behavior */
		gtk_tree_view_set_hover_selection(tree_view, FALSE);
		gtk_tree_view_set_activate_on_single_click(tree_view, FALSE);

	} else {
		GvStation *current_station = gv_player_get_station(player);
		GvStation *station;
		GvStationListIter *iter;

		/* Populate menu with every station */
		iter = gv_station_list_iter_new(station_list);

		while (gv_station_list_iter_loop(iter, &station)) {
			GtkTreeIter tree_iter;
			const gchar *station_name;
			PangoWeight weight;

			station_name = gv_station_get_name_or_uri(station);

			if (station == current_station)
				weight = PANGO_WEIGHT_BOLD;
			else
				weight = PANGO_WEIGHT_NORMAL;

			gtk_list_store_append(list_store, &tree_iter);
			gtk_list_store_set(list_store, &tree_iter,
					   STATION_COLUMN, station,
					   STATION_NAME_COLUMN, station_name,
					   STATION_WEIGHT_COLUMN, weight,
					   STATION_STYLE_COLUMN, PANGO_STYLE_NORMAL,
					   -1);
		}
		gv_station_list_iter_free(iter);

		/* Configure behavior */
		gtk_tree_view_set_hover_selection(tree_view, TRUE);
		gtk_tree_view_set_activate_on_single_click(tree_view, TRUE);
	}

	/* Unblock list store handlers */
	g_signal_handlers_unblock(list_store, list_store_handlers, self);

	/* Emit a signal */
	g_signal_emit(self, signals[SIGNAL_POPULATED], 0);
}

gboolean
gv_stations_tree_view_has_context_menu(GvStationsTreeView *self)
{
	GvStationsTreeViewPrivate *priv = self->priv;

	return priv->context_menu ? TRUE : FALSE;
}

GtkWidget *
gv_stations_tree_view_new(void)
{
	return g_object_new(GV_TYPE_STATIONS_TREE_VIEW, NULL);
}

/*
 * GObject methods
 */

static void
gv_stations_tree_view_constructed(GObject *object)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(object);
	GvStationsTreeView *self = GV_STATIONS_TREE_VIEW(object);

	/* Hide headers */
	gtk_tree_view_set_headers_visible(tree_view, FALSE);

	/* Enable hover selection mode, and single click activation */
	gtk_tree_view_set_hover_selection(tree_view, TRUE);
	gtk_tree_view_set_activate_on_single_click(tree_view, TRUE);

	/* Allow re-ordering. The tree view then becomes a drag source and
	 * a drag destination, and all the drag-and-drop mess is handled
	 * by the tree view. We will just have to watch the drag-* signals.
	 */
	gtk_tree_view_set_reorderable(tree_view, TRUE);

	/* Horizontally, the tree view grows as wide as the longer station name.
	 * I'm OK with this behavior, let it be.
	 */

	/* Vertically, the tree view grows forever. If someone has too many stations,
	 * it might cause a problem.
	 * I tried to put the tree view inside a scrolled window, but it creates more
	 * problems than it solves, mainly because then we have to assign a fixed size
	 * to the tree view. Finding the appropriate size (that would be the natural,
	 * expanded size if NOT within a scrolled window, OR the screen height if too
	 * many stations) seems VERY VERY tricky and slippery...
	 */

	/*
	 * Create the stations list store. It has 4 columns:
	 * - the station object
	 * - the station represented by a string (for displaying)
	 * - the station's font weight (bold characters for current station)
	 * - the station's font style (italic characters if no station)
	 */

	/* Create a new list store */
	GtkListStore *list_store;
	list_store = gtk_list_store_new(4, G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

	/* Associate it with the tree view */
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(list_store));
	g_object_unref(list_store);

	/*
	 * Create the column that will be displayed
	 */

	/* Create a renderer */
	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	/* Create a column that uses this renderer */
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("Station", renderer, NULL);

	/* Set the function that will render this column */
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						station_cell_data_func,
						NULL, NULL);

	/* Append the column */
	gtk_tree_view_append_column(tree_view, column);

	/*
	 * Tree View signal handlers
	 */

	/* Left click or keyboard */
	g_signal_connect_object(tree_view, "row-activated",
				G_CALLBACK(on_tree_view_row_activated), NULL, 0);

	/* We handle the right-click here */
	g_signal_connect_object(tree_view, "button-press-event",
				G_CALLBACK(on_tree_view_button_press_event), NULL, 0);

	/* Drag-n-drop signal handlers.
	 * We need to watch it just to know when a drag-n-drop is in progress.
	 */
	g_signal_handlers_connect_object(tree_view, tree_view_drag_handlers, NULL, 0);

	/*
	 * List Store signal handlers
	 */

	/* If stations are re-ordered, we have to propagate to the core */
	g_signal_handlers_connect_object(list_store, list_store_handlers, self, 0);

	/*
	 * Core signal handlers
	 */

	GvPlayer *player = gv_core_player;
	GvStationList *station_list = gv_core_station_list;

	g_signal_connect_object(player, "notify::station",
				G_CALLBACK(on_player_notify_station), self, 0);
	g_signal_handlers_connect_object(station_list, station_list_handlers, self, 0);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_stations_tree_view, object);
}

static void
gv_stations_tree_view_init(GvStationsTreeView *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_stations_tree_view_get_instance_private(self);

	/* Initialize internal state */
	self->priv->station_new_pos = -1;
}

static void
gv_stations_tree_view_class_init(GvStationsTreeViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->constructed = gv_stations_tree_view_constructed;

	/* Signals */
	signals[SIGNAL_POPULATED] =
		g_signal_new("populated", G_TYPE_FROM_CLASS(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
