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

#ifndef __LIBCAPHE_CAPHE_INHIBITOR_LIST_H__
#define __LIBCAPHE_CAPHE_INHIBITOR_LIST_H__

#include <glib-object.h>

#include "caphe-inhibitor.h"

/* GObject declarations */

#define CAPHE_TYPE_INHIBITOR_LIST caphe_inhibitor_list_get_type()

G_DECLARE_FINAL_TYPE(CapheInhibitorList, caphe_inhibitor_list, CAPHE, INHIBITOR_LIST, GObject)

/* Methods */

CapheInhibitorList  *caphe_inhibitor_list_new           (void);
CapheInhibitor     **caphe_inhibitor_list_get_inhibitors(CapheInhibitorList *self);

#endif /* __LIBCAPHE_CAPHE_INHIBITOR_LIST_H__ */
