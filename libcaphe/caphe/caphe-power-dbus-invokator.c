/*
 * Libcaphe
 *
 * Copyright (C) 2016-2017 Arnaud Rebillout
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
#include "caphe-dbus-invokator.h"
#include "caphe-power-dbus-invokator.h"

/*
 * Debug macros
 */


#define debug(proxy, fmt, ...)    \
        g_debug("[%s]: " fmt, \
                proxy ? g_dbus_proxy_get_name(proxy) : "PowerManagement", \
                ##__VA_ARGS__)

/*
 * GObject definitions
 */

struct _CaphePowerDbusInvokatorPrivate {
	guint32 cookie;
};

typedef struct _CaphePowerDbusInvokatorPrivate CaphePowerDbusInvokatorPrivate;

struct _CaphePowerDbusInvokator {
	/* Parent instance structure */
	CapheDbusInvokator           parent_instance;
	/* Private data */
	CaphePowerDbusInvokatorPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(CaphePowerDbusInvokator, caphe_power_dbus_invokator,
                           CAPHE_TYPE_DBUS_INVOKATOR)

/*
 * DbusInvokator methods
 */

static gboolean
caphe_power_dbus_invokator_is_inhibited(CapheDbusInvokator *dbus_invokator)
{
	CaphePowerDbusInvokator *self = CAPHE_POWER_DBUS_INVOKATOR(dbus_invokator);
	CaphePowerDbusInvokatorPrivate *priv = self->priv;

	return priv->cookie != 0;
}

static void
caphe_power_dbus_invokator_uninhibit(CapheDbusInvokator *dbus_invokator)
{
	CaphePowerDbusInvokator *self = CAPHE_POWER_DBUS_INVOKATOR(dbus_invokator);
	CaphePowerDbusInvokatorPrivate *priv = self->priv;
	GDBusProxy *proxy = caphe_dbus_invokator_get_proxy(dbus_invokator);

	if (priv->cookie == 0) {
		debug(proxy, "Not inhibited (no cookie)");
		return;
	}

	if (proxy == NULL) {
		debug(proxy, "No proxy");
		return;
	}

	g_dbus_proxy_call(proxy,
	                  "UnInhibit",
	                  g_variant_new("(u)",
	                                priv->cookie),
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1,
	                  NULL, NULL, NULL);

	priv->cookie = 0;

	debug(proxy, "Uninhibited");
}

static gboolean
caphe_power_dbus_invokator_inhibit(CapheDbusInvokator *dbus_invokator,
                                   const gchar *application, const gchar *reason,
                                   GError **error)
{
	CaphePowerDbusInvokator *self = CAPHE_POWER_DBUS_INVOKATOR(dbus_invokator);
	CaphePowerDbusInvokatorPrivate *priv = self->priv;
	GDBusProxy *proxy = caphe_dbus_invokator_get_proxy(dbus_invokator);
	GVariant *res;

	if (priv->cookie != 0) {
		debug(proxy, "Already inhibited (cookie: %u)", priv->cookie);
		return TRUE;
	}

	if (proxy == NULL) {
		debug(proxy, "No proxy");
		return FALSE;
	}

	res = g_dbus_proxy_call_sync(proxy,
	                             "Inhibit",
	                             g_variant_new("(ss)",
	                                             application,
	                                             reason),
	                             G_DBUS_CALL_FLAGS_NONE,
	                             -1,
	                             NULL,
	                             error);

	if (res == NULL)
		return FALSE;

	g_variant_get(res, "(u)", &priv->cookie);
	g_variant_unref(res);

	debug(proxy, "Inhibited (cookie: %u)", priv->cookie);

	return TRUE;
}

/*
 * GObject methods
 */

static void
caphe_power_dbus_invokator_finalize(GObject *object)
{
	CapheDbusInvokator *dbus_invokator = CAPHE_DBUS_INVOKATOR(object);

	TRACE("%p", object);

	/* Ensure uninhibition */
	caphe_power_dbus_invokator_uninhibit(dbus_invokator);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_power_dbus_invokator_parent_class)->finalize)
		G_OBJECT_CLASS(caphe_power_dbus_invokator_parent_class)->finalize(object);
}

static void
caphe_power_dbus_invokator_init(CaphePowerDbusInvokator *self)
{
	CaphePowerDbusInvokatorPrivate *priv =
	        caphe_power_dbus_invokator_get_instance_private(self);

	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = priv;
}

static void
caphe_power_dbus_invokator_class_init(CaphePowerDbusInvokatorClass *class)
{
	GObjectClass            *object_class = G_OBJECT_CLASS(class);
	CapheDbusInvokatorClass *dbus_invokator_class = CAPHE_DBUS_INVOKATOR_CLASS(class);

	TRACE("%p", class);

	/* GObject methods */
	object_class->finalize             = caphe_power_dbus_invokator_finalize;

	/* Implement methods */
	dbus_invokator_class->inhibit      = caphe_power_dbus_invokator_inhibit;
	dbus_invokator_class->uninhibit    = caphe_power_dbus_invokator_uninhibit;
	dbus_invokator_class->is_inhibited = caphe_power_dbus_invokator_is_inhibited;
}
