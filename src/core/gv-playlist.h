/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib-object.h>

/* GObject declarations */

#define GV_TYPE_PLAYLIST gv_playlist_get_type()

G_DECLARE_FINAL_TYPE(GvPlaylist, gv_playlist, GV, PLAYLIST, GObject)

/* Data types */

typedef enum {
	GV_PLAYLIST_FORMAT_UNKNOWN,
	GV_PLAYLIST_FORMAT_M3U,
	GV_PLAYLIST_FORMAT_PLS,
	GV_PLAYLIST_FORMAT_ASX,
	GV_PLAYLIST_FORMAT_XSPF
} GvPlaylistFormat;

/* Class methods */

GvPlaylistFormat gv_playlist_get_format(const gchar *uri);

/* Methods */

GvPlaylist *gv_playlist_new     (const gchar *uri);
void        gv_playlist_download(GvPlaylist  *playlist,
                                 gboolean     insecure,
                                 const gchar *user_agent);

/* Property accessors */

const gchar *gv_playlist_get_uri        (GvPlaylist *self);
GSList      *gv_playlist_get_stream_list(GvPlaylist *playlist);
