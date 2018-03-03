/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
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

#include "ui/gv-station-dialog.h"

#define UI_FILE "station-dialog.glade"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_STATION,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvStationDialogPrivate {
	/* Widgets */
	/* Top-level */
	GtkWidget *main_grid;
	/* Entries */
	GtkWidget *name_entry;
	GtkWidget *uri_entry;
	/* Buttons */
	GtkWidget *save_button;
	/* Existing station if any */
	GvStation *station;
};

typedef struct _GvStationDialogPrivate GvStationDialogPrivate;

struct _GvStationDialog {
	/* Parent instance structure */
	GtkDialog               parent_instance;
	/* Private data */
	GvStationDialogPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStationDialog, gv_station_dialog, GTK_TYPE_DIALOG)

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
 * Signal handlers
 */

static void
on_uri_entry_insert_text(GtkEditable *editable,
                         gchar       *text,
                         gint         length G_GNUC_UNUSED,
                         gpointer     position,
                         GvStationDialog *self)
{
	gchar *new_text;
	guint new_len;

	/* Remove weird characters */
	remove_weird_chars(text, &new_text, &new_len);

	/* Replace text in entry */
	g_signal_handlers_block_by_func(editable,
	                                on_uri_entry_insert_text,
	                                self);

	gtk_editable_insert_text(editable, new_text, new_len, position);

	g_signal_handlers_unblock_by_func(editable,
	                                  on_uri_entry_insert_text,
	                                  self);

	/* We inserted the text in the entry, so now we just need to
	 * prevent the default handler to run.
	 */
	g_signal_stop_emission_by_name(editable, "insert-text");

	/* Free */
	g_free(new_text);
}

static void
on_uri_entry_changed(GtkEditable *editable,
                     GvStationDialog *self)
{
	GvStationDialogPrivate *priv = self->priv;
	guint text_len;

	text_len = gtk_entry_get_text_length(GTK_ENTRY(editable));

	/* If the entry is empty, the save button is not clickable */
	if (text_len > 0)
		gtk_widget_set_sensitive(priv->save_button, TRUE);
	else
		gtk_widget_set_sensitive(priv->save_button, FALSE);
}

/*
 * Property accessors
 */

static void
gv_station_dialog_set_station(GvStationDialog *self, GvStation *station)
{
	GvStationDialogPrivate *priv = self->priv;

	/* This is a construct-only property - NULL is allowed */
	g_assert_null(priv->station);
	g_set_object(&priv->station, station);
}

static void
gv_station_dialog_get_property(GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void
gv_station_dialog_set_property(GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	GvStationDialog *self = GV_STATION_DIALOG(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_STATION:
		gv_station_dialog_set_station(self, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

void
gv_station_dialog_retrieve(GvStationDialog *self, GvStation *station)
{
	GvStationDialogPrivate *priv = self->priv;
	GtkEntry *name_entry = GTK_ENTRY(priv->name_entry);
	GtkEntry *uri_entry = GTK_ENTRY(priv->uri_entry);

	gv_station_set_name(station, gtk_entry_get_text(name_entry));
	gv_station_set_uri(station, gtk_entry_get_text(uri_entry));
}

GvStation *
gv_station_dialog_create(GvStationDialog *self)
{
	GvStationDialogPrivate *priv = self->priv;
	GtkEntry *name_entry = GTK_ENTRY(priv->name_entry);
	GtkEntry *uri_entry = GTK_ENTRY(priv->uri_entry);
	const gchar *name, *uri;

	name = gtk_entry_get_text(name_entry);
	uri = gtk_entry_get_text(uri_entry);

	return gv_station_new(name, uri);
}

void
gv_station_dialog_fill_uri(GvStationDialog *self, const gchar *uri)
{
	GvStationDialogPrivate *priv = self->priv;
	GtkEntry *uri_entry = GTK_ENTRY(priv->uri_entry);

	gtk_entry_set_text(uri_entry, uri);
}

GtkWidget *
gv_station_dialog_new(GvStation *station)
{
	return g_object_new(GV_TYPE_STATION_DIALOG, "station", station, NULL);
}

/*
 * Construct helpers
 */

static void
gv_station_dialog_populate_widgets(GvStationDialog *self)
{
	GvStationDialogPrivate *priv = self->priv;
	GtkWidget *content_area;
	GtkBuilder *builder;
	gchar *uifile;

	/* Build the ui */
	gv_builder_load(UI_FILE, &builder, &uifile);
	DEBUG("Built from ui file '%s'", uifile);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, main_grid);

	/* Text entries */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, name_entry);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, uri_entry);

	/* Add to content area */
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(self));
	gtk_container_add(GTK_CONTAINER(content_area), priv->main_grid);

	/* Populate the action area */
	gtk_dialog_add_buttons(GTK_DIALOG(self),
	                       _("Cancel"), GTK_RESPONSE_CANCEL,
	                       _("Save"), GTK_RESPONSE_OK,
	                       NULL);
	priv->save_button = gtk_dialog_get_widget_for_response
	                    (GTK_DIALOG(self), GTK_RESPONSE_OK);

	/* Cleanup */
	g_object_unref(G_OBJECT(builder));
	g_free(uifile);
}

static void
gv_station_dialog_setup_widgets(GvStationDialog *self)
{
	GvStationDialogPrivate *priv = self->priv;
	GtkEntry *name_entry = GTK_ENTRY(priv->name_entry);
	GtkEntry *uri_entry = GTK_ENTRY(priv->uri_entry);
	GvStation *station = priv->station;

	/* We don't allow creating a station without an empty uri, therefore
	 * the save button is insensitive when the uri empty. We can set it
	 * insensitive now because of the following code...
	 * Watch out if you change something here !
	 */
	gtk_widget_set_sensitive(priv->save_button, FALSE);

	/* Configure uri widget */
	gtk_entry_set_input_purpose(uri_entry, GTK_INPUT_PURPOSE_URL);
	g_signal_connect_object(uri_entry, "insert-text", G_CALLBACK(on_uri_entry_insert_text),
	                        self, 0);
	g_signal_connect_object(uri_entry, "changed", G_CALLBACK(on_uri_entry_changed),
	                        self, 0);

	/* Fill widgets with station info */
	if (station) {
		const gchar *name, *uri;

		name = gv_station_get_name(station);
		if (name)
			gtk_entry_set_text(name_entry, name);

		uri = gv_station_get_uri(station);
		if (uri)
			gtk_entry_set_text(uri_entry, uri);
	}
}

static void
gv_station_dialog_setup_appearance(GvStationDialog *self)
{
	GvStationDialogPrivate *priv = self->priv;

	/* Main window */
	gtk_window_set_default_size(GTK_WINDOW(self), 400, -1);

	/* Main grid */
	g_object_set(priv->main_grid,
	             "margin", GV_UI_WINDOW_BORDER,
	             "row-spacing", GV_UI_ELEM_SPACING,
	             "column-spacing", GV_UI_COLUMN_SPACING,
	             NULL);
}

/*
 * GObject methods
 */

static void
gv_station_dialog_finalize(GObject *object)
{
	GvStationDialog *self = GV_STATION_DIALOG(object);
	GvStationDialogPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Unref station */
	if (priv->station)
		g_object_unref(priv->station);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station_dialog, object);
}

static void
gv_station_dialog_constructed(GObject *object)
{
	GvStationDialog *self = GV_STATION_DIALOG(object);

	/* Build window */
	gv_station_dialog_populate_widgets(self);
	gv_station_dialog_setup_widgets(self);
	gv_station_dialog_setup_appearance(self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station_dialog, object);
}

static void
gv_station_dialog_init(GvStationDialog *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_dialog_get_instance_private(self);
}

static void
gv_station_dialog_class_init(GvStationDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_dialog_finalize;
	object_class->constructed = gv_station_dialog_constructed;

	/* Properties */
	object_class->get_property = gv_station_dialog_get_property;
	object_class->set_property = gv_station_dialog_set_property;

	properties[PROP_STATION] =
	        g_param_spec_object("station", "Station", NULL, GV_TYPE_STATION,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_WRITABLE |
	                            G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(object_class, PROP_N, properties);
}

/*
 * Convenience functions
 */

static GtkWidget *
make_station_dialog(GtkWindow *parent, GvStation *station)
{
	GtkWidget *dialog;

	dialog = gv_station_dialog_new(station);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(dialog), station ?
	                     _("Edit Station") : _("Add Station"));

	/* When 'Add Station' is invoked from the popup menu, in status icon mode,
	 * we want a better position than centering on screen.
	 */
	if (!gtk_widget_is_visible(GTK_WIDGET(parent)))
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	return dialog;
}

void
gv_show_edit_station_dialog(GtkWindow *parent, GvStation *station)
{
	GtkWidget *dialog;
	gint response;

	/* Create and configure the dialog */
	dialog = make_station_dialog(parent, station);

	/* Run */
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_OK)
		gv_station_dialog_retrieve(GV_STATION_DIALOG(dialog), station);

	/* Cleanup */
	gtk_widget_destroy(dialog);
}

GvStation *
gv_show_add_station_dialog(GtkWindow *parent)
{
	GvStation *station;
	GtkWidget *dialog;
	gint response;

	/* Create and configure the dialog */
	dialog = make_station_dialog(parent, NULL);

	/* When we're asked to display an empty station dialog, we play a little trick.
	 * If the application was started with an uri in argument, it's possible that
	 * the current station is not part of the station list. In such case, we assume
	 * that the user intends to add this station to the list, so we save him a bit
	 * of time and populate the dialog with this uri.
	 */
	GvPlayer      *player       = gv_core_player;
	GvStationList *station_list = gv_core_station_list;
	GvStation     *current_station;

	current_station = gv_player_get_station(player);
	if (current_station &&
	    gv_station_list_find(station_list, current_station) == NULL) {
		gv_station_dialog_fill_uri(GV_STATION_DIALOG(dialog),
		                           gv_station_get_uri(current_station));
	}

	/* Run */
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_OK)
		station = gv_station_dialog_create(GV_STATION_DIALOG(dialog));
	else
		station = NULL;

	/* Cleanup */
	gtk_widget_destroy(dialog);

	return station;
}
