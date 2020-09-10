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

#pragma once

#include <glib-object.h>

/* GObject declarations */

#define GV_TYPE_INHIBITOR_IMPL gv_inhibitor_impl_get_type()

G_DECLARE_DERIVABLE_TYPE(GvInhibitorImpl, gv_inhibitor_impl,
		GV, INHIBITOR_IMPL, GObject)

/* Data types */

struct _GvInhibitorImplClass
{
	/* Parent class */
	GObjectClass parent_class;
	/* Virtual methods */
	gboolean (* inhibit)     (GvInhibitorImpl *impl,
			          const gchar *reason,
				  GError **err);
	void     (* uninhibit)   (GvInhibitorImpl *impl);
	gboolean (* is_inhibited)(GvInhibitorImpl *impl);
};

typedef struct _GvInhibitorImplClass GvInhibitorImplClass;

/* Public methods */

GvInhibitorImpl * gv_inhibitor_impl_make        (const gchar *name);
gboolean          gv_inhibitor_impl_inhibit     (GvInhibitorImpl *impl,
		                                 const gchar *reason,
		                                 GError **err);
void              gv_inhibitor_impl_uninhibit   (GvInhibitorImpl *impl);
gboolean          gv_inhibitor_impl_is_inhibited(GvInhibitorImpl *impl);
const gchar *     gv_inhibitor_impl_get_name    (GvInhibitorImpl *impl);
