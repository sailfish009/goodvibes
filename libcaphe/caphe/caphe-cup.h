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

#ifndef __LIBCAPHE_CAPHE_CUP_H__
#define __LIBCAPHE_CAPHE_CUP_H__

#include <glib-object.h>

#include "caphe-inhibitor.h"

/* GObject declarations */

#define CAPHE_TYPE_CUP caphe_cup_get_type()

G_DECLARE_FINAL_TYPE(CapheCup, caphe_cup, CAPHE, CUP, GObject)

/* Global instance (private, don't access) */

extern CapheCup *caphe_cup_default_instance;

/* Methods */

CapheCup *caphe_cup_get_default(void);

void     caphe_cup_inhibit     (CapheCup *self, const gchar *reason);
void     caphe_cup_uninhibit   (CapheCup *self);
gboolean caphe_cup_is_inhibited(CapheCup *self);

/* Property accessors */

void            caphe_cup_set_application_name(CapheCup *self, const gchar *name);
CapheInhibitor *caphe_cup_get_inhibitor       (CapheCup *self);

#endif /* __LIBCAPHE_CAPHE_CUP_H__ */
