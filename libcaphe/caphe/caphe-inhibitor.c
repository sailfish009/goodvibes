/*
 * Libcaphe
 *
 * Copyright (C) 2016-2017 Arnaud Rebillout
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

#include <glib-object.h>

#include "caphe-trace.h"
#include "caphe-inhibitor.h"

/*
 * GObject definitions
 */

G_DEFINE_INTERFACE(CapheInhibitor, caphe_inhibitor, G_TYPE_OBJECT)

/*
 * Methods
 */

gboolean
caphe_inhibitor_is_inhibited(CapheInhibitor *self)
{
	g_return_val_if_fail(CAPHE_IS_INHIBITOR(self), FALSE);

	return CAPHE_INHIBITOR_GET_IFACE(self)->is_inhibited(self);
}

void
caphe_inhibitor_uninhibit(CapheInhibitor *self)
{
	g_return_if_fail(CAPHE_IS_INHIBITOR(self));

	return CAPHE_INHIBITOR_GET_IFACE(self)->uninhibit(self);
}

gboolean
caphe_inhibitor_inhibit(CapheInhibitor *self, const gchar *application,
                        const gchar *reason, GError **error)
{
	g_return_val_if_fail(CAPHE_IS_INHIBITOR(self), FALSE);

	return CAPHE_INHIBITOR_GET_IFACE(self)->inhibit(self, application, reason, error);
}

/*
 * Property accessors
 */

const gchar *
caphe_inhibitor_get_name(CapheInhibitor *self)
{
	g_return_val_if_fail(CAPHE_IS_INHIBITOR(self), FALSE);

	return CAPHE_INHIBITOR_GET_IFACE(self)->get_name(self);
}

gboolean
caphe_inhibitor_get_available(CapheInhibitor *self)
{
	g_return_val_if_fail(CAPHE_IS_INHIBITOR(self), FALSE);

	return CAPHE_INHIBITOR_GET_IFACE(self)->get_available(self);
}

/*
 * GObject methods
 */

static void
caphe_inhibitor_default_init(CapheInhibitorInterface *iface)
{
	TRACE("%p", iface);

	g_object_interface_install_property
	(iface, g_param_spec_string("name", "Name", NULL, NULL,
	                            G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE |
	                            G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
	(iface, g_param_spec_boolean("available", "Available", NULL, FALSE,
	                             G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));
}
