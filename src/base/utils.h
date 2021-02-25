/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2018-2021 Arnaud Rebillout
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

#pragma once

#include <gio/gio.h>

gboolean gv_in_test_suite(void);

GSettings *gv_get_settings(const gchar *component);

const gchar *gv_get_app_user_config_dir(void);
const gchar *gv_get_app_user_data_dir(void);
const gchar *const *gv_get_app_system_config_dirs(void);
const gchar *const *gv_get_app_system_data_dirs(void);
