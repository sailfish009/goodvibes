/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2021-2024 Arnaud Rebillout
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

#include "ui/gv-main-window.h"

/* GObject declarations */

#define GV_TYPE_MAIN_WINDOW_STATUS_ICON gv_main_window_status_icon_get_type()

G_DECLARE_FINAL_TYPE(GvMainWindowStatusIcon, gv_main_window_status_icon, GV, MAIN_WINDOW_STATUS_ICON, GvMainWindow)

/* Property accessors */

// gint gv_main_window_get_natural_height (GvMainWindow *self);

/* Methods  */

GvMainWindow *gv_main_window_status_icon_new(GApplication *application);
