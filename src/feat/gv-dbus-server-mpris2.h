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

#include "feat/gv-dbus-server.h"

/* GObject declarations */

#define GV_TYPE_DBUS_SERVER_MPRIS2 gv_dbus_server_mpris2_get_type()

G_DECLARE_FINAL_TYPE(GvDbusServerMpris2, gv_dbus_server_mpris2,       \
                     GV, DBUS_SERVER_MPRIS2, GvDbusServer)

/* Public methods */

GvFeature *gv_dbus_server_mpris2_new(void);
