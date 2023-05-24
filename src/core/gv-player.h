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

#pragma once

#include <glib-object.h>

#include "core/gv-engine.h"
#include "core/gv-metadata.h"
#include "core/gv-station.h"
#include "core/gv-station-list.h"
#include "core/gv-streaminfo.h"

/* GObject declarations */

#define GV_TYPE_PLAYER gv_player_get_type()

G_DECLARE_FINAL_TYPE(GvPlayer, gv_player, GV, PLAYER, GObject)

/* Playback state */

typedef enum {
	GV_PLAYBACK_STATE_STOPPED,
	GV_PLAYBACK_STATE_DOWNLOADING_PLAYLIST,
	GV_PLAYBACK_STATE_CONNECTING,
	GV_PLAYBACK_STATE_BUFFERING,
	GV_PLAYBACK_STATE_PLAYING
} GvPlaybackState;

const gchar *gv_playback_state_to_string(GvPlaybackState);

/* Playback error */

#define GV_TYPE_PLAYBACK_ERROR gv_playback_error_get_type()

GType gv_playback_error_get_type(void) G_GNUC_CONST;

typedef struct _GvPlaybackError GvPlaybackError;

struct _GvPlaybackError {
	gchar *message;
	gchar *details;
};

GvPlaybackError *gv_playback_error_new (const gchar *message, const gchar *details);
GvPlaybackError *gv_playback_error_copy(GvPlaybackError *self);
void             gv_playback_error_free(GvPlaybackError *self);

/* Methods */

GvPlayer *gv_player_new   (GvEngine *engine, GvStationList *station_list);

void      gv_player_go    (GvPlayer *self, const gchar *string_to_play);

void      gv_player_play  (GvPlayer *self);
void      gv_player_stop  (GvPlayer *self);
void      gv_player_toggle(GvPlayer *self);
gboolean  gv_player_prev  (GvPlayer *self);
gboolean  gv_player_next  (GvPlayer *self);

/* Property accessors */

GvPlaybackState  gv_player_get_playback_state       (GvPlayer *self);
GvPlaybackError *gv_player_get_playback_error       (GvPlayer *self);
const gchar   *gv_player_get_redirection_uri(GvPlayer *self);
GvStreaminfo  *gv_player_get_streaminfo  (GvPlayer *self);
GvMetadata    *gv_player_get_metadata    (GvPlayer *self);

GvStation   *gv_player_get_station            (GvPlayer *self);
GvStation   *gv_player_get_prev_station       (GvPlayer *self);
GvStation   *gv_player_get_next_station       (GvPlayer *self);
void         gv_player_set_station            (GvPlayer *self, GvStation *station);
gboolean     gv_player_set_station_by_name    (GvPlayer *self, const gchar *name);
gboolean     gv_player_set_station_by_uri     (GvPlayer *self, const gchar *uri);
gboolean     gv_player_set_station_by_guessing(GvPlayer *self, const gchar *string);

gboolean     gv_player_get_repeat      (GvPlayer *self);
void         gv_player_set_repeat      (GvPlayer *self, gboolean repeat);
gboolean     gv_player_get_shuffle     (GvPlayer *self);
void         gv_player_set_shuffle     (GvPlayer *self, gboolean shuffle);
gboolean     gv_player_get_autoplay    (GvPlayer *self);
void         gv_player_set_autoplay    (GvPlayer *self, gboolean autoplay);
guint        gv_player_get_volume      (GvPlayer *self);
void         gv_player_set_volume      (GvPlayer *self, guint volume);
void         gv_player_lower_volume    (GvPlayer *self);
void         gv_player_raise_volume    (GvPlayer *self);
gboolean     gv_player_get_mute        (GvPlayer *self);
void         gv_player_set_mute        (GvPlayer *self, gboolean mute);
void         gv_player_toggle_mute     (GvPlayer *self);
gboolean     gv_player_get_pipeline_enabled(GvPlayer *self);
void         gv_player_set_pipeline_enabled(GvPlayer *self, gboolean enabled);
const gchar *gv_player_get_pipeline_string (GvPlayer *self);
void         gv_player_set_pipeline_string (GvPlayer *self, const gchar *pipeline);
