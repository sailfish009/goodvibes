/*
 * Libcaphe
 *
 * Copyright (C) 2016-2018 Arnaud Rebillout
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

#include <stdlib.h>

#include <glib-object.h>
#include <gio/gio.h>

#include "caphe-trace.h"
#include "caphe-inhibitor.h"
#include "caphe-dbus-inhibitor.h"

#include "caphe-inhibitor-list.h"

/* Inhibitors we know of. This list is ordered by priority. */

enum {
	CAPHE_GNOME_SESSION_MANAGER_IDX,
	CAPHE_XFCE_SESSION_MANAGER_IDX,
	CAPHE_XDG_PORTAL_IDX,
	CAPHE_XDG_POWER_MANAGEMENT_IDX,
	CAPHE_XDG_LOGIN1_IDX,
	CAPHE_N_INHIBITORS,
};

static const gchar *caphe_inhibitor_ids[] = {
	"gnome-session-manager",
	"xfce-session-manager",
	"xdg-portal",
	"xdg-power-management",
	"xdg-login1",
	NULL
};

/*
 * Signals
 */

enum {
	SIGNAL_READY,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _CapheInhibitorListPrivate {
	/* List of inhibitors */
	CapheInhibitor *inhibitors[CAPHE_N_INHIBITORS];

	/* For init-time */
	gboolean inhibitors_ready[CAPHE_N_INHIBITORS];
	gboolean ready;
};

typedef struct _CapheInhibitorListPrivate CapheInhibitorListPrivate;

struct _CapheInhibitorList {
	/* Parent instance structure */
	GObject                    parent_instance;
	/* Private data */
	CapheInhibitorListPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(CapheInhibitorList, caphe_inhibitor_list, G_TYPE_OBJECT)

/*
 * Helpers
 */

static gint
get_inhibitor_index(CapheInhibitor *inhibitors[], CapheInhibitor *inhibitor)
{
	gint i;

	for (i = 0; i < CAPHE_N_INHIBITORS; i++) {
		if (inhibitors[i] == inhibitor)
			return i;
	}

	return -1;
}

const gchar *
get_inhibitor_id(CapheInhibitor *inhibitors[], CapheInhibitor *inhibitor)
{
	gint i;

	for (i = 0; i < CAPHE_N_INHIBITORS; i++) {
		if (inhibitors[i] == inhibitor)
			return caphe_inhibitor_ids[i];
	}

	return "unknown";
}

/*
 * Signal handlers
 */

static void
on_inhibitor_notify_available(CapheInhibitor *inhibitor,
                              GParamSpec *pspec G_GNUC_UNUSED,
                              CapheInhibitorList *self)
{
	CapheInhibitorListPrivate *priv = self->priv;
	gboolean available = caphe_inhibitor_get_available(inhibitor);

	/* Log */
	g_debug("Inhibitor '%s' availability changed: %s",
	        get_inhibitor_id(priv->inhibitors, inhibitor),
	        available ? "true" : "false");

	/* We watch this signals only to know when all the inhibitors are ready */
	if (priv->ready == TRUE)
		return;

	gint index;

	/* Update ready list */
	index = get_inhibitor_index(priv->inhibitors, inhibitor);
	priv->inhibitors_ready[index] = TRUE;

	/* Check if every inhibitors are ready */
	for (index = 0; index < CAPHE_N_INHIBITORS; index++) {
		if (priv->inhibitors_ready[index] == FALSE)
			return;
	}

	/* We're ready */
	g_debug("All inhibitors ready");
	priv->ready = TRUE;

	g_signal_emit(self, signals[SIGNAL_READY], 0);
}

/*
 * Public methods
 */

CapheInhibitor **
caphe_inhibitor_list_get_inhibitors(CapheInhibitorList *self)
{
	CapheInhibitorListPrivate *priv = self->priv;

	return priv->inhibitors;
}

CapheInhibitorList *
caphe_inhibitor_list_new(void)
{
	return g_object_new(CAPHE_TYPE_INHIBITOR_LIST, NULL);
}

/*
 * GObject methods
 */

static void
caphe_inhibitor_list_finalize(GObject *object)
{
	CapheInhibitorList *self = CAPHE_INHIBITOR_LIST(object);
	CapheInhibitorListPrivate *priv = self->priv;
	guint i;

	TRACE("%p", self);

	/* Disconnect and unref inhibitors */
	for (i = 0; i < CAPHE_N_INHIBITORS; i++) {
		g_signal_handlers_disconnect_by_data(priv->inhibitors[i], self);
		g_object_unref(priv->inhibitors[i]);
	}

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_inhibitor_list_parent_class)->finalize)
		G_OBJECT_CLASS(caphe_inhibitor_list_parent_class)->finalize(object);
}

static void
caphe_inhibitor_list_constructed(GObject *object)
{
	CapheInhibitorList *self = CAPHE_INHIBITOR_LIST(object);
	CapheInhibitorListPrivate *priv = self->priv;
	guint i;

	TRACE("%p", self);

	/* Create inhibitors */
	for (i = 0; i < CAPHE_N_INHIBITORS; i++) {
		priv->inhibitors[i] = caphe_dbus_inhibitor_new(caphe_inhibitor_ids[i]);

		g_signal_connect_object(priv->inhibitors[i], "notify::available",
		                        G_CALLBACK(on_inhibitor_notify_available),
		                        self, 0);
	}

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_inhibitor_list_parent_class)->constructed)
		G_OBJECT_CLASS(caphe_inhibitor_list_parent_class)->constructed(object);
}

static void
caphe_inhibitor_list_init(CapheInhibitorList *self)
{
	CapheInhibitorListPrivate *priv = caphe_inhibitor_list_get_instance_private(self);

	TRACE("%p", self);

	/* Init some stuff */
	priv->ready = FALSE;

	/* Set private pointer */
	self->priv = priv;
}

static void
caphe_inhibitor_list_class_init(CapheInhibitorListClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize    = caphe_inhibitor_list_finalize;
	object_class->constructed = caphe_inhibitor_list_constructed;

	/* Signals */
	signals[SIGNAL_READY] =
	        g_signal_new("ready", G_TYPE_FROM_CLASS(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
