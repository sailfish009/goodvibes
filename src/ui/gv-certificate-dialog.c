/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023-2024 Arnaud Rebillout
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

#if !GLIB_CHECK_VERSION(2, 74, 0)
#define G_TLS_CERTIFICATE_NO_FLAGS 0
#endif

#define PLAYLIST_URL_ROW 0
#define PLAYLIST_URL_REDIRECTION_ROW 1
#define STREAM_URL_ROW 2
#define STREAM_URL_REDIRECTION_ROW 3
#define TLS_ERRORS_ROW 4

/*
 * Signals
 */

enum {
	SIGNAL_RESPONSE,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvCertificateDialogPrivate {
	GtkWidget *dialog;
};

typedef struct _GvCertificateDialogPrivate GvCertificateDialogPrivate;

struct _GvCertificateDialog {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvCertificateDialogPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvCertificateDialog, gv_certificate_dialog, G_TYPE_OBJECT)

/*
 * Helpers
 */

gchar *
make_tls_errors_description(GTlsCertificateFlags errors)
{
	GPtrArray *a;
	gchar *res;

	a = g_ptr_array_new_full(2, NULL);

	if (errors & G_TLS_CERTIFICATE_UNKNOWN_CA)
		g_ptr_array_add(a, "unknown certificate authority");
	if (errors & G_TLS_CERTIFICATE_BAD_IDENTITY)
		g_ptr_array_add(a, "bad identity");
	if (errors & G_TLS_CERTIFICATE_NOT_ACTIVATED)
		g_ptr_array_add(a, "not yet activated");
	if (errors & G_TLS_CERTIFICATE_EXPIRED)
		g_ptr_array_add(a, "expired");
	if (errors & G_TLS_CERTIFICATE_REVOKED)
		g_ptr_array_add(a, "revoked");
	if (errors & G_TLS_CERTIFICATE_INSECURE)
		g_ptr_array_add(a, "insecure algorithm");

	if (a->len > 0) {
		g_ptr_array_add(a, NULL);
		res = g_strjoinv(", ", (gchar **) a->pdata);
	} else
		res = g_strdup("unknown error");

	g_ptr_array_free(a, TRUE);

	return res;
}

static void
add_row(GtkGrid *grid, guint row, const gchar *title)
{
	GtkWidget *label;
	GtkStyleContext *style_context;

	label = gtk_label_new(title);
	gtk_label_set_xalign(GTK_LABEL(label), 1);
	style_context = gtk_widget_get_style_context(label);
	gtk_style_context_add_class(style_context, "dim-label");
	gtk_widget_set_visible(label, FALSE);
	gtk_grid_attach(grid, label, 0, row, 1, 1);

	label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_widget_set_visible(label, FALSE);
	gtk_grid_attach(grid, label, 1, row, 1, 1);
}

static void
set_row(GtkGrid *grid, guint row, const gchar *text)
{
	GtkWidget *title_label, *value_label;
	gboolean visible = (text != NULL);

	title_label = gtk_grid_get_child_at(grid, 0, row);
	value_label = gtk_grid_get_child_at(grid, 1, row);

	gtk_label_set_text(GTK_LABEL(value_label), text);
	gtk_widget_set_visible(title_label, visible);
	gtk_widget_set_visible(value_label, visible);
}

static void
update_grid(GtkGrid *grid, GvPlayback *playback, GTlsCertificateFlags tls_errors)
{
	const gchar *text;

	text = gv_playback_get_playlist_uri(playback);
	set_row(grid, PLAYLIST_URL_ROW, text);

	text = gv_playback_get_playlist_redirection_uri(playback);
	set_row(grid, PLAYLIST_URL_REDIRECTION_ROW, text);

	text = gv_playback_get_stream_uri(playback);
	set_row(grid, STREAM_URL_ROW, text);

	text = gv_playback_get_stream_redirection_uri(playback);
	set_row(grid, STREAM_URL_REDIRECTION_ROW, text);

	/* If tls_errors is G_TLS_CERTIFICATE_NO_FLAGS, it doesn't mean that
	 * there's no error, it just means that we must leave the errors as
	 * is (we're here to update other values).
	 */
	if (tls_errors != G_TLS_CERTIFICATE_NO_FLAGS) {
		gchar *errors;
		errors = make_tls_errors_description(tls_errors);
		set_row(grid, TLS_ERRORS_ROW, errors);
		g_free(errors);
	}
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
update_dialog(GtkWidget *dialog, GvPlayback *playback, GTlsCertificateFlags tls_errors)
{
	GtkMessageDialog *message_dialog;
	GtkWidget *message_area;
	GtkWidget *grid;

	/* Get last child in message area */
	message_dialog = GTK_MESSAGE_DIALOG(dialog);
	message_area = gtk_message_dialog_get_message_area(message_dialog);
	grid = get_last_child(GTK_CONTAINER(message_area));

	/* Update it */
	update_grid(GTK_GRID(grid), playback, tls_errors);
}

/*
 * Signal handlers & callbacks
 */

static void
on_dialog_response(GtkWidget* dialog G_GNUC_UNUSED, gint response_id, GvCertificateDialog *self)
{
	g_signal_emit(self, signals[SIGNAL_RESPONSE], 0, response_id);
}

static void
on_playback_notify(GvPlayback *playback, GParamSpec *pspec, GvCertificateDialog *self)
{
	GvCertificateDialogPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", playback, property_name, self);

	if (!g_strcmp0(property_name, "playlist-uri") ||
	    !g_strcmp0(property_name, "playlist-redirection-uri") ||
	    !g_strcmp0(property_name, "stream-uri") ||
	    !g_strcmp0(property_name, "stream-redirection-uri"))
		update_dialog(priv->dialog, playback, G_TLS_CERTIFICATE_NO_FLAGS);
}

/*
 * Public methods
 */

void
gv_certificate_dialog_show(GvCertificateDialog *self)
{
	GvCertificateDialogPrivate *priv = self->priv;

	gtk_widget_show(priv->dialog);
}

void
gv_certificate_dialog_update(GvCertificateDialog *self, GvPlayback *playback,
		GTlsCertificateFlags tls_errors)
{
	GvCertificateDialogPrivate *priv = self->priv;

	g_return_if_fail(self != NULL);
	g_return_if_fail(playback != NULL);

	update_dialog(priv->dialog, playback, tls_errors);
}

GvCertificateDialog *
gv_certificate_dialog_new(void)
{
	return g_object_new(GV_TYPE_CERTIFICATE_DIALOG, NULL);
}

/*
 * GObject methods
 */

static void
gv_certificate_dialog_finalize(GObject *object)
{
	GvCertificateDialog *self = GV_CERTIFICATE_DIALOG(object);
	GvCertificateDialogPrivate *priv = self->priv;

	TRACE("%p", object);

	if (priv->dialog != NULL)
		gtk_widget_destroy(priv->dialog);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_certificate_dialog, object);
}

static void
gv_certificate_dialog_constructed(GObject *object)
{
	GvCertificateDialog *self = GV_CERTIFICATE_DIALOG(object);
	GvCertificateDialogPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Initialize properties */
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_certificate_dialog, object);
}

static void
gv_certificate_dialog_init(GvCertificateDialog *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_certificate_dialog_get_instance_private(self);
}

static void
gv_certificate_dialog_class_init(GvCertificateDialogClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_certificate_dialog_finalize;
	object_class->constructed = gv_certificate_dialog_constructed;

	/* Signals */
	signals[SIGNAL_RESPONSE] =
		g_signal_new("response", G_OBJECT_CLASS_TYPE(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_INT);
}

/*
 * Convenience functions
 */

static GtkWidget *
make_grid(void)
{
	GtkWidget *widget;
	GtkGrid *grid;

	widget = gtk_grid_new();

	grid = GTK_GRID(widget);
	add_row(grid, PLAYLIST_URL_ROW, _("Playlist URL"));
	add_row(grid, PLAYLIST_URL_REDIRECTION_ROW, _("Redirection"));
	add_row(grid, STREAM_URL_ROW, _("Stream URL"));
	add_row(grid, STREAM_URL_REDIRECTION_ROW, _("Redirection"));
	add_row(grid, TLS_ERRORS_ROW, _("TLS Errors"));

	g_object_set(widget, "row-spacing", GV_UI_ELEM_SPACING,
			"column-spacing", GV_UI_COLUMN_SPACING,
			NULL);

	return widget;
}

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

	widget = make_grid();
	gtk_container_add(GTK_CONTAINER(message_area), widget);

	gtk_widget_show_all(message_area);

	return dialog;
}

GvCertificateDialog *
gv_make_certificate_dialog(GtkWindow *parent, GvPlayback *playback)
{
	GtkWidget *dialog;
	GvCertificateDialog *self;

	dialog = make_dialog(parent);
	self = gv_certificate_dialog_new();
	self->priv->dialog = dialog;

	/* Connect to 'response' so that we can forward it */
	g_signal_connect_object(dialog, "response",
			G_CALLBACK(on_dialog_response), self, 0);

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
			G_CALLBACK(on_playback_notify), self, 0);

	return self;
}
