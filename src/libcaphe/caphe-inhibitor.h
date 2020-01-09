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

#ifndef __LIBCAPHE_CAPHE_INHIBITOR_H__
#define __LIBCAPHE_CAPHE_INHIBITOR_H__

#include <glib-object.h>

/* GObject declarations */

#define CAPHE_TYPE_INHIBITOR caphe_inhibitor_get_type()

G_DECLARE_INTERFACE(CapheInhibitor, caphe_inhibitor, CAPHE, INHIBITOR, GObject)

struct _CapheInhibitorInterface {
	/* Parent class */
	GTypeInterface parent_iface;

	/* Virtual Methods */
	gboolean (*inhibit)     (CapheInhibitor *self,
	                         const gchar *application,
	                         const gchar *reason,
	                         GError **error);
	void     (*uninhibit)   (CapheInhibitor *self);
	gboolean (*is_inhibited)(CapheInhibitor *self);

	/* Virtual property accessors */
	const gchar *(*get_name)     (CapheInhibitor *self);
	gboolean     (*get_available)(CapheInhibitor *self);
};

/* Methods */

gboolean caphe_inhibitor_inhibit     (CapheInhibitor *self,
                                      const gchar *application,
                                      const gchar *reason,
                                      GError **error);
void     caphe_inhibitor_uninhibit   (CapheInhibitor *self);
gboolean caphe_inhibitor_is_inhibited(CapheInhibitor *self);

/* Property accessors */

const gchar *caphe_inhibitor_get_name     (CapheInhibitor *self);
gboolean     caphe_inhibitor_get_available(CapheInhibitor *self);

#endif /* __LIBCAPHE_CAPHE_INHIBITOR_H__ */
