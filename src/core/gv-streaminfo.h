/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2020-2021 Arnaud Rebillout
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
#include <gst/gst.h>

/* GObject declarations */

#define GV_TYPE_STREAMINFO gv_streaminfo_get_type()

GType gv_streaminfo_get_type(void) G_GNUC_CONST;

typedef struct _GvStreaminfo GvStreaminfo;

typedef struct _GvStreamBitrate GvStreamBitrate;

struct _GvStreamBitrate {
	guint current;
	guint maximum;
	guint minimum;
	guint nominal;
};

typedef enum {
	GV_STREAM_TYPE_UNKNOWN = 0,
	GV_STREAM_TYPE_HTTP,
	GV_STREAM_TYPE_HTTP_ICY,
	GV_STREAM_TYPE_HLS,
	GV_STREAM_TYPE_DASH,
} GvStreamType;

/* Methods */

GvStreaminfo *gv_streaminfo_new  (void);
GvStreaminfo *gv_streaminfo_ref  (GvStreaminfo *self);
void          gv_streaminfo_unref(GvStreaminfo *self);

#define gv_clear_streaminfo(object_ptr) \
	g_clear_pointer((object_ptr), gv_streaminfo_unref)

gboolean gv_streaminfo_update_from_element_setup(GvStreaminfo *self,
		                                 GstElement *element);
gboolean gv_streaminfo_update_from_gst_caps     (GvStreaminfo *self,
		                                 GstCaps *caps);
gboolean gv_streaminfo_update_from_gst_taglist  (GvStreaminfo *self,
		                                 GstTagList *taglist);

void         gv_streaminfo_get_bitrate        (GvStreaminfo *self,
                                               GvStreamBitrate *bitrate);
guint        gv_streaminfo_get_channels       (GvStreaminfo *self);
const gchar *gv_streaminfo_get_codec          (GvStreaminfo *self);
guint        gv_streaminfo_get_sample_rate    (GvStreaminfo *self);
GvStreamType gv_streaminfo_get_stream_type    (GvStreaminfo *self);
