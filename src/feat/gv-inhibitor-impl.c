/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020 Arnaud Rebillout
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

#include <glib.h>
#include <glib-object.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gv-ui.h"

#include "feat/gv-inhibitor-impl.h"


/*
 * Inhibitor implementation via GtkApplication.
 * <https://developer.gnome.org/gtk3/stable/GtkApplication.html>
 */

#define GV_TYPE_INHIBITOR_GTK gv_inhibitor_gtk_get_type()

G_DECLARE_FINAL_TYPE(GvInhibitorGtk, gv_inhibitor_gtk,
		GV, INHIBITOR_GTK, GvInhibitorImpl)

struct _GvInhibitorGtk
{
	GvInhibitorImpl parent_instance;
	guint cookie;
};

typedef struct _GvInhibitorGtk GvInhibitorGtk;

G_DEFINE_TYPE(GvInhibitorGtk, gv_inhibitor_gtk, GV_TYPE_INHIBITOR_IMPL)

static gboolean
gv_inhibitor_gtk_inhibit(GvInhibitorImpl *impl, const gchar *reason,
		GError **err G_GNUC_UNUSED)
{
	GvInhibitorGtk *self = GV_INHIBITOR_GTK(impl);
	GtkApplication *app = GTK_APPLICATION(gv_core_application);
	GtkWindow *win = GTK_WINDOW(gv_ui_main_window);

	if (self->cookie != 0)
		return TRUE;

	self->cookie = gtk_application_inhibit(app, win,
			GTK_APPLICATION_INHIBIT_SUSPEND, reason);

	return self->cookie != 0;
}

static void
gv_inhibitor_gtk_uninhibit(GvInhibitorImpl *impl)
{
	GvInhibitorGtk *self = GV_INHIBITOR_GTK(impl);
	GtkApplication *app = GTK_APPLICATION(gv_core_application);

	if (self->cookie == 0)
		return;

	gtk_application_uninhibit(app, self->cookie);
	self->cookie = 0;
}

static gboolean
gv_inhibitor_gtk_is_inhibited(GvInhibitorImpl *impl)
{
	GvInhibitorGtk *self = GV_INHIBITOR_GTK(impl);

	return self->cookie != 0;
}

static void
gv_inhibitor_gtk_finalize(GObject *object)
{
	GvInhibitorImpl *impl = GV_INHIBITOR_IMPL(object);

	TRACE("%p", object);

	gv_inhibitor_gtk_uninhibit(impl);

	G_OBJECT_CHAINUP_FINALIZE(gv_inhibitor_gtk, object);
}

static void
gv_inhibitor_gtk_init(GvInhibitorGtk *self)
{
	TRACE("%p", self);
}

static void
gv_inhibitor_gtk_class_init(GvInhibitorGtkClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GvInhibitorImplClass *impl_class = GV_INHIBITOR_IMPL_CLASS(class);

	TRACE("%p", class);

	object_class->finalize = gv_inhibitor_gtk_finalize;
	impl_class->inhibit = gv_inhibitor_gtk_inhibit;
	impl_class->uninhibit = gv_inhibitor_gtk_uninhibit;
	impl_class->is_inhibited = gv_inhibitor_gtk_is_inhibited;
}



/*
 * Inhibitor via the D-Bus service org.freedesktop.PowerManagement
 */

#define FDO_PM_BUS_NAME            "org.freedesktop.PowerManagement"
#define FDO_PM_INHIBIT_OBJECT_PATH "/org/freedesktop/PowerManagement/Inhibit"
#define FDO_PM_INHIBIT_INTERFACE   "org.freedesktop.PowerManagement.Inhibit"

#define GV_TYPE_INHIBITOR_PM gv_inhibitor_pm_get_type()

G_DECLARE_FINAL_TYPE(GvInhibitorPm, gv_inhibitor_pm,
		GV, INHIBITOR_PM, GvInhibitorImpl)

struct _GvInhibitorPm
{
	GvInhibitorImpl parent_instance;
	GDBusProxy *proxy;
	guint32 cookie;
};

typedef struct _GvInhibitorPm GvInhibitorPm;

G_DEFINE_TYPE(GvInhibitorPm, gv_inhibitor_pm, GV_TYPE_INHIBITOR_IMPL)

static GDBusProxy*
get_proxy_if_service_present(GDBusConnection  *connection,
                             GDBusProxyFlags   flags,
                             const gchar      *bus_name,
                             const gchar      *object_path,
                             const gchar      *interface,
                             GError          **err)
{
	GDBusProxy *proxy;
	gchar *owner;

	proxy = g_dbus_proxy_new_sync(
			connection,
			flags,
			NULL,    /* interface info */
			bus_name,
			object_path,
			interface,
			NULL,    /* cancellable */
			err);

	if (proxy == NULL)
		return NULL;

	/* Is there anyone actually providing the service? */
	owner = g_dbus_proxy_get_name_owner (proxy);
	if (owner) {
		g_free(owner);
	} else {
		g_clear_object(&proxy);
		g_set_error(err, G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER,
				"The name %s has no owner", bus_name);
	}

	return proxy;
}

static gboolean
gv_inhibitor_pm_inhibit(GvInhibitorImpl *impl, const gchar *reason,
		GError **err)
{
	GvInhibitorPm *self = GV_INHIBITOR_PM(impl);
	const gchar *app_name = g_get_application_name();
	GVariant *res;

	if (self->cookie != 0)
		return TRUE;

	if (self->proxy == NULL) {
		GApplication *app = G_APPLICATION(gv_core_application);

		g_assert_true(g_application_get_is_registered(app));

		self->proxy = get_proxy_if_service_present(
				g_application_get_dbus_connection(app),
				G_DBUS_PROXY_FLAGS_NONE,
				FDO_PM_BUS_NAME,
				FDO_PM_INHIBIT_OBJECT_PATH,
				FDO_PM_INHIBIT_INTERFACE,
				err);

		if (self->proxy == NULL)
			return FALSE;

	}

	res = g_dbus_proxy_call_sync(self->proxy, "Inhibit",
			g_variant_new("(ss)", app_name, reason),
	                G_DBUS_CALL_FLAGS_NONE, -1, NULL, err);

	if (res) {
	      g_variant_get(res, "(u)", &self->cookie);
	      g_variant_unref(res);
	}

	return self->cookie != 0;
}

static void
gv_inhibitor_pm_uninhibit(GvInhibitorImpl *impl)
{
	GvInhibitorPm *self = GV_INHIBITOR_PM(impl);

	if (self->proxy == NULL)
		return;

	if (self->cookie == 0)
		return;

	g_dbus_proxy_call(self->proxy,
	                  "UnInhibit",
	                  g_variant_new("(u)", self->cookie),
	                  G_DBUS_CALL_FLAGS_NONE,
	                  -1,
	                  NULL, NULL, NULL);

	self->cookie = 0;
}

static gboolean
gv_inhibitor_pm_is_inhibited(GvInhibitorImpl *impl)
{
	GvInhibitorPm *self = GV_INHIBITOR_PM(impl);

	return self->cookie != 0;
}

static void
gv_inhibitor_pm_finalize(GObject *object)
{
	GvInhibitorImpl *impl = GV_INHIBITOR_IMPL(object);
	GvInhibitorPm *self = GV_INHIBITOR_PM(object);

	TRACE("%p", object);

	gv_inhibitor_pm_uninhibit(impl);
	g_clear_object(&self->proxy);

	G_OBJECT_CHAINUP_FINALIZE(gv_inhibitor_pm, object);
}

static void
gv_inhibitor_pm_init(GvInhibitorPm *self)
{
	TRACE("%p", self);
}

static void
gv_inhibitor_pm_class_init(GvInhibitorPmClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GvInhibitorImplClass *impl_class = GV_INHIBITOR_IMPL_CLASS(class);

	TRACE("%p", class);

	object_class->finalize = gv_inhibitor_pm_finalize;
	impl_class->inhibit = gv_inhibitor_pm_inhibit;
	impl_class->uninhibit = gv_inhibitor_pm_uninhibit;
	impl_class->is_inhibited = gv_inhibitor_pm_is_inhibited;
}



/*
 * Base abstract class for the various inhibitors.
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties - refer to class_init() for more details */
	PROP_NAME,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

struct _GvInhibitorImplPrivate {
	gchar *name;
};

typedef struct _GvInhibitorImplPrivate GvInhibitorImplPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(GvInhibitorImpl, gv_inhibitor_impl, G_TYPE_OBJECT)

gboolean
gv_inhibitor_impl_inhibit(GvInhibitorImpl *impl, const gchar *reason, GError **err)
{
	GvInhibitorImplClass *impl_class = GV_INHIBITOR_IMPL_GET_CLASS(impl);

	g_return_val_if_fail(reason != NULL, FALSE);
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
	g_return_val_if_fail(impl_class->inhibit != NULL, FALSE);
	return impl_class->inhibit(impl, reason, err);
}

void
gv_inhibitor_impl_uninhibit(GvInhibitorImpl *impl)
{
	GvInhibitorImplClass *impl_class = GV_INHIBITOR_IMPL_GET_CLASS(impl);

	g_return_if_fail(impl_class->uninhibit != NULL);
	impl_class->uninhibit(impl);
}

gboolean
gv_inhibitor_impl_is_inhibited(GvInhibitorImpl *impl)
{
	GvInhibitorImplClass *impl_class = GV_INHIBITOR_IMPL_GET_CLASS(impl);

	g_return_val_if_fail(impl_class->is_inhibited != NULL, FALSE);
	return impl_class->is_inhibited(impl);
}

const gchar *
gv_inhibitor_impl_get_name(GvInhibitorImpl *impl)
{
	GvInhibitorImplPrivate *priv = gv_inhibitor_impl_get_instance_private(impl);

	return priv->name;
}

static void
gv_inhibitor_impl_set_name(GvInhibitorImpl *impl, const gchar *name)
{
	GvInhibitorImplPrivate *priv = gv_inhibitor_impl_get_instance_private(impl);

	/* This is a construct-only property */
	g_assert_null(priv->name);
	g_assert_nonnull(name);
	priv->name = g_strdup(name);
}

GvInhibitorImpl *
gv_inhibitor_impl_make(const gchar *name)
{
	if (!g_strcmp0(name, "gtk"))
		return g_object_new(GV_TYPE_INHIBITOR_GTK, "name", name, NULL);
	else if (!g_strcmp0(name, "pm"))
		return g_object_new(GV_TYPE_INHIBITOR_PM, "name", name, NULL);

	ERROR("Unsupported implementation name: %s", name);
	/* Program execution stops here */
}

static void
gv_inhibitor_impl_get_property(GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec)
{
	GvInhibitorImpl *impl = GV_INHIBITOR_IMPL(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NAME:
		g_value_set_string(value, gv_inhibitor_impl_get_name(impl));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_inhibitor_impl_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec   *pspec)
{
	GvInhibitorImpl *impl = GV_INHIBITOR_IMPL(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NAME:
		gv_inhibitor_impl_set_name(impl, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_inhibitor_impl_finalize(GObject *object)
{
	GvInhibitorImpl *impl = GV_INHIBITOR_IMPL(object);
	GvInhibitorImplPrivate *priv = gv_inhibitor_impl_get_instance_private(impl);

	TRACE("%p", object);

	g_free(priv->name);

	G_OBJECT_CHAINUP_FINALIZE(gv_inhibitor_impl, object);
}

static void
gv_inhibitor_impl_init(GvInhibitorImpl *impl)
{
	TRACE("%p", impl);
}

static void
gv_inhibitor_impl_class_init(GvInhibitorImplClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_inhibitor_impl_finalize;

	/* Properties */
	object_class->get_property = gv_inhibitor_impl_get_property;
	object_class->set_property = gv_inhibitor_impl_set_property;

	properties[PROP_NAME] =
		g_param_spec_string ("name", "Name", NULL, NULL,
				     GV_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
