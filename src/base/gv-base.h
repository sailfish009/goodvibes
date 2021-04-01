/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <glib/gi18n.h> /* _() is defined here */

#include "base/config.h" /* generated by the build system */

#include "base/gv-configurable.h"
#include "base/gv-errorable.h"
#include "base/gv-feature.h"
#include "base/gv-base-enum-types.h"
#include "base/gv-param-specs.h"
#include "base/log.h"
#include "base/uri-schemes.h"
#include "base/utils.h"
#include "base/vt-codes.h"

void gv_base_init          (void);
void gv_base_init_completed(void);
void gv_base_cleanup       (void);

void   gv_base_register_object(gpointer object);
GList *gv_base_get_objects    (void);
