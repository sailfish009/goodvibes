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

#include <glib-object.h>

#include "core/gv-engine.h"
#include "core/gv-metadata.h"
#include "core/gv-playlist.h"
#include "core/gv-station.h"
#include "core/gv-streaminfo.h"

/* GObject declarations */

#define GV_TYPE_PLAYBACK gv_playback_get_type()

G_DECLARE_FINAL_TYPE(GvPlayback, gv_playback, GV, PLAYBACK, GObject)

/* Playback state */

typedef enum {
	GV_PLAYBACK_STATE_STOPPED,
	GV_PLAYBACK_STATE_DOWNLOADING_PLAYLIST,
	GV_PLAYBACK_STATE_CONNECTING,
	GV_PLAYBACK_STATE_BUFFERING,
	GV_PLAYBACK_STATE_PLAYING,
	GV_PLAYBACK_STATE_WAITING_RETRY,
} GvPlaybackState;

const gchar *gv_playback_state_to_string(GvPlaybackState);

/* Playback error */

#define GV_TYPE_PLAYBACK_ERROR gv_playback_error_get_type()

GType gv_playback_error_get_type(void) G_GNUC_CONST;

typedef struct _GvPlaybackError GvPlaybackError;

struct _GvPlaybackError {
	gchar *message;  /* translatable string */
	gchar *details;  /* more details, not translated */
};

GvPlaybackError *gv_playback_error_new (const gchar *message, const gchar *details);
GvPlaybackError *gv_playback_error_copy(GvPlaybackError *self);
void             gv_playback_error_free(GvPlaybackError *self);

/* Methods */

GvPlayback *gv_playback_new  (GvEngine *engine);
void        gv_playback_start(GvPlayback *self);
void        gv_playback_stop (GvPlayback *self);

/* Property accessors */

GvStation       *gv_playback_get_station        (GvPlayback *self);
void             gv_playback_set_station        (GvPlayback *self, GvStation *station);
GvPlaybackState  gv_playback_get_state          (GvPlayback *self);
GvPlaybackError *gv_playback_get_error          (GvPlayback *self);
GvMetadata      *gv_playback_get_metadata       (GvPlayback *self);
GvStreaminfo    *gv_playback_get_streaminfo     (GvPlayback *self);
GvPlaylist      *gv_playback_get_playlist       (GvPlayback *self);
const gchar     *gv_playback_get_playlist_uri   (GvPlayback *self);
const gchar     *gv_playback_get_playlist_redirection_uri(GvPlayback *self);
const gchar     *gv_playback_get_stream_uri     (GvPlayback *self);
const gchar     *gv_playback_get_stream_redirection_uri(GvPlayback *self);
