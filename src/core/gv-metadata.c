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

/**
 * SECTION:gv-metadata
 * @title: GvMetadata
 * @short_description: Non-technical information about a stream
 *
 * Metadata is a term that appears in the GStreamer documentation,
 * so you might want to read the GStreamer documentation first:
 * <https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/tagging.html>
 *
 * Metadata are tags that describe the non-technical parts of stream content.
 *
 * A GvMetadata object is created and updated by GvEngine, according to GStreamer
 * tags. That's all.
 */

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

#include "framework/gv-framework.h"

#include "core/gv-metadata.h"

/*
 * GObject definitions
 */

G_DEFINE_BOXED_TYPE(GvMetadata, gv_metadata,
		gv_metadata_ref, gv_metadata_unref);

struct _GvMetadata {
	gchar *album;
	gchar *artist;
	gchar *comment;
	gchar *genre;
	gchar *title;
	gchar *year;
	// WISHED Label
	// WISHED Cover
	// WISHED Organization

	/*< private >*/
	volatile guint ref_count;
};

/*
 * Public methods
 */

const gchar *
gv_metadata_get_album(GvMetadata *self)
{
	return self->album;
}

const gchar *
gv_metadata_get_artist(GvMetadata *self)
{
	return self->artist;
}

const gchar *
gv_metadata_get_comment(GvMetadata *self)
{
	return self->comment;
}

const gchar *
gv_metadata_get_genre(GvMetadata *self)
{
	return self->genre;
}

const gchar *
gv_metadata_get_title(GvMetadata *self)
{
	return self->title;
}

const gchar *
gv_metadata_get_year(GvMetadata *self)
{
	return self->year;
}

gchar *
gv_metadata_make_title_artist(GvMetadata *self, gboolean escape)
{
	gchar *str;

	g_return_val_if_fail(self != NULL, NULL);

	if (self->artist && self->title)
		str = g_strdup_printf("%s - %s", self->title, self->artist);
	else if (self->title)
		str = g_strdup_printf("%s", self->title);
	else if (self->artist)
		str = g_strdup_printf("%s", self->artist);
	else
		str = NULL;

	if (str && escape == TRUE) {
		gchar *str2 = g_markup_escape_text(str, -1);
		g_free(str);
		str = str2;
	}

	return str;
}

gchar *
gv_metadata_make_album_year(GvMetadata *self, gboolean escape)
{
	gchar *str;

	g_return_val_if_fail(self != NULL, NULL);

	if (self->album && self->year)
		str = g_strdup_printf("%s (%s)", self->album, self->year);
	else if (self->album)
		str = g_strdup_printf("%s", self->album);
	else if (self->year)
		str = g_strdup_printf("(%s)", self->year);
	else
		str = NULL;

	if (str && escape == TRUE) {
		gchar *str2 = g_markup_escape_text(str, -1);
		g_free(str);
		str = str2;
	}

	return str;
}

gboolean
gv_metadata_is_equal(GvMetadata *self1, GvMetadata *self2)
{
	if (self1 == self2)
		return TRUE;

	if (self1 == NULL || self2 == NULL)
		return FALSE;

	if (g_strcmp0(self1->album, self2->album))
		return FALSE;

	if (g_strcmp0(self1->artist, self2->artist))
		return FALSE;

	if (g_strcmp0(self1->comment, self2->comment))
		return FALSE;

	if (g_strcmp0(self1->genre, self2->genre))
		return FALSE;

	if (g_strcmp0(self1->title, self2->title))
		return FALSE;

	if (g_strcmp0(self1->year, self2->year))
		return FALSE;

	return TRUE;
}

void
gv_metadata_unref(GvMetadata *self)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(g_atomic_int_get(&self->ref_count) > 0);

	if (!g_atomic_int_dec_and_test(&self->ref_count))
		return;

	g_free(self->album);
	g_free(self->artist);
	g_free(self->comment);
	g_free(self->genre);
	g_free(self->title);
	g_free(self->year);
	g_free(self);
}

GvMetadata *
gv_metadata_ref(GvMetadata *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(g_atomic_int_get(&self->ref_count) > 0, NULL);

	g_atomic_int_inc(&self->ref_count);

	return self;
}

GvMetadata *
gv_metadata_new_from_gst_taglist(GstTagList *taglist)
{
	GvMetadata *self;
	GDate *date = NULL;

	self = gv_metadata_new();

	gst_tag_list_get_string(taglist, GST_TAG_ALBUM, &self->album);
	gst_tag_list_get_string(taglist, GST_TAG_ARTIST, &self->artist);
	gst_tag_list_get_string(taglist, GST_TAG_COMMENT, &self->comment);
	gst_tag_list_get_string(taglist, GST_TAG_GENRE, &self->genre);
	gst_tag_list_get_string(taglist, GST_TAG_TITLE, &self->title);

	gst_tag_list_get_date_index(taglist, GST_TAG_DATE, 0, &date);
	if (date) {
		self->year = g_date_valid(date) ?
			g_strdup_printf("%d", g_date_get_year(date)) :
			NULL;
		g_date_free(date);
	}

	return self;
}

GvMetadata *
gv_metadata_new(void)
{
	GvMetadata *self;

	self = g_new0(GvMetadata, 1);
	self->ref_count = 1;

	return self;
}
