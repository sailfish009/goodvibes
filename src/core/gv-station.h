/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2020 Arnaud Rebillout
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

#include <glib-object.h>

/* GObject declarations */

#define GV_TYPE_STATION gv_station_get_type()

G_DECLARE_FINAL_TYPE(GvStation, gv_station, GV, STATION, GInitiallyUnowned)

/* Methods */

GvStation *gv_station_new              (const gchar *name, const gchar *uri);
gchar     *gv_station_make_name        (GvStation *self, gboolean escape);
gboolean   gv_station_download_playlist(GvStation *self);

/* Property accessors */

const gchar *gv_station_get_uid             (GvStation *self);
const gchar *gv_station_get_name            (GvStation *self);
void         gv_station_set_name            (GvStation *self, const gchar *name);
const gchar *gv_station_get_uri             (GvStation *self);
void         gv_station_set_uri             (GvStation *self, const gchar *uri);
const gchar *gv_station_get_name_or_uri     (GvStation *self);
GSList      *gv_station_get_stream_uris     (GvStation *self);
const gchar *gv_station_get_first_stream_uri(GvStation *self);
const gchar *gv_station_get_user_agent      (GvStation *self);
void         gv_station_set_user_agent      (GvStation *self, const gchar *user_agent);
guint        gv_station_get_channels        (GvStation *self);
void         gv_station_set_channels        (GvStation *self, guint channels);
const gchar *gv_station_get_codec           (GvStation *self);
void         gv_station_set_codec           (GvStation *self, const gchar *codec);
guint        gv_station_get_maximum_bitrate (GvStation *self);
void         gv_station_set_maximum_bitrate (GvStation *self, guint bitrate);
guint        gv_station_get_minimum_bitrate (GvStation *self);
void         gv_station_set_minimum_bitrate (GvStation *self, guint bitrate);
guint        gv_station_get_nominal_bitrate (GvStation *self);
void         gv_station_set_nominal_bitrate (GvStation *self, guint bitrate);
guint        gv_station_get_sample_rate     (GvStation *self);
void         gv_station_set_sample_rate     (GvStation *self, guint sample_rate);
