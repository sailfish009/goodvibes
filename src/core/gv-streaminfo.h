/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020 Arnaud Rebillout
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
#include <gst/gst.h>

/* GObject declarations */

#define GV_TYPE_STREAMINFO gv_streaminfo_get_type()

GType gv_streaminfo_get_type(void) G_GNUC_CONST;

typedef struct _GvStreaminfo GvStreaminfo;

/* Methods */

GvStreaminfo *gv_streaminfo_new  (void);
GvStreaminfo *gv_streaminfo_ref  (GvStreaminfo *self);
void          gv_streaminfo_unref(GvStreaminfo *self);

#define gv_clear_streaminfo(object_ptr) \
	g_clear_pointer((object_ptr), gv_streaminfo_unref)

gboolean gv_streaminfo_update_from_gst_audio_pad(GvStreaminfo *self,
		                                 GstPad *audio_pad);
gboolean gv_streaminfo_update_from_gst_taglist  (GvStreaminfo *self,
		                                 GstTagList *taglist);

guint        gv_streaminfo_get_bitrate        (GvStreaminfo *self);
guint        gv_streaminfo_get_channels       (GvStreaminfo *self);
const gchar *gv_streaminfo_get_codec          (GvStreaminfo *self);
guint        gv_streaminfo_get_maximum_bitrate(GvStreaminfo *self);
guint        gv_streaminfo_get_minimum_bitrate(GvStreaminfo *self);
guint        gv_streaminfo_get_nominal_bitrate(GvStreaminfo *self);
guint        gv_streaminfo_get_sample_rate    (GvStreaminfo *self);
