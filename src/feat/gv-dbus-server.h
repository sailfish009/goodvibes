/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2024 Arnaud Rebillout
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

#pragma once

#include <glib-object.h>

#include "base/gv-feature.h"

/* GObject declarations */

#define GV_TYPE_DBUS_SERVER gv_dbus_server_get_type()

G_DECLARE_DERIVABLE_TYPE(GvDbusServer, gv_dbus_server, GV, DBUS_SERVER, GvFeature)

/* Data types */

struct _GvDbusServerClass {
	/* Parent class */
	GvFeatureClass parent_class;
};

typedef GVariant *(*GvDbusMethodCall)  (GvDbusServer *, GVariant *, GError **);
typedef GVariant *(*GvDbusPropertyGet) (GvDbusServer *);
typedef gboolean  (*GvDbusPropertySet) (GvDbusServer *, GVariant *, GError **);

struct _GvDbusMethod {
	const gchar             *name;
	const GvDbusMethodCall  call;
};

typedef struct _GvDbusMethod GvDbusMethod;

struct _GvDbusProperty {
	const gchar              *name;
	const GvDbusPropertyGet  get;
	const GvDbusPropertySet  set;
};

typedef struct _GvDbusProperty GvDbusProperty;

struct _GvDbusInterface {
	const gchar           *name;
	const GvDbusMethod   *methods;
	const GvDbusProperty *properties;
};

typedef struct _GvDbusInterface GvDbusInterface;

/* Methods */

GvDbusServer *gv_dbus_server_new(void);

void gv_dbus_server_emit_signal(GvDbusServer *self,
                                const gchar *interface_name,
                                const gchar *signal_name,
                                GVariant *parameters);

void gv_dbus_server_emit_signal_property_changed(GvDbusServer *self,
                                                 const gchar *interface_name,
                                                 const gchar *property_name,
                                                 GVariant *value);

/* Property accessors */

void gv_dbus_server_set_dbus_name           (GvDbusServer *self, const gchar *name);
void gv_dbus_server_set_dbus_path           (GvDbusServer *self, const gchar *path);
void gv_dbus_server_set_dbus_introspection  (GvDbusServer *self, const gchar *introspection);
void gv_dbus_server_set_dbus_interface_table(GvDbusServer *self,
                                             GvDbusInterface *interface_table);
