/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
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

#define GV_TYPE_CONSOLE_OUTPUT gv_console_output_get_type()

G_DECLARE_FINAL_TYPE(GvConsoleOutput, gv_console_output, GV, CONSOLE_OUTPUT, GvFeature)

/* Public methods */

GvFeature *gv_console_output_new(void);
