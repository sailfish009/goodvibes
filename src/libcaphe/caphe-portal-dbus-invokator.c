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
#include "caphe-dbus-invokator.h"
#include "caphe-portal-dbus-invokator.h"

/*
 * Debug macros
 */

#define debug(proxy, fmt, ...)    \
        g_debug("[%s]: " fmt, \
                proxy ? g_dbus_proxy_get_name(proxy) : "Portal", \
                ##__VA_ARGS__)

/*
 * GObject definitions
 */

struct _CaphePortalDbusInvokatorPrivate {
	char *handle;
};

typedef struct _CaphePortalDbusInvokatorPrivate CaphePortalDbusInvokatorPrivate;

struct _CaphePortalDbusInvokator {
	/* Parent instance structure */
	CapheDbusInvokator               parent_instance;
	/* Private data */
	CaphePortalDbusInvokatorPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(CaphePortalDbusInvokator, caphe_portal_dbus_invokator,
                           CAPHE_TYPE_DBUS_INVOKATOR)

/*
 * DbusInvokator methods
 */

static gboolean
caphe_portal_dbus_invokator_is_inhibited(CapheDbusInvokator *dbus_invokator)
{
	CaphePortalDbusInvokator *self = CAPHE_PORTAL_DBUS_INVOKATOR(dbus_invokator);
	CaphePortalDbusInvokatorPrivate *priv = self->priv;

	return priv->handle != NULL;
}

static void
caphe_portal_dbus_invokator_uninhibit(CapheDbusInvokator *dbus_invokator)
{
	CaphePortalDbusInvokator *self = CAPHE_PORTAL_DBUS_INVOKATOR(dbus_invokator);
	CaphePortalDbusInvokatorPrivate *priv = self->priv;
	GDBusProxy *proxy = caphe_dbus_invokator_get_proxy(dbus_invokator);

	if (priv->handle == NULL) {
		debug(proxy, "Not inhibited (no handle)");
		return;
	}

	if (proxy == NULL) {
		debug(proxy, "No proxy");
		return;
	}

	g_dbus_connection_call(g_dbus_proxy_get_connection(proxy),
	                       "org.freedesktop.portal.Desktop",
	                       priv->handle,
	                       "org.freedesktop.portal.Request",
	                       "Close",
	                       g_variant_new("()"),
	                       G_VARIANT_TYPE_UNIT,
	                       G_DBUS_CALL_FLAGS_NONE,
	                       -1,
	                       NULL, NULL, NULL);

	g_free(priv->handle);
	priv->handle = NULL;

	debug(proxy, "Uninhibited");
}

static gboolean
caphe_portal_dbus_invokator_inhibit(CapheDbusInvokator *dbus_invokator,
                                    const gchar *application G_GNUC_UNUSED,
                                    const gchar *reason, GError **error)
{
	CaphePortalDbusInvokator *self = CAPHE_PORTAL_DBUS_INVOKATOR(dbus_invokator);
	CaphePortalDbusInvokatorPrivate *priv = self->priv;
	GDBusProxy *proxy = caphe_dbus_invokator_get_proxy(dbus_invokator);
	GVariant *res;

	if (priv->handle) {
		debug(proxy, "Already inhibited (handle: %s)", priv->handle);
		return TRUE;
	}

	if (proxy == NULL) {
		debug(proxy, "No proxy");
		return FALSE;
	}

	GVariantBuilder options;
	g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&options, "{sv}", "reason", g_variant_new_string(reason));
	res = g_dbus_proxy_call_sync(proxy,
	                             "Inhibit",
	                             g_variant_new("(su@a{sv})",
	                                             "", /* window */
	                                             4,
	                                             g_variant_builder_end(&options)),
	                             G_DBUS_CALL_FLAGS_NONE,
	                             -1,
	                             NULL,
	                             error);

	if (res == NULL)
		return FALSE;

	g_variant_get(res, "(o)", &priv->handle);
	g_variant_unref(res);

	debug(proxy, "Inhibited (handle: %s)", priv->handle);

	return TRUE;
}

/*
 * GObject methods
 */

static void
caphe_portal_dbus_invokator_finalize(GObject *object)
{
	CapheDbusInvokator *dbus_invokator = CAPHE_DBUS_INVOKATOR(object);

	TRACE("%p", object);

	/* Ensure uninhibition */
	caphe_portal_dbus_invokator_uninhibit(dbus_invokator);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_portal_dbus_invokator_parent_class)->finalize)
		G_OBJECT_CLASS(caphe_portal_dbus_invokator_parent_class)->finalize(object);
}

static void
caphe_portal_dbus_invokator_init(CaphePortalDbusInvokator *self)
{
	CaphePortalDbusInvokatorPrivate *priv =
	        caphe_portal_dbus_invokator_get_instance_private(self);

	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = priv;
}

static void
caphe_portal_dbus_invokator_class_init(CaphePortalDbusInvokatorClass *class)
{
	GObjectClass            *object_class = G_OBJECT_CLASS(class);
	CapheDbusInvokatorClass *dbus_invokator_class = CAPHE_DBUS_INVOKATOR_CLASS(class);

	TRACE("%p", class);

	/* GObject methods */
	object_class->finalize             = caphe_portal_dbus_invokator_finalize;

	/* Implement methods */
	dbus_invokator_class->inhibit      = caphe_portal_dbus_invokator_inhibit;
	dbus_invokator_class->uninhibit    = caphe_portal_dbus_invokator_uninhibit;
	dbus_invokator_class->is_inhibited = caphe_portal_dbus_invokator_is_inhibited;
}
