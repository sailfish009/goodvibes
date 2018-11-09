/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2018 Arnaud Rebillout
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

#ifndef __GOODVIBES_FRAMEWORK_GV_FRAMEWORK_H__
#define __GOODVIBES_FRAMEWORK_GV_FRAMEWORK_H__

#include <glib.h>
#include <glib/gi18n.h> /* _() is defined here */

#include "framework/config.h" /* generated by the build system */

#include "framework/gv-configurable.h"
#include "framework/gv-errorable.h"
#include "framework/gv-feature.h"
#include "framework/gv-framework-enum-types.h"
#include "framework/gv-param-specs.h"
#include "framework/log.h"
#include "framework/uri-schemes.h"
#include "framework/utils.h"
#include "framework/vt-codes.h"

void gv_framework_init          (void);
void gv_framework_init_completed(void);
void gv_framework_cleanup       (void);

void   gv_framework_register_object(gpointer object);
GList *gv_framework_get_objects    (void);

#endif /* __GOODVIBES_FRAMEWORK_GV_FRAMEWORK_H__ */
