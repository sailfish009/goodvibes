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
fill_grid(GtkGrid *grid, GvPlayback *playback)
{
	const gchar *text;
	guint row = 0;

	text = gv_playback_get_playlist_uri(playback);
	if (text != NULL)
		add_row(grid, row++, _("Playlist URL"), text);

	text = gv_playback_get_playlist_redirection_uri(playback);
	if (text != NULL)
		add_row(grid, row++, _("Redirection"), text);

	text = gv_playback_get_stream_uri(playback);
	if (text != NULL)
		add_row(grid, row++, _("Stream URL"), text);

	text = gv_playback_get_stream_redirection_uri(playback);
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
update_dialog(GtkWidget *dialog, GvPlayback *playback)
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
	fill_grid(GTK_GRID(grid), playback);

	/* Pack and leave */
	gtk_container_add(GTK_CONTAINER(message_area), grid);
	gtk_widget_show_all(message_area);
}

/*
 * Signal handlers & callbacks
 */

static void
on_playback_notify(GvPlayback *playback, GParamSpec *pspec, GtkWidget *dialog)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", playback, property_name, dialog);

	if (!g_strcmp0(property_name, "playlist-uri") ||
	    !g_strcmp0(property_name, "playlist-redirection-uri") ||
	    !g_strcmp0(property_name, "stream-uri") ||
	    !g_strcmp0(property_name, "stream-redirection-uri"))
		update_dialog(dialog, playback);
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
gv_make_certificate_dialog(GtkWindow *parent, GvPlayback *playback)
{
	GtkWidget *dialog;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(playback != NULL, NULL);

	dialog = make_dialog(parent);
	update_dialog(dialog, playback);

	/* Connect to notify signals from playback, so that we can update the
	 * dialog if ever it's updated. In practice, when Gstreamer follows a
	 * redirection and then a certificate error happens (all of that
	 * handled by GstSoupHttpSrc), we receive first the certificate error
	 * signal, and then only after we're notified of the redirection.
	 *
	 * It's important to handle this scenario well, especially in case of a
	 * http -> https, otherwise our error message will be of the type
	 * "certificate error for url http://..." and that wouldn't make much
	 * sense.
	 */
	g_signal_connect_object(playback, "notify",
			G_CALLBACK(on_playback_notify), dialog, 0);

	return dialog;
}
