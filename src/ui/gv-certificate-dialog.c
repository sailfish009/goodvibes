/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023 Arnaud Rebillout
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
#include "ui/gv-ui-internal.h"

#include "ui/gv-certificate-dialog.h"

/*
 * Helpers
 */

static void
add_row(GtkGrid *grid, guint row, const gchar *title, const gchar *value)
{
	GtkWidget *label;
	GtkStyleContext *style_context;

	label = gtk_label_new(title);
	gtk_label_set_xalign(GTK_LABEL(label), 1);
	style_context = gtk_widget_get_style_context(label);
	gtk_style_context_add_class(style_context, "dim-label");
	gtk_grid_attach(grid, label, 0, row, 1, 1);

	label = gtk_label_new(value);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_grid_attach(grid, label, 1, row, 1, 1);
}

static void
fill_grid(GtkGrid *grid, GvStation *station, GvPlayer *player)
{
	GvPlaylist *playlist;
	const gchar *text;
	guint row = 0;

	playlist = gv_station_get_playlist(station);
	if (playlist != NULL) {
		text = gv_playlist_get_uri(playlist);
		add_row(grid, row++, _("Playlist URL"), text);

		text = gv_playlist_get_redirection_uri(playlist);
		if (text != NULL)
			add_row(grid, row++, _("Redirection"), text);
	}

	text = gv_station_get_stream_uri(station);
	if (text != NULL)
		add_row(grid, row++, _("Stream URL"), text);

	text = gv_player_get_redirection_uri(player);
	if (text != NULL)
		add_row(grid, row++, _("Redirection"), text);
}

static GtkWidget *
get_last_child(GtkContainer *container)
{
	GList *children, *last_child;
	GtkWidget *widget;

	children = gtk_container_get_children(container);
	last_child = g_list_last(children);
	if (last_child != NULL)
		widget = GTK_WIDGET(last_child->data);
	else
		widget = NULL;

	g_list_free(children);

	return widget;
}

static void
update_dialog(GtkWidget *dialog, GvStation *station, GvPlayer *player)
{
	GtkMessageDialog *message_dialog;
	GtkWidget *message_area;
	GtkWidget *grid;

	/* Get last child in message area */
	message_dialog = GTK_MESSAGE_DIALOG(dialog);
	message_area = gtk_message_dialog_get_message_area(message_dialog);
	grid = get_last_child(GTK_CONTAINER(message_area));

	/* Destroy it */
	g_assert(GTK_IS_GRID(grid));
	gtk_widget_destroy(grid);

	/* Add new grid */
	grid = gtk_grid_new();
	g_object_set(grid, "row-spacing", GV_UI_ELEM_SPACING,
			"column-spacing", GV_UI_COLUMN_SPACING,
			NULL);

	/* Fill it up */
	if (station != NULL)
		fill_grid(GTK_GRID(grid), station, player);

	/* Pack and leave */
	gtk_container_add(GTK_CONTAINER(message_area), grid);
	gtk_widget_show_all(message_area);
}

/*
 * Signal handlers & callbacks
 */

static void
on_player_notify(GvPlayer *player, GParamSpec *pspec,
		 GtkWidget *dialog)
{
	const gchar *property_name = g_param_spec_get_name(pspec);
	GvStation *station;

	TRACE("%p, %s, %p", player, property_name, dialog);

	if (!g_strcmp0(property_name, "station") ||
	    !g_strcmp0(property_name, "redirection-uri")) {
		station = gv_player_get_station(player);
		update_dialog(dialog, station, player);
	}
}

/*
 * Convenience functions
 */

static GtkWidget *
make_dialog(GtkWindow *parent)
{
	GtkWidget *dialog, *message_area, *widget;
	const gchar *title;
	const gchar *text;

	title = _("Add a Security Exception?");
	text = _("The TLS certificate for this station is not valid. "
			"The issue is most likely a misconfiguration of the website.");

	dialog = gtk_message_dialog_new(parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, title);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), text);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Continue"), GTK_RESPONSE_ACCEPT);

	message_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));

	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add(GTK_CONTAINER(message_area), widget);

	widget = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(message_area), widget);

	gtk_widget_show_all(message_area);

	return dialog;
}

GtkWidget *
gv_make_certificate_dialog(GtkWindow *parent, GvStation *station, GvPlayer *player)
{
	GtkWidget *dialog;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(station != NULL, NULL);
	g_return_val_if_fail(player != NULL, NULL);

	dialog = make_dialog(parent);
	update_dialog(dialog, station, player);

	g_signal_connect_object(player, "notify",
			G_CALLBACK(on_player_notify), dialog, 0);

	return dialog;
}
