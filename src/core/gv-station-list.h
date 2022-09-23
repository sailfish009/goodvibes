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

#include "core/gv-station.h"

/* GObject declarations */

#define GV_TYPE_STATION_LIST gv_station_list_get_type()

G_DECLARE_FINAL_TYPE(GvStationList, gv_station_list, GV, STATION_LIST, GObject)

/* Data types */

typedef struct _GvStationListIter GvStationListIter;

/* Methods */

GvStationList *gv_station_list_new_from_xdg_dirs(const gchar *default_stations);
GvStationList *gv_station_list_new_from_paths   (const gchar *load_path,
                                                 const gchar *save_path);

void  gv_station_list_load  (GvStationList *self);
void  gv_station_list_save  (GvStationList *self);
void  gv_station_list_empty (GvStationList *self);
guint gv_station_list_length(GvStationList *self);

void gv_station_list_prepend      (GvStationList *self, GvStation *station);
void gv_station_list_append       (GvStationList *self, GvStation *station);
void gv_station_list_insert       (GvStationList *self, GvStation *station, gint position);
void gv_station_list_insert_before(GvStationList *self, GvStation *station, GvStation *before);
void gv_station_list_insert_after (GvStationList *self, GvStation *station, GvStation *after);
void gv_station_list_remove       (GvStationList *self, GvStation *station);

void gv_station_list_move       (GvStationList *self, GvStation *station, gint position);
void gv_station_list_move_before(GvStationList *self, GvStation *station, GvStation *before);
void gv_station_list_move_after (GvStationList *self, GvStation *station, GvStation *after);
void gv_station_list_move_first (GvStationList *self, GvStation *station);
void gv_station_list_move_last  (GvStationList *self, GvStation *station);

GvStation *gv_station_list_first(GvStationList *self);
GvStation *gv_station_list_last (GvStationList *self);
GvStation *gv_station_list_at   (GvStationList *self, guint n);
GvStation *gv_station_list_prev (GvStationList *self, GvStation *station, gboolean repeat,
                                 gboolean shuffle);
GvStation *gv_station_list_next (GvStationList *self, GvStation *station, gboolean repeat,
                                 gboolean shuffle);

GvStation *gv_station_list_find            (GvStationList *self, GvStation *station);
GvStation *gv_station_list_find_by_name    (GvStationList *self, const gchar *name);
GvStation *gv_station_list_find_by_uri     (GvStationList *self, const gchar *uri);
GvStation *gv_station_list_find_by_uid     (GvStationList *self, const gchar *uid);
GvStation *gv_station_list_find_by_guessing(GvStationList *self, const gchar *string);

/* Iterator methods */

GvStationListIter *gv_station_list_iter_new (GvStationList *self);
void               gv_station_list_iter_free(GvStationListIter *iter);
gboolean           gv_station_list_iter_loop(GvStationListIter *iter, GvStation **station);

/* Property accessors */

const gchar *gv_station_list_get_load_path(GvStationList *self);
const gchar *gv_station_list_get_save_path(GvStationList *self);
