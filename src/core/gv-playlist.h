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

#include <gio/gio.h>
#include <glib-object.h>

/* GObject declarations */

#define GV_TYPE_PLAYLIST gv_playlist_get_type()

G_DECLARE_FINAL_TYPE(GvPlaylist, gv_playlist, GV, PLAYLIST, GObject)

/* Errors */

#define GV_PLAYLIST_ERROR (g_uri_error_quark())
GQuark gv_playlist_error_quark(void);

typedef enum {
	GV_PLAYLIST_ERROR_CONTENT,
	GV_PLAYLIST_ERROR_CONTENT_TYPE,
	GV_PLAYLIST_ERROR_DOWNLOAD,
	GV_PLAYLIST_ERROR_EXTENSION,
	GV_PLAYLIST_ERROR_TOO_BIG,
} GvPlaylistError;

/* Data types */

typedef enum {
	GV_PLAYLIST_FORMAT_UNKNOWN,
	GV_PLAYLIST_FORMAT_ASX,
	GV_PLAYLIST_FORMAT_M3U,
	GV_PLAYLIST_FORMAT_PLS,
	GV_PLAYLIST_FORMAT_XSPF
} GvPlaylistFormat;

/* Functions */

GvPlaylistFormat gv_playlist_guess_format(const gchar *playlist_uri,
					  GError **error);

/* Methods */

GvPlaylist  *gv_playlist_new             (const gchar *uri);
void         gv_playlist_download_async  (GvPlaylist *self,
					  const gchar *user_agent,
					  GCancellable *cancellable,
					  GAsyncReadyCallback callback,
					  gpointer user_data);
gboolean     gv_playlist_download_finish (GvPlaylist *self,
					  GAsyncResult *res,
					  GError **error);
gboolean     gv_playlist_parse           (GvPlaylist *self, GError **error);
const gchar *gv_playlist_get_first_stream(GvPlaylist *self);
GSList      *gv_playlist_get_stream_uris (GvPlaylist *self);

/* Property accessors */

const gchar *gv_playlist_get_uri           (GvPlaylist *self);
const gchar *gv_playlist_get_redirected_uri(GvPlaylist *self);
