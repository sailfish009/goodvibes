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
#include "caphe-login-dbus-invokator.h"
#include "caphe-portal-dbus-invokator.h"
#include "caphe-power-dbus-invokator.h"
#include "caphe-session-dbus-invokator.h"
#include "caphe-dbus-invokator.h"
#include "caphe-dbus-proxy.h"
#include "caphe-dbus-inhibitor.h"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Number of installable properties */
	LAST_INSTALLABLE_PROP,
	/* Overriden properties */
	PROP_NAME,
	PROP_AVAILABLE,
	/* Total number of properties */
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

/*
 * GObject definitions
 */

struct _CapheDbusInhibitorPrivate {
	/* Properties */
	gchar              *name;
	/* Internal */
	CapheDbusProxy     *proxy;
	GType               invokator_type;
	CapheDbusInvokator *invokator;
};

typedef struct _CapheDbusInhibitorPrivate CapheDbusInhibitorPrivate;

struct _CapheDbusInhibitor {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	CapheDbusInhibitorPrivate *priv;
};

static void caphe_inhibitor_interface_init(CapheInhibitorInterface *iface);

G_DEFINE_TYPE_WITH_CODE(CapheDbusInhibitor, caphe_dbus_inhibitor, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(CapheDbusInhibitor)
                        G_IMPLEMENT_INTERFACE(CAPHE_TYPE_INHIBITOR,
                                        caphe_inhibitor_interface_init))

/*
 * Signal handlers
 */

static void
on_caphe_dbus_proxy_notify_proxy(CapheDbusProxy *caphe_proxy,
                                 GParamSpec *pspec G_GNUC_UNUSED,
                                 CapheDbusInhibitor *self)
{
	CapheDbusInhibitorPrivate *priv = self->priv;
	GDBusProxy *dbus_proxy = caphe_dbus_proxy_get_proxy(caphe_proxy);

	/* If the proxy disappeared, we must dispose of the invokator.
	 * Otherwise, we must create an invokator.
	 */
	if (dbus_proxy == NULL) {
		g_clear_object(&priv->invokator);

	} else {
		g_assert(priv->invokator == NULL);
		priv->invokator = caphe_dbus_invokator_new(priv->invokator_type, dbus_proxy);
	}

	/* Notify that availability changed */
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AVAILABLE]);
}

/*
 * Inhibitor methods
 */

static gboolean
caphe_dbus_inhibitor_is_inhibited(CapheInhibitor *inhibitor)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(inhibitor);
	CapheDbusInhibitorPrivate *priv = self->priv;

	if (priv->invokator == NULL)
		return FALSE;

	return caphe_dbus_invokator_is_inhibited(priv->invokator);
}

static void
caphe_dbus_inhibitor_uninhibit(CapheInhibitor *inhibitor)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(inhibitor);
	CapheDbusInhibitorPrivate *priv = self->priv;

	g_return_if_fail(priv->invokator != NULL);

	caphe_dbus_invokator_uninhibit(priv->invokator);
}

static gboolean
caphe_dbus_inhibitor_inhibit(CapheInhibitor *inhibitor, const gchar *application,
                             const gchar *reason, GError **error)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(inhibitor);
	CapheDbusInhibitorPrivate *priv = self->priv;

	g_return_val_if_fail(application != NULL, FALSE);
	g_return_val_if_fail(reason != NULL, FALSE);
	g_return_val_if_fail(priv->invokator != NULL, FALSE);

	return caphe_dbus_invokator_inhibit(priv->invokator, application, reason, error);
}

/*
 * Inhibitor property accessors
 */

static const gchar *
caphe_dbus_inhibitor_get_name(CapheInhibitor *inhibitor)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(inhibitor);
	CapheDbusInhibitorPrivate *priv = self->priv;

	return priv->name;
}

static void
caphe_dbus_inhibitor_set_name(CapheInhibitor *inhibitor, const gchar *name)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(inhibitor);
	CapheDbusInhibitorPrivate *priv = self->priv;

	/* Construct-only property */
	g_assert_null(priv->name);
	g_assert_nonnull(name);
	priv->name = g_strdup(name);
}

static gboolean
caphe_dbus_inhibitor_get_available(CapheInhibitor *inhibitor)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(inhibitor);
	CapheDbusInhibitorPrivate *priv = self->priv;

	return priv->invokator ? TRUE : FALSE;
}

static void
caphe_dbus_inhibitor_get_property(GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
	CapheInhibitor *inhibitor = CAPHE_INHIBITOR(object);

	switch (property_id) {
	/* Inhibitor */
	case PROP_NAME:
		g_value_set_string(value, caphe_dbus_inhibitor_get_name(inhibitor));
		break;
	case PROP_AVAILABLE:
		g_value_set_boolean(value, caphe_dbus_inhibitor_get_available(inhibitor));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
caphe_dbus_inhibitor_set_property(GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	CapheInhibitor *inhibitor = CAPHE_INHIBITOR(object);

	switch (property_id) {
	case PROP_NAME:
		caphe_dbus_inhibitor_set_name(inhibitor, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

CapheInhibitor *
caphe_dbus_inhibitor_new(const gchar *name)
{
	return g_object_new(CAPHE_TYPE_DBUS_INHIBITOR,
	                    "name", name,
	                    NULL);
}

/*
 * GObject methods
 */

static void
caphe_dbus_inhibitor_finalize(GObject *object)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(object);
	CapheDbusInhibitorPrivate *priv = self->priv;

	TRACE("%p", object);

	if (priv->invokator)
		g_object_unref(priv->invokator);

	if (priv->proxy)
		g_object_unref(priv->proxy);

	g_free(priv->name);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_dbus_inhibitor_parent_class)->finalize)
		G_OBJECT_CLASS(caphe_dbus_inhibitor_parent_class)->finalize(object);
}

static void
caphe_dbus_inhibitor_constructed(GObject *object)
{
	CapheDbusInhibitor *self = CAPHE_DBUS_INHIBITOR(object);
	CapheDbusInhibitorPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Create the dbus proxy */
	if (!g_strcmp0(priv->name, "gnome-session-manager")) {
		priv->invokator_type = CAPHE_TYPE_SESSION_DBUS_INVOKATOR;
		priv->proxy = caphe_dbus_proxy_new(G_BUS_TYPE_SESSION,
		                                   "org.gnome.SessionManager",
		                                   "/org/gnome/SessionManager",
		                                   "org.gnome.SessionManager");

	} else if (!g_strcmp0(priv->name, "xfce-session-manager")) {
		priv->invokator_type = CAPHE_TYPE_SESSION_DBUS_INVOKATOR;
		priv->proxy = caphe_dbus_proxy_new(G_BUS_TYPE_SESSION,
		                                   "org.xfce.SessionManager",
		                                   "/org/xfce/SessionManager",
		                                   "org.xfce.SessionManager");

	} else if (!g_strcmp0(priv->name, "xdg-portal")) {
		priv->invokator_type = CAPHE_TYPE_PORTAL_DBUS_INVOKATOR;
		priv->proxy = caphe_dbus_proxy_new(G_BUS_TYPE_SESSION,
		                                   "org.freedesktop.portal.Desktop",
		                                   "/org/freedesktop/portal/desktop",
		                                   "org.freedesktop.portal.Inhibit");

	} else if (!g_strcmp0(priv->name, "xdg-power-management")) {
		/* Also provided by the following names:
		 * - org.xfce.PowerManager
		 * - org.kde.Solid.PowerManagement
		 */
		priv->invokator_type = CAPHE_TYPE_POWER_DBUS_INVOKATOR;
		priv->proxy = caphe_dbus_proxy_new(G_BUS_TYPE_SESSION,
		                                   "org.freedesktop.PowerManagement",
		                                   "/org/freedesktop/PowerManagement/Inhibit",
		                                   "org.freedesktop.PowerManagement.Inhibit");

	} else if (!g_strcmp0(priv->name, "xdg-login1")) {
		priv->invokator_type = CAPHE_TYPE_LOGIN_DBUS_INVOKATOR;
		priv->proxy = caphe_dbus_proxy_new(G_BUS_TYPE_SYSTEM,
		                                   "org.freedesktop.login1",
		                                   "/org/freedesktop/login1",
		                                   "org.freedesktop.login1.Manager");
	} else {
		g_error("Invalid inhibitor name '%s'", priv->name);
		/* Program execution stops here */

	}

	/* Connect to the 'proxy' signal to be notified as soon as the proxy
	 * is created. The signal is ensured to be sent once at init time.
	 */
	g_signal_connect_object(priv->proxy, "notify::proxy",
	                        G_CALLBACK(on_caphe_dbus_proxy_notify_proxy),
	                        self, 0);

	/* Chain up */
	if (G_OBJECT_CLASS(caphe_dbus_inhibitor_parent_class)->constructed)
		G_OBJECT_CLASS(caphe_dbus_inhibitor_parent_class)->constructed(object);
}

static void
caphe_dbus_inhibitor_init(CapheDbusInhibitor *self)
{
	CapheDbusInhibitorPrivate *priv = caphe_dbus_inhibitor_get_instance_private(self);

	TRACE("%p", self);

	/* Set private pointer */
	self->priv = priv;
}

static void
caphe_dbus_inhibitor_class_init(CapheDbusInhibitorClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize    = caphe_dbus_inhibitor_finalize;
	object_class->constructed = caphe_dbus_inhibitor_constructed;

	/* Install properties */
	object_class->get_property = caphe_dbus_inhibitor_get_property;
	object_class->set_property = caphe_dbus_inhibitor_set_property;

	/* Override Inhibitor properties */
	g_object_class_override_property(object_class, PROP_NAME, "name");
	properties[PROP_NAME] =
	        g_object_class_find_property(object_class, "name");

	g_object_class_override_property(object_class, PROP_AVAILABLE, "available");
	properties[PROP_AVAILABLE] =
	        g_object_class_find_property(object_class, "available");
}

static void
caphe_inhibitor_interface_init(CapheInhibitorInterface *iface)
{
	/* Implement methods */
	iface->inhibit       = caphe_dbus_inhibitor_inhibit;
	iface->uninhibit     = caphe_dbus_inhibitor_uninhibit;
	iface->is_inhibited  = caphe_dbus_inhibitor_is_inhibited;
	iface->get_name      = caphe_dbus_inhibitor_get_name;
	iface->get_available = caphe_dbus_inhibitor_get_available;
}
