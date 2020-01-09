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
#include "caphe-session-dbus-invokator.h"

/*
 * Debug macros
 */

#define debug(proxy, fmt, ...)    \
        g_debug("[%s]: " fmt, \
                proxy ? g_dbus_proxy_get_name(proxy) : "SessionManager", \
                ##__VA_ARGS__)

/*
 * GObject definitions
 */

struct _CapheSessionDbusInvokatorPrivate {
	guint32 cookie;
};

typedef struct _CapheSessionDbusInvokatorPrivate CapheSessionDbusInvokatorPrivate;

struct _CapheSessionDbusInvokator {
	/* Parent instance structure */
	CapheDbusInvokator              parent_instance;
	/* Private data */
	CapheSessionDbusInvokatorPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(CapheSessionDbusInvokator, caphe_session_dbus_invokator,
                           CAPHE_TYPE_DBUS_INVOKATOR)

/*
 * DbusInvokator methods
 */

static gboolean
caphe_session_dbus_invokator_is_inhibited(CapheDbusInvokator *dbus_invokator)
{
	CapheSessionDbusInvokator *self = CAPHE_SESSION_DBUS_INVOKATOR(dbus_invokator);
	CapheSessionDbusInvokatorPrivate *priv = self->priv;

	return priv->cookie != 0;
}

static void
caphe_session_dbus_invokator_uninhibit(CapheDbusInvokator *dbus_invokator)
{
	CapheSessionDbusInvokator *self = CAPHE_SESSION_DBUS_INVOKATOR(dbus_invokator);
	CapheSessionDbusInvokatorPrivate *priv = self->priv;
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
	                  "Uninhibit",
	                  g_variant_new("(u)",
	                                priv->cookie),
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1,
	                  NULL, NULL, NULL);

	priv->cookie = 0;

	debug(proxy, "Uninhibited");
}

static gboolean
caphe_session_dbus_invokator_inhibit(CapheDbusInvokator *dbus_invokator,
                                     const gchar *application, const gchar *reason,
                                     GError **error)
{
	CapheSessionDbusInvokator *self = CAPHE_SESSION_DBUS_INVOKATOR(dbus_invokator);
	CapheSessionDbusInvokatorPrivate *priv = self->priv;
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

	/*
	 * From org.gnome.SessionManager.xml, the 4 arguments to pass are:
	 * - app_id       The application identifier
	 * - toplevel_xid The toplevel X window identifier
	 * - reason       The reason for the inhibit
	 * - flags        Flags that specify what should be inhibited
	 *
	 * The flags parameter must include at least one of the following:
	 * -  1 Inhibit logging out
	 * -  2 Inhibit user switching
	 * -  4 Inhibit suspending the session or computer
	 * -  8 Inhibit the session being marked as idle
	 * - 16 Inhibit auto-mounting removable media for the session
	 */
	res = g_dbus_proxy_call_sync(proxy,
	                             "Inhibit",
	                             g_variant_new("(susu)",
	                                             application,
	                                             0,
	                                             reason,
	                                             4),
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
caphe_session_dbus_invokator_finalize(GObject *object)
{
	CapheDbusInvokator *dbus_invokator = CAPHE_DBUS_INVOKATOR(object);

	TRACE("%p", object);

	/* Ensure uninhibition */
	caphe_session_dbus_invokator_uninhibit(dbus_invokator);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_session_dbus_invokator_parent_class)->finalize)
		G_OBJECT_CLASS(caphe_session_dbus_invokator_parent_class)->finalize(object);
}

static void
caphe_session_dbus_invokator_init(CapheSessionDbusInvokator *self)
{
	CapheSessionDbusInvokatorPrivate *priv =
	        caphe_session_dbus_invokator_get_instance_private(self);

	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = priv;
}

static void
caphe_session_dbus_invokator_class_init(CapheSessionDbusInvokatorClass *class)
{
	GObjectClass            *object_class = G_OBJECT_CLASS(class);
	CapheDbusInvokatorClass *dbus_invokator_class = CAPHE_DBUS_INVOKATOR_CLASS(class);

	TRACE("%p", class);

	/* GObject methods */
	object_class->finalize             = caphe_session_dbus_invokator_finalize;

	/* Implement methods */
	dbus_invokator_class->inhibit      = caphe_session_dbus_invokator_inhibit;
	dbus_invokator_class->uninhibit    = caphe_session_dbus_invokator_uninhibit;
	dbus_invokator_class->is_inhibited = caphe_session_dbus_invokator_is_inhibited;
}
