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

#include <glib.h>
#include <glib-object.h>

#include "core/gv-station.h"
#include "core/gv-metadata.h"
#include "core/gv-streaminfo.h"

/* GObject declarations */

#define GV_TYPE_ENGINE gv_engine_get_type()

G_DECLARE_FINAL_TYPE(GvEngine, gv_engine, GV, ENGINE, GObject)

/* Data types */

typedef enum {
	GV_ENGINE_STATE_STOPPED = 0,
	GV_ENGINE_STATE_CONNECTING,
	GV_ENGINE_STATE_BUFFERING,
	GV_ENGINE_STATE_PLAYING
} GvEngineState;

/* Methods */

GvEngine *gv_engine_new (void);
void      gv_engine_play(GvEngine *self, GvStation *station);
void      gv_engine_stop(GvEngine *self);

/* Property accessors */

GvEngineState  gv_engine_get_state           (GvEngine *self);
guint          gv_engine_get_bitrate         (GvEngine *self);
GvStreaminfo  *gv_engine_get_streaminfo      (GvEngine *self);
GvMetadata    *gv_engine_get_metadata        (GvEngine *self);
guint          gv_engine_get_volume          (GvEngine *self);
void           gv_engine_set_volume          (GvEngine *self, guint volume);
gboolean       gv_engine_get_mute            (GvEngine *self);
void           gv_engine_set_mute            (GvEngine *self, gboolean mute);
gboolean       gv_engine_get_pipeline_enabled(GvEngine *self);
void           gv_engine_set_pipeline_enabled(GvEngine *self, gboolean enabled);
const gchar   *gv_engine_get_pipeline_string (GvEngine *self);
void           gv_engine_set_pipeline_string (GvEngine *self, const gchar *pipeline);
