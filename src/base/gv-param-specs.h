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

/* Default flags for objects:
 * - STATIC_STRINGS because we only use static strings.
 * - EXPLICIT_NOTIFY because we want to notify only when a property
 *   is changed. It's a design choice, every object should stick to it,
 *   otherwise expect surprises.
 */

#define GV_PARAM_READABLE  G_PARAM_READABLE  | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY
#define GV_PARAM_WRITABLE  G_PARAM_WRITABLE  | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY
#define GV_PARAM_READWRITE G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY
