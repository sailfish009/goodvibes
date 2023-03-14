/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023 Arnaud Rebillout
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

typedef enum {
	GV_PLAYLIST_FORMAT_UNKNOWN,
	GV_PLAYLIST_FORMAT_M3U,
	GV_PLAYLIST_FORMAT_PLS,
	GV_PLAYLIST_FORMAT_ASX,
	GV_PLAYLIST_FORMAT_XSPF
} GvPlaylistFormat;

GvPlaylistFormat gv_playlist_get_format(const gchar *uri);
GSList * gv_playlist_parse(GvPlaylistFormat format, const gchar *content, gsize content_length);
