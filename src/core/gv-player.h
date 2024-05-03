/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2024 Arnaud Rebillout
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
#include "core/gv-playback.h"
#include "core/gv-station.h"
#include "core/gv-station-list.h"

/* GObject declarations */

#define GV_TYPE_PLAYER gv_player_get_type()

G_DECLARE_FINAL_TYPE(GvPlayer, gv_player, GV, PLAYER, GObject)

/* Methods */

GvPlayer *gv_player_new   (GvEngine *engine, GvPlayback *playback, GvStationList *station_list);

void      gv_player_go    (GvPlayer *self, const gchar *string_to_play);

void      gv_player_play  (GvPlayer *self);
void      gv_player_stop  (GvPlayer *self);
void      gv_player_toggle(GvPlayer *self);
gboolean  gv_player_prev  (GvPlayer *self);
gboolean  gv_player_next  (GvPlayer *self);

/* Property accessors */

gboolean     gv_player_get_playing            (GvPlayer *self);
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
