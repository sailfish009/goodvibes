/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023-2024 Arnaud Rebillout
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

#include <glib.h>

typedef GSList *(*GvPlaylistParser)(const gchar *, gsize);

GSList *gv_parse_m3u_playlist (const gchar *text, gsize text_size);
GSList *gv_parse_pls_playlist (const gchar *text, gsize text_size);
GSList *gv_parse_asx_playlist (const gchar *text, gsize text_size);
GSList *gv_parse_xspf_playlist(const gchar *text, gsize text_size);
