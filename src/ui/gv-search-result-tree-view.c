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
#include "core/gv-core.h"

#include "ui/gv-search-result-tree-view.h"

/*
 * GObject definitions
 */

struct _GvSearchResultTreeView {
	/* Parent instance structure */
	GtkTreeView parent_instance;
};

G_DEFINE_TYPE(GvSearchResultTreeView, gv_search_result_tree_view, GTK_TYPE_TREE_VIEW)

/*
 * Columns
 */

enum {
	DETAILS_COLUMN,
	NAME_COLUMN,
	COUNTRY_COLUMN,
	CLICK_COUNT_COLUMN,
	CLICK_COUNT_STR_COLUMN,
	N_COLUMNS
};

/*
 * Helpers
 */

static gint
get_header_height(GtkTreeView *tree_view)
{
	GtkTreeViewColumn *col;
	GtkWidget *button;
	gint natural_height;

	col = gtk_tree_view_get_column(tree_view, 0);
	button = gtk_tree_view_column_get_button(col);

	gtk_widget_get_preferred_height(button,
	                                NULL,
	                                &natural_height);

	return natural_height;
}

static gint
get_row_count(GtkTreeView *tree_view)
{
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter;
	gboolean valid;
	gint row_count = 0;

	valid = gtk_tree_model_get_iter_first(tree_model, &iter);
	while (valid) {
		row_count++;
		valid = gtk_tree_model_iter_next (tree_model, &iter);
	}

	return row_count;
}

static gint
get_row_height(GtkTreeView *tree_view)
{
	GtkTreeViewColumn *col;
	GtkCellLayout *layout;
	GList *cells;
	GtkCellRenderer *cell;
	gint natural_height;

	col = gtk_tree_view_get_column(tree_view, 0);
	layout = GTK_CELL_LAYOUT(col);
	cells = gtk_cell_layout_get_cells(layout);
	g_assert(cells->next == NULL);
	cell = cells->data;
	gtk_cell_renderer_get_preferred_height(cell,
	                                       GTK_WIDGET(tree_view),
	                                       NULL,
	                                       &natural_height);
	g_list_free(cells);

	return natural_height;
}

/*
 * Public methods
 */

gint
gv_search_result_tree_view_get_optimal_height(GvSearchResultTreeView *self)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	gint header_height, row_height, row_count;

	header_height = get_header_height(tree_view);
	row_height = get_row_height(tree_view);
	row_count = get_row_count(tree_view);

	/* This way of computing the height is a bit lame and not exactly accurate,
	 * so we must add a bit of padding.
	 */
	return header_height + (row_height * row_count) + 8;
}

void
gv_search_result_tree_view_populate(GvSearchResultTreeView *self, GList *details_list)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);
	GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
	GtkListStore *list_store = GTK_LIST_STORE(tree_model);
	GList *item;

	/* Make internal list store empty */
	gtk_list_store_clear(list_store);

	/* Populate */
	for (item = details_list; item; item = item->next) {
		GvStationDetails *details = item->data;
		GtkTreeIter tree_iter;
		gchar click_count_str[32];

		g_snprintf(click_count_str, sizeof click_count_str, "%u", details->click_count);

		// TODO: we need to store details somehow

		gtk_list_store_append(list_store, &tree_iter);
		gtk_list_store_set(list_store, &tree_iter,
//		                   DETAILS_COLUMN, details,
		                   NAME_COLUMN, details->name,
		                   COUNTRY_COLUMN, details->country,
		                   CLICK_COUNT_COLUMN, details->click_count,
		                   CLICK_COUNT_STR_COLUMN, click_count_str,
		                   -1);
	}
}

GtkWidget *
gv_search_result_tree_view_new(void)
{
	return g_object_new(GV_TYPE_SEARCH_RESULT_TREE_VIEW, NULL);
}

/*
 * Construct helpers
 */

static void
gv_search_result_tree_view_setup(GvSearchResultTreeView *self)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(self);

	/* We want to be able to re-order per column */
	gtk_tree_view_set_headers_clickable(tree_view, TRUE);
}

/*
 * GObject methods
 */

static void
gv_search_result_tree_view_finalize(GObject *object)
{
	GvSearchResultTreeView *self = GV_SEARCH_RESULT_TREE_VIEW(object);

	TRACE("%p", self);
}

static void
gv_search_result_tree_view_constructed(GObject *object)
{
	GvSearchResultTreeView *self = GV_SEARCH_RESULT_TREE_VIEW(object);
	GtkTreeView *tree_view = GTK_TREE_VIEW(object);

	TRACE("%p", self);

	gv_search_result_tree_view_setup(self);

	/*
	 * Create the stations list store. It has 4 columns:
	 * - //TODO the station details (not displayed)
	 * - a button to add/remove the station
	 * - station name (+ uri ?)
	 * - country
	 * - click count
	 */

	/*
	 * Create the model
	 */

	GtkListStore *list_store;
	list_store = gtk_list_store_new(5,
	                                G_TYPE_OBJECT,
	                                G_TYPE_STRING, G_TYPE_STRING,
	                                G_TYPE_UINT, G_TYPE_STRING);

	/* Associate it with the tree view */
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(list_store));
	g_object_unref(list_store);

	/*
	 * Create the columns that will be displayed
	 */

	const gchar *title;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	// TODO: ellipsis, how to resize smartly ?

	title = _("Station");
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
	         (title, renderer, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, NAME_COLUMN);
	gtk_tree_view_append_column(tree_view, column);

	title = _("Country");
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
	         (title, renderer, "text", COUNTRY_COLUMN, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COUNTRY_COLUMN);
	gtk_tree_view_append_column(tree_view, column);

	// TODO add stream type ?

	title = _("Hits");
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
	         (title, renderer, "text", CLICK_COUNT_STR_COLUMN, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CLICK_COUNT_COLUMN);
	gtk_tree_view_append_column(tree_view, column);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_search_result_tree_view, object);
}

static void
gv_search_result_tree_view_init(GvSearchResultTreeView *self)
{
	TRACE("%p", self);
}

static void
gv_search_result_tree_view_class_init(GvSearchResultTreeViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->constructed = gv_search_result_tree_view_constructed;
	object_class->finalize    = gv_search_result_tree_view_finalize;
}
