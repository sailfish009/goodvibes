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

/*
 * This header contains definitions to be used internally by ui files
 */

#pragma once

#include <gio/gio.h>

#include "ui/gv-status-icon.h"
#include "ui/gv-main-window-manager.h"

/* Global variables */

extern GSettings *gv_ui_settings;

extern GvStatusIcon *gv_ui_status_icon;
extern GvMainWindow *gv_ui_main_window;

/*
 * Visual layout, according to:
 * https://developer.gnome.org/hig/stable/visual-layout.html
 */

#define GV_UI_WINDOW_MARGIN  18
#define GV_UI_GROUP_SPACING  18
#define GV_UI_ELEM_SPACING   6
#define GV_UI_COLUMN_SPACING 12

#define GV_UI_MAIN_WINDOW_MARGIN 6
