/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2017-2021 Arnaud Rebillout
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
#include <gio/gio.h>

/* GObject declarations */

#define GV_TYPE_CONSOLE_APPLICATION gv_console_application_get_type()

G_DECLARE_FINAL_TYPE(GvConsoleApplication, gv_console_application,
                     GV, CONSOLE_APPLICATION, GApplication)

/* Methods */

GApplication *gv_console_application_new(const gchar *application_id);
