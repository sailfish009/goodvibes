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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib-object.h>
#include <gst/gst.h>

/* GObject declarations */

#define GV_TYPE_METADATA gv_metadata_get_type()

GType gv_metadata_get_type(void) G_GNUC_CONST;

typedef struct _GvMetadata GvMetadata;

/* Methods */

GvMetadata *gv_metadata_new  (void);
GvMetadata *gv_metadata_ref  (GvMetadata *self);
void        gv_metadata_unref(GvMetadata *self);

#define gv_clear_metadata(object_ptr) \
	g_clear_pointer((object_ptr), gv_metadata_unref)

gboolean    gv_metadata_is_empty         (GvMetadata *self);
gboolean    gv_metadata_update_from_gst_taglist(GvMetadata *self, GstTagList *taglist);
gchar      *gv_metadata_make_title_artist(GvMetadata *self, gboolean escape);
gchar      *gv_metadata_make_album_year  (GvMetadata *self, gboolean escape);

const gchar *gv_metadata_get_album  (GvMetadata *self);
const gchar *gv_metadata_get_artist (GvMetadata *self);
const gchar *gv_metadata_get_comment(GvMetadata *self);
const gchar *gv_metadata_get_genre  (GvMetadata *self);
const gchar *gv_metadata_get_title  (GvMetadata *self);
const gchar *gv_metadata_get_year   (GvMetadata *self);
