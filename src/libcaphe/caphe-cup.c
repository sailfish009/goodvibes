/*
 * Libcaphe
 *
 * Copyright (C) 2016-2020 Arnaud Rebillout
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
#include "caphe-inhibitor-list.h"
#include "caphe-cup.h"

/*
 * Signals
 */

enum {
	SIGNAL_INHIBIT_FAILURE,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Construct-only properties */
	PROP_APPLICATION_NAME,
	/* Properties */
	PROP_INHIBITOR,
	/* Total number of properties */
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

/*
 * GObject definitions
 */

typedef struct {
	/* If NULL, means an unhibibit request.
	 * If non NULL, means an inhibit request.
	 */
	gchar *reason;
} CapheRequest;

struct _CapheCupPrivate {
	/* List of inhibitors */
	CapheInhibitorList *inhibitor_list;
	gboolean ready;

	/* Application name */
	gchar          *application_name;

	/* Current state */
	CapheInhibitor *current_inhibitor;
	gchar          *current_reason;
	/* Pending request */
	CapheRequest   *pending_request;
	/* Pending processing */
	guint           when_idle_process_id;
};

typedef struct _CapheCupPrivate CapheCupPrivate;

struct _CapheCup {
	/* Parent instance structure */
	GObject          parent_instance;
	/* Private data */
	CapheCupPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(CapheCup, caphe_cup, G_TYPE_OBJECT)

/*
 * Helpers
 */

CapheRequest *
caphe_request_new(const gchar *reason)
{
	CapheRequest *self;

	self = g_new0(CapheRequest, 1);
	self->reason = g_strdup(reason);

	return self;
}

void
caphe_request_free(CapheRequest *self)
{
	if (self == NULL)
		return;

	g_free(self->reason);
	g_free(self);
}

/*
 * Private methods
 */

static void caphe_cup_set_inhibitor(CapheCup *self, CapheInhibitor *inhibitor);

static void
caphe_cup_process(CapheCup *self)
{
	CapheCupPrivate *priv = self->priv;

	/* As long as we're not ready, we can't do anything */
	if (priv->ready == FALSE)
		return;

	/* If there's nothing pending, do nothing */
	if (priv->pending_request == NULL)
		return;

	/* We can start processing the pending request.
	 * In case we're trying to inhibit while already inhibited
	 * (it's possible in case the user wants to change the reason),
	 * we must slip an unhibit request in between.
	 */
	if (priv->current_reason && priv->pending_request->reason) {
		g_debug("Slipping uninhibit request between two inhibit requests");

		/* Uninhibit */
		if (priv->current_inhibitor) {
			caphe_inhibitor_uninhibit(priv->current_inhibitor);
			priv->current_inhibitor = NULL;
			g_free(priv->current_reason);
			priv->current_reason = NULL;
		}
	}

	/* Process now */
	if (priv->pending_request->reason) {
		CapheInhibitor **inhibitors;
		gboolean success;

		g_debug("Inhibition: processing request");

		for (inhibitors = caphe_inhibitor_list_get_inhibitors(priv->inhibitor_list);
		     *inhibitors; inhibitors++) {
			CapheInhibitor *inhibitor = *inhibitors;
			GError *error = NULL;

			/* Ignore inhibitors that are not available */
			if (caphe_inhibitor_get_available(inhibitor) == FALSE)
				continue;

			/* Attempt inhibition */
			success = caphe_inhibitor_inhibit(inhibitor,
			                                  priv->application_name,
			                                  priv->pending_request->reason,
			                                  &error);
			if (success == FALSE) {
				g_warning("Error while inhibiting: %s", error->message);
				g_clear_error(&error);
				continue;
			}

			/* Save new status */
			caphe_cup_set_inhibitor(self, inhibitor);
			g_free(priv->current_reason);
			priv->current_reason = g_strdup(priv->pending_request->reason);

			/* Done */
			break;
		}

		if (success == FALSE) {
			g_signal_emit(self, signals[SIGNAL_INHIBIT_FAILURE], 0);
		}

		/* Consume request */
		caphe_request_free(priv->pending_request);
		priv->pending_request = NULL;

	} else {
		g_debug("Uninhibition: processing request");

		/* Uninhibit */
		if (priv->current_inhibitor) {
			caphe_inhibitor_uninhibit(priv->current_inhibitor);
			caphe_cup_set_inhibitor(self, NULL);
			g_free(priv->current_reason);
			priv->current_reason = NULL;
		}

		/* Consume request */
		caphe_request_free(priv->pending_request);
		priv->pending_request = NULL;
	}
}

/*
 * Signal handlers
 */

static void
on_inhibitor_list_ready(CapheInhibitorList *inhibitor_list G_GNUC_UNUSED,
                        CapheCup *self)
{
	CapheCupPrivate *priv = self->priv;

	/* We're ready, let's remember that */
	priv->ready = TRUE;

	/* Process */
	caphe_cup_process(self);
}

/*
 * Public
 */

gboolean
caphe_cup_is_inhibited(CapheCup *self)
{
	CapheCupPrivate *priv = self->priv;

	return priv->current_inhibitor != NULL;
}

static gboolean
when_idle_process(CapheCup *self)
{
	CapheCupPrivate *priv = self->priv;

	caphe_cup_process(self);

	priv->when_idle_process_id = 0;

	return G_SOURCE_REMOVE;
}

void
caphe_cup_uninhibit(CapheCup *self)
{
	CapheCupPrivate *priv = self->priv;

	/* We might ignore the request in the following case:
	 * - we're already uninhibited
	 * - we're sure there's nothing happening at the moment
	 */
	if (priv->current_inhibitor == NULL &&
	    priv->pending_request == NULL)
		return;

	/* Otherwise, create a request */
	caphe_request_free(priv->pending_request);
	priv->pending_request = caphe_request_new(NULL);
	g_debug("Uninhibition: request created and pending");

	/* Schedule process */
	if (priv->when_idle_process_id == 0)
		priv->when_idle_process_id = g_idle_add((GSourceFunc) when_idle_process, self);
}

void
caphe_cup_inhibit(CapheCup *self, const gchar *reason)
{
	CapheCupPrivate *priv = self->priv;

	g_return_if_fail(reason != NULL);

	/* We might ignore the request in the following case:
	 * - we're already inhibited with the same reason
	 * - we're sure there's nothing happening at the moment
	 * ...
	 */
	if (priv->current_inhibitor &&
	    !g_strcmp0(reason, priv->current_reason) &&
	    priv->pending_request == NULL)
		return;

	/* Otherwise, create a request */
	caphe_request_free(priv->pending_request);
	priv->pending_request = caphe_request_new(reason);
	g_debug("Inhibition: request created and pending");

	/* Schedule process */
	if (priv->when_idle_process_id == 0)
		priv->when_idle_process_id = g_idle_add((GSourceFunc) when_idle_process, self);
}

CapheCup *
caphe_cup_new(void)
{
	return g_object_new(CAPHE_TYPE_CUP, NULL);
}

/*
 * Property accessors
 */

void
caphe_cup_set_application_name(CapheCup *self, const gchar *application_name)
{
	CapheCupPrivate *priv = self->priv;

	if (!g_strcmp0(priv->application_name, application_name))
		return;

	g_free(priv->application_name);
	priv->application_name = g_strdup(application_name);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_APPLICATION_NAME]);
}

CapheInhibitor *
caphe_cup_get_inhibitor(CapheCup *self)
{
	return self->priv->current_inhibitor;
}

static void
caphe_cup_set_inhibitor(CapheCup *self, CapheInhibitor *inhibitor)
{
	CapheCupPrivate *priv = self->priv;

	if (g_set_object(&priv->current_inhibitor, inhibitor))
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_INHIBITOR]);
}

static void
caphe_cup_get_property(GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
	CapheCup *self = CAPHE_CUP(object);

	switch (property_id) {
	case PROP_INHIBITOR:
		g_value_set_object(value, caphe_cup_get_inhibitor(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
caphe_cup_set_property(GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
	CapheCup *self = CAPHE_CUP(object);

	switch (property_id) {
	case PROP_APPLICATION_NAME:
		caphe_cup_set_application_name(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * GObject methods
 */

static void
caphe_cup_finalize(GObject *object)
{
	CapheCup *self = CAPHE_CUP(object);
	CapheCupPrivate *priv = self->priv;

	TRACE("%p", self);

	if (priv->when_idle_process_id)
		g_source_remove(priv->when_idle_process_id);

	caphe_request_free(priv->pending_request);

	g_free(priv->application_name);

	/* Unref current status */
	g_clear_object(&priv->current_inhibitor);
	g_free(priv->current_reason);

	/* Destroy inhibitor list */
	g_clear_object(&priv->inhibitor_list);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_cup_parent_class)->finalize)
		G_OBJECT_CLASS(caphe_cup_parent_class)->finalize(object);
}

static void
caphe_cup_constructed(GObject *object)
{
	CapheCup *self = CAPHE_CUP(object);
	CapheCupPrivate *priv = self->priv;

	TRACE("%p", self);

	/* Create inhibitor list */
	priv->inhibitor_list = caphe_inhibitor_list_new();
	g_signal_connect_object(priv->inhibitor_list, "ready",
	                        G_CALLBACK(on_inhibitor_list_ready),
	                        self, 0);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_cup_parent_class)->constructed)
		G_OBJECT_CLASS(caphe_cup_parent_class)->constructed(object);
}

static void
caphe_cup_init(CapheCup *self)
{
	CapheCupPrivate *priv = caphe_cup_get_instance_private(self);
	const gchar *app_name;

	TRACE("%p", self);

	/* Default values */
	app_name = g_get_application_name();
	if (app_name == NULL)
		app_name = "Unknown";
	priv->application_name = g_strdup(app_name);

	/* Set private pointer */
	self->priv = priv;
}

static void
caphe_cup_class_init(CapheCupClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize    = caphe_cup_finalize;
	object_class->constructed = caphe_cup_constructed;

	/* Install properties */
	object_class->get_property = caphe_cup_get_property;
	object_class->set_property = caphe_cup_set_property;

	properties[PROP_APPLICATION_NAME] =
	        g_param_spec_string("application-name", "Application name", NULL,
	                            NULL,
	                            G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE);

	properties[PROP_INHIBITOR] =
	        g_param_spec_object("inhibitor", "Current inhibitor", NULL,
	                            CAPHE_TYPE_INHIBITOR,
	                            G_PARAM_STATIC_STRINGS | G_PARAM_READABLE);

	g_object_class_install_properties(object_class, LAST_PROP, properties);

	/* Signals */
	signals[SIGNAL_INHIBIT_FAILURE] =
	        g_signal_new("inhibit-failure", G_TYPE_FROM_CLASS(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}

/*
 * Convenience functions
 */

CapheCup *caphe_cup_default_instance;

CapheCup *
caphe_cup_get_default(void)
{
	if (caphe_cup_default_instance == NULL)
		caphe_cup_default_instance = caphe_cup_new();

	return caphe_cup_default_instance;
}
