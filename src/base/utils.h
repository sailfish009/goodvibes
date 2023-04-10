/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2018-2021 Arnaud Rebillout
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

#include <gio/gio.h>

/* URI utils */

extern const gchar *GV_SUPPORTED_URI_SCHEMES[];
extern const gchar *GV_SUPPORTED_MIME_TYPES[];

gboolean gv_is_uri_scheme_supported    (const gchar *uri);
gboolean gv_get_uri_extension_lowercase(const gchar *uri, gchar **extension, GError **err);

/* XDG utils */

const gchar *gv_get_app_user_config_dir(void);
const gchar *gv_get_app_user_data_dir(void);
const gchar *const *gv_get_app_system_config_dirs(void);
const gchar *const *gv_get_app_system_data_dirs(void);

/* Misc utils */

GSettings *gv_get_settings(const gchar *component);

gboolean gv_in_test_suite(void);
