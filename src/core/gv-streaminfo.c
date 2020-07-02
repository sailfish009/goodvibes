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

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

#include "framework/gv-framework.h"

#include "core/gv-streaminfo.h"

/*
 * GObject definitions
 */

G_DEFINE_BOXED_TYPE(GvStreaminfo, gv_streaminfo,
		gv_streaminfo_ref, gv_streaminfo_unref);

/*
 * Public methods
 */

gboolean
gv_streaminfo_update_from_gst_audio_pad(GvStreaminfo *self, GstPad *audio_pad)
{
	GstCaps *caps;
	gint channels;
	gint sample_rate;
	gboolean changed = FALSE;

	g_return_val_if_fail(self != NULL, FALSE);

	caps = audio_pad ? gst_pad_get_current_caps(audio_pad) : NULL;

	if (caps) {
		GstStructure *s;
		// DEBUG("Caps: %s", gst_caps_to_string(caps));  // should be freed
		s = gst_caps_get_structure(caps, 0);
		gst_structure_get_int(s, "channels", &channels);
		gst_structure_get_int(s, "rate", &sample_rate);
		gst_caps_unref(caps);
	} else {
		channels = 0;
		sample_rate = 0;
	}

	if ((guint) channels != self->channels) {
		self->channels = channels;
		changed = TRUE;
	}

	if ((guint) sample_rate != self->sample_rate) {
		self->sample_rate = sample_rate;
		changed = TRUE;
	}

	return changed;
}

gboolean
gv_streaminfo_update_from_gst_taglist(GvStreaminfo *self, GstTagList *taglist)
{
	const gchar *codec = NULL;
	guint bitrate = 0;
	guint maximum_bitrate = 0;
	guint minimum_bitrate = 0;
	guint nominal_bitrate = 0;
	gboolean changed = FALSE;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(taglist != NULL, FALSE);

	gst_tag_list_peek_string_index(taglist, GST_TAG_AUDIO_CODEC, 0, &codec);
	gst_tag_list_get_uint_index(taglist, GST_TAG_BITRATE, 0, &bitrate);
	gst_tag_list_get_uint_index(taglist, GST_TAG_MAXIMUM_BITRATE, 0, &maximum_bitrate);
	gst_tag_list_get_uint_index(taglist, GST_TAG_MINIMUM_BITRATE, 0, &minimum_bitrate);
	gst_tag_list_get_uint_index(taglist, GST_TAG_NOMINAL_BITRATE, 0, &nominal_bitrate);

	if (g_strcmp0(codec, self->codec)) {
		g_free(self->codec);
		self->codec = g_strdup(codec);
		changed = TRUE;
	}

	if (bitrate != self->bitrate) {
		self->bitrate = bitrate;
		changed = TRUE;
	}

	if (maximum_bitrate != self->maximum_bitrate) {
		self->maximum_bitrate = maximum_bitrate;
		changed = TRUE;
	}

	if (minimum_bitrate != self->minimum_bitrate) {
		self->minimum_bitrate = minimum_bitrate;
		changed = TRUE;
	}

	if (nominal_bitrate != self->nominal_bitrate) {
		self->nominal_bitrate = nominal_bitrate;
		changed = TRUE;
	}

	return changed;
}

GvStreaminfo *
gv_streaminfo_ref(GvStreaminfo *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(g_atomic_int_get(&self->ref_count) > 0, NULL);

	g_atomic_int_inc(&self->ref_count);

	return self;
}

void
gv_streaminfo_unref(GvStreaminfo *self)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(g_atomic_int_get(&self->ref_count) > 0);

	if (!g_atomic_int_dec_and_test(&self->ref_count))
		return;

	g_free(self->codec);
	g_free(self);
}

GvStreaminfo *
gv_streaminfo_new(void)
{
	GvStreaminfo *self;

	self = g_new0(GvStreaminfo, 1);
	self->ref_count = 1;

	return self;
}
