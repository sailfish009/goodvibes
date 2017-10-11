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

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "additions/gtk.h"
#include "additions/glib-object.h"
#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-search-result-tree-view.h"

#include "ui/gv-station-window.h"

#define UI_FILE "station-window.glade"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_NEW_STATION,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvStationWindowPrivate {
	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *window_vbox;
	GtkWidget *notebook;
	/* Search */
	GtkWidget *search_vbox;
	GtkWidget *search_grid;
	GtkWidget *search_entry;
	GtkWidget *search_button;
	GtkWidget *search_status_label;
	GtkWidget *search_status_spinner;
	GtkWidget *search_title_label;
	GtkWidget *search_scrolled_window;
	/* Manual */
	GtkWidget *manual_vbox;
	GtkWidget *manual_grid;
	GtkWidget *manual_name_entry;
	GtkWidget *manual_uri_entry;
	GtkWidget *manual_save_button;

	/*
	 * Status
	 */
	guint      start_search_spinner_source_id;

	/*
	 * Station that was created
	 */
	GvStation *new_station;
};

typedef struct _GvStationWindowPrivate GvStationWindowPrivate;

struct _GvStationWindow {
	/* Parent instance structure */
	GtkWindow               parent_instance;
	/* Private data */
	GvStationWindowPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStationWindow, gv_station_window, GTK_TYPE_WINDOW)

/*
 * Private methods
 */

static void
gv_station_window_fill_from_station(GvStationWindow *self, GvStation *station)
{
	GvStationWindowPrivate *priv = self->priv;
	GtkEntry *uri_entry = GTK_ENTRY(priv->manual_uri_entry);
	const gchar *uri;

	if (station == NULL)
		return;

	uri = gv_station_get_uri(station);
	if (uri == NULL)
		return;

	gtk_entry_set_text(uri_entry, uri);
}

static GtkWidget *
gv_station_window_new(void)
{
	return g_object_new(GV_TYPE_STATION_WINDOW, NULL);
}

/*
 * Helpers
 */

static void
remove_weird_chars(const gchar *text, gchar **out, guint *out_len)
{
	gchar *start, *ptr;
	guint length, i;

	length = strlen(text);
	start = g_malloc(length + 1);

	for (i = 0, ptr = start; i < length; i++) {
		/* Discard every character below space */
		if (text[i] > ' ')
			*ptr++ = text[i];
	}

	*ptr = '\0';

	if (length != ptr - start) {
		length = ptr - start;
		start = g_realloc(start, length);
	}

	*out = start;
	*out_len = length;
}

/*
 * Signal handlers - window
 */

static void
on_notebook_switch_page(GtkNotebook     *notebook G_GNUC_UNUSED,
                        GtkWidget       *page G_GNUC_UNUSED,
                        guint            page_num,
                        GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	/* The button box actually belongs to the page 'Add Manually' */
	if (page_num == 1)
		gtk_widget_set_visible(priv->manual_save_button, TRUE);
	else
		gtk_widget_set_visible(priv->manual_save_button, FALSE);
}

/*
 * Private methods
 */

static void
gv_station_window_stop_search_spinner_now(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	GtkSpinner *spinner = GTK_SPINNER(priv->search_status_spinner);

	/* If there's a start operation pending, remove it */
	if (priv->start_search_spinner_source_id) {
		g_source_remove(priv->start_search_spinner_source_id);
		priv->start_search_spinner_source_id = 0;
	}

	/* Stop spinner in case it's running */
	gtk_spinner_stop(spinner);
}

static void
gv_station_window_start_search_spinner_now(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	GtkSpinner *spinner = GTK_SPINNER(priv->search_status_spinner);

	/* Start spinner */
	gtk_spinner_start(spinner);
}

static gboolean
when_timeout_start_search_spinner(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	gv_station_window_start_search_spinner_now(self);

	priv->start_search_spinner_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_station_window_start_search_spinner_delayed(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	gv_station_window_stop_search_spinner_now(self);

	/* As for the delay before displaying the spinner, we follow GNOME guidelines:
	 * https://developer.gnome.org/hig/stable/progress-spinners.html.en
	 */
	priv->start_search_spinner_source_id =
	        g_timeout_add_seconds(3, (GSourceFunc) when_timeout_start_search_spinner, self);
}

/*
 * Signal handlers - 'Search' part
 */

static void
on_search_result_tree_view_map(GvSearchResultTreeView *search_result_tree_view,
                               GvStationWindow *self)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(search_result_tree_view);
	GtkWindow *window = GTK_WINDOW(self);
	gint tv_natural_height, tv_optimal_height;
	gint diff;
	gint win_width, win_height, win_new_height;

	/* This is an attempt to smartly set the window height.
	 * All this crap could be done better from GTK+ 3.22, using additional
	 * GtkScrolledWindow methods (well, I think).
	 */

	/* Fiddle with tree view height */
	gtk_widget_get_preferred_height(GTK_WIDGET(tree_view), NULL, &tv_natural_height);
	tv_optimal_height = gv_search_result_tree_view_get_optimal_height
	                    (GV_SEARCH_RESULT_TREE_VIEW(tree_view));

	diff = tv_optimal_height - tv_natural_height;
	if (diff < 0)
		return;

	/* Fiddle with window height */
	gtk_window_get_size(window, &win_width, &win_height);
	win_new_height = win_height + diff;
	if (win_new_height > 600)
		win_new_height = 600;

	/* Resize */
	gtk_window_resize(window, win_width, win_new_height);
}

static void
when_search_returned(GvBrowser       *browser,
                     GAsyncResult    *result,
                     GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	GList *details_list;
	GError *error = NULL;

	DEBUG("Finished");

	/* Stop the spinner */
	gv_station_window_stop_search_spinner_now(self);

	/* Finish the search */
	details_list = gv_browser_search_finish(browser, result, &error);
	if (error) {
		gtk_label_set_text(GTK_LABEL(priv->search_status_label),
		                   error->message);
		return;
	}

	if (details_list == NULL) {
		gtk_label_set_text(GTK_LABEL(priv->search_status_label),
		                   _("No result"));
		return;
	}

	/* Ok */
	gtk_label_set_text(GTK_LABEL(priv->search_status_label), _("Results"));

	/* Create and display tree view */
	GtkWidget *result_tree_view;
	result_tree_view = gv_search_result_tree_view_new();

	gv_search_result_tree_view_populate(GV_SEARCH_RESULT_TREE_VIEW(result_tree_view),
	                                    details_list);

	/* When the tree view will be effectively displayed, we will want to resize it */
	g_signal_connect_object(result_tree_view,
	                        "map", G_CALLBACK(on_search_result_tree_view_map),
	                        self, 0);

	gtk_container_add(GTK_CONTAINER(priv->search_scrolled_window), result_tree_view);

	/* Show */
	DEBUG("Showing all");
	gtk_widget_show_all(GTK_WIDGET(priv->search_vbox));
}

static void
gv_station_window_start_new_search(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	GtkEntry *search_entry = GTK_ENTRY(priv->search_entry);
	const gchar *station_name;

	/* If there's no station name for some reason, bail out */
	station_name = gtk_entry_get_text(search_entry);
	if (station_name == NULL || !g_strcmp0(station_name, ""))
		return;

	/* In case a previous search happened, empty the result area */

	// TODO
	// Probably we also need to free the details associated

	GList *children, *item;

	children = gtk_container_get_children(GTK_CONTAINER(priv->search_scrolled_window));
	for (item = children; item != NULL; item = item->next)
		gtk_widget_destroy(GTK_WIDGET(item->data));
	g_list_free(children);

	/* Start searching */
	gv_station_window_start_search_spinner_delayed(self);
	gtk_label_set_text(GTK_LABEL(priv->search_status_label),
	                   _("Searching, please be patient..."));

	gv_browser_search_async(gv_core_browser, station_name,
	                        (GAsyncReadyCallback) when_search_returned, self);
}

static void
on_search_button_clicked(GtkButton *button G_GNUC_UNUSED,
                         GvStationWindow *self)
{
	gv_station_window_start_new_search(self);
}

static void
on_search_entry_activate(GtkEntry *entry G_GNUC_UNUSED,
                         GvStationWindow *self)
{
	gv_station_window_start_new_search(self);
}

static void
on_search_entry_changed(GtkEditable *editable,
                        GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	guint text_len;

	text_len = gtk_entry_get_text_length(GTK_ENTRY(editable));

	/* If the entry is empty, the search button is not clickable */
	if (text_len > 0)
		gtk_widget_set_sensitive(priv->search_button, TRUE);
	else
		gtk_widget_set_sensitive(priv->search_button, FALSE);
}

/*
 * Signal handlers - 'Add Manually' part
 */

static void
on_manual_button_clicked(GtkButton *button,
                         GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	if ((GtkWidget *) button == priv->manual_save_button) {
		GtkEntry *name_entry = GTK_ENTRY(priv->manual_name_entry);
		GtkEntry *uri_entry = GTK_ENTRY(priv->manual_uri_entry);
		const gchar *name, *uri;
		GvStation *station;

		name = gtk_entry_get_text(name_entry);
		uri = gtk_entry_get_text(uri_entry);
		station = gv_station_new(name, uri);

		/* We take ownership, so that we can unref when we're finalized */
		priv->new_station = g_object_ref_sink(station);
	}

	gtk_window_close(GTK_WINDOW(self));
}

static void
on_manual_uri_entry_insert_text(GtkEditable *editable,
                                gchar       *text,
                                gint         length G_GNUC_UNUSED,
                                gpointer     position,
                                GvStationWindow *self)
{
	gchar *new_text;
	guint new_len;

	/* Remove weird characters */
	remove_weird_chars(text, &new_text, &new_len);

	/* Replace text in entry */
	g_signal_handlers_block_by_func(editable,
	                                on_manual_uri_entry_insert_text,
	                                self);

	gtk_editable_insert_text(editable, new_text, new_len, position);

	g_signal_handlers_unblock_by_func(editable,
	                                  on_manual_uri_entry_insert_text,
	                                  self);

	/* We inserted the text in the entry, so now we just need to
	 * prevent the default handler to run.
	 */
	g_signal_stop_emission_by_name(editable, "insert-text");

	/* Free */
	g_free(new_text);
}

static void
on_manual_uri_entry_changed(GtkEditable *editable,
                            GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	guint text_len;

	text_len = gtk_entry_get_text_length(GTK_ENTRY(editable));

	/* If the entry is empty, the save button is not clickable */
	if (text_len > 0)
		gtk_widget_set_sensitive(priv->manual_save_button, TRUE);
	else
		gtk_widget_set_sensitive(priv->manual_save_button, FALSE);
}

/*
 * Property accessors
 */

static GvStation *
gv_station_window_get_new_station(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	return priv->new_station;
}

static void
gv_station_window_get_property(GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	GvStationWindow *self = GV_STATION_WINDOW(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NEW_STATION:
		g_value_set_object(value, gv_station_window_get_new_station(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_station_window_set_property(GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

/*
 * Public methods
 */


/*
 * Construct helpers
 */

static void
gv_station_window_populate_widgets(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;
	GtkBuilder *builder;
	gchar *uifile;

	/* Build the ui */
	gv_builder_load(UI_FILE, &builder, &uifile);
	DEBUG("Built from ui file '%s'", uifile);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_vbox);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, notebook);

	/* Search */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_vbox);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_entry);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_status_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_status_spinner);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_title_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, search_scrolled_window);

	/* Manual */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, manual_vbox);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, manual_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, manual_name_entry);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, manual_uri_entry);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, manual_save_button);

	/* Pack that within the window */
	gtk_container_add(GTK_CONTAINER(self), priv->window_vbox);

	/* Cleanup */
	g_object_unref(G_OBJECT(builder));
	g_free(uifile);
}

static void
gv_station_window_setup_widgets(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	/*
	 * Main window
	 */

	gtk_widget_set_visible(priv->manual_save_button, FALSE);
	g_signal_connect_object(priv->notebook,
	                        "switch-page", G_CALLBACK(on_notebook_switch_page),
	                        self, 0);


	/*
	 * Search
	 */

	/* Set the title */
	// TODO: take url from browser object
	gtk_label_set_markup(GTK_LABEL(priv->search_title_label),
	                     "Enter a station name to search on "
	                     "<a href=\"http://www.radio-browser.info\">"
	                     "Community Radio Browser"
	                     "</a>.");

	/* Give focus to the entry */
	gtk_widget_grab_focus(priv->search_entry);

	/* Search button is insensitive as long as search entry is empty */
	gtk_widget_set_sensitive(priv->search_button, FALSE);
	g_signal_connect_object(priv->search_entry,
	                        "changed", G_CALLBACK(on_search_entry_changed),
	                        self, 0);

	/* Enter pressed */
	g_signal_connect_object(priv->search_entry,
	                        "activate", G_CALLBACK(on_search_entry_activate),
	                        self, 0);

	/* Button clicked */
	g_signal_connect_object(priv->search_button,
	                        "clicked", G_CALLBACK(on_search_button_clicked),
	                        self, 0);

	/* Scrolled window config */
	gtk_widget_set_vexpand(priv->search_scrolled_window, TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(priv->search_scrolled_window),
	                               GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);


	/*
	 * Manual
	 */

	GtkEntry *uri_entry = GTK_ENTRY(priv->manual_uri_entry);


	/* Save button is insensitive as long as the uri entry is empty */
	gtk_widget_set_sensitive(priv->manual_save_button, FALSE);
	g_signal_connect_object(uri_entry,
	                        "changed", G_CALLBACK(on_manual_uri_entry_changed),
	                        self, 0);

	/* Ensure there's no junk in the characters entered */
	gtk_entry_set_input_purpose(uri_entry, GTK_INPUT_PURPOSE_URL);
	g_signal_connect_object(uri_entry,
	                        "insert-text", G_CALLBACK(on_manual_uri_entry_insert_text),
	                        self, 0);

	/* Buttons signal handlers */
	g_signal_connect_object(priv->manual_save_button,
	                        "clicked", G_CALLBACK(on_manual_button_clicked),
	                        self, 0);
}

static void
gv_station_window_setup_appearance(GvStationWindow *self)
{
	GvStationWindowPrivate *priv = self->priv;

	/* Main window */
	g_object_set(priv->window_vbox,
	             "margin", 0,
	             "spacing", 0,
	             NULL);

	gtk_window_set_default_size(GTK_WINDOW(self), 400, -1);

	/* Search */
	g_object_set(priv->search_vbox,
	             "margin", GV_UI_WINDOW_BORDER,
	             "spacing", GV_UI_GROUP_SPACING,
	             NULL);

	g_object_set(priv->search_grid,
//	             "margin", GV_UI_WINDOW_BORDER,
	             "row-spacing", GV_UI_ELEM_SPACING,
	             "column-spacing", GV_UI_COLUMN_SPACING,
	             NULL);

	/* Manual */
	g_object_set(priv->manual_vbox,
	             "margin", GV_UI_WINDOW_BORDER,
	             "spacing", GV_UI_GROUP_SPACING,
	             NULL);

	g_object_set(priv->manual_grid,
//	             "margin", GV_UI_WINDOW_BORDER,
	             "row-spacing", GV_UI_ELEM_SPACING,
	             "column-spacing", GV_UI_COLUMN_SPACING,
	             NULL);
}

/*
 * GObject methods
 */

static void
gv_station_window_finalize(GObject *object)
{
	GvStationWindow *self = GV_STATION_WINDOW(object);
	GvStationWindowPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Remove pending operations */
	if (priv->start_search_spinner_source_id)
		g_source_remove(priv->start_search_spinner_source_id);

	/* Unref station */
	if (priv->new_station)
		g_object_unref(priv->new_station);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station_window, object);
}

static void
gv_station_window_constructed(GObject *object)
{
	GvStationWindow *self = GV_STATION_WINDOW(object);

	TRACE("%p", self);

	/* Build window */
	gv_station_window_populate_widgets(self);
	gv_station_window_setup_widgets(self);
	gv_station_window_setup_appearance(self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station_window, object);
}

static void
gv_station_window_init(GvStationWindow *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_window_get_instance_private(self);
}

static void
gv_station_window_class_init(GvStationWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_window_finalize;
	object_class->constructed = gv_station_window_constructed;

	/* Properties */
	object_class->get_property = gv_station_window_get_property;
	object_class->set_property = gv_station_window_set_property;

	properties[PROP_NEW_STATION] =
	        g_param_spec_object("new-station", "New station", NULL,
	                            GV_TYPE_STATION,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}

/*
 * Convenience functions
 */

static GtkWidget *station_window;
static GvStation *insert_after_station;

static gboolean
on_window_delete_event(GtkWidget *widget,
                       GdkEvent  *event G_GNUC_UNUSED,
                       gpointer   user_data G_GNUC_UNUSED)
{
	GvStationList *station_list = gv_core_station_list;
	GvStation *new_station;

	/* There can be only one station window */
	g_assert(widget == station_window);

	/* Check if a station was created */
	new_station = gv_station_window_get_new_station(GV_STATION_WINDOW(station_window));
	if (new_station == NULL)
		goto cleanup;

	/* Add to station list */
	if (insert_after_station)
		gv_station_list_insert_after(station_list, new_station, insert_after_station);
	else
		gv_station_list_append(station_list, new_station);

cleanup:
	/* Destroy window */
	gtk_widget_destroy(station_window);
	station_window = NULL;

	/* Remove weak pointer if any */
	if (insert_after_station) {
		g_object_remove_weak_pointer(G_OBJECT(insert_after_station),
		                             (gpointer *) &insert_after_station);
		insert_after_station = NULL;
	}

	return TRUE;
}

void
gv_show_station_window(GtkWindow *parent, GvStation *next_to_station)
{
	GtkWindow *window;

	/* There can be only one station window */
	if (station_window) {
		window = GTK_WINDOW(station_window);
		gtk_window_present(window);

		return;
	}

	/* Create a new station window */
	station_window = gv_station_window_new();
	window = GTK_WINDOW(station_window);

	gtk_window_set_title              (window, _("Add New Station"));
	gtk_window_set_modal              (window, TRUE);
	gtk_window_set_skip_taskbar_hint  (window, TRUE);
	gtk_window_set_transient_for      (window, parent);
	gtk_window_set_destroy_with_parent(window, TRUE);

	/* In status icon mode, 'Add Station' is invoked from a popup menu,
	 * while the main window is not visible. In this case we want the
	 * window to appear close to the mouse pointer.
	 */
	if (!gtk_widget_is_visible(GTK_WIDGET(parent)))
		gtk_window_set_position(window, GTK_WIN_POS_MOUSE);

	/* Save the station after which we might want to create a new station */
	g_assert_null(insert_after_station);
	if (next_to_station)
		g_object_add_weak_pointer(G_OBJECT(next_to_station),
		                          (gpointer *) &insert_after_station);

	/* If the application was started with an uri in argument, it's possible that
	 * the current station is not part of the station list. In such case, we assume
	 * that the user intends to add this station to the list, so we save him a bit
	 * of time and populate the window with this uri.
	 */
	GvPlayer      *player       = gv_core_player;
	GvStationList *station_list = gv_core_station_list;
	GvStation     *current_station;

	current_station = gv_player_get_station(player);
	if (current_station &&
	    gv_station_list_find(station_list, current_station) == NULL) {
		gv_station_window_fill_from_station(GV_STATION_WINDOW(station_window),
		                                    current_station);
	}

	/* Connect signal handlers */
	g_signal_connect(station_window, "delete-event",
	                 G_CALLBACK(on_window_delete_event), NULL);

	/* Show the window */
	gtk_widget_show(station_window);
}
