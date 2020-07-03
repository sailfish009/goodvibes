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

/**
 * SECTION:gv-streaminfo
 * @title: GvStreaminfo
 * @short_description: Technical information about a stream
 *
 * Streaminfo is a term that appears in the GStreamer documentation,
 * and that was reused here in Goodvibes. So you might want to read
 * the GStreamer documentation first:
 * <https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/tagging.html>
 *
 * Streaminfo are tags that describe the technical parts of stream content.
 *
 * A GvStreaminfo object is created and updated by GvEngine, according to the
 * data that is provided by GStreamer during the playback. More specifically,
 * some fields are updated from a list of Gstreamer tags, while others are
 * updated from the caps of the GStreamer audio pad.
 *
 * When a new stream is being played, a new GvStreaminfo is created. Then we
 * expect most of the fields of the GvStreaminfo to be settled after a few
 * seconds of playback, except for the bitrate fields. In particular, for a
 * variable bitrate stream, the bitrate will change constantly, every second or
 * so. The minimum and maximum bitrate are also updated when a new minimum or a
 * new maximum is reached.
 *
 * There's no data aggregation done in the various update() functions. It means
 * that zero (or NULL) is considered a valid value, rather than "unset". For
 * examples, if we update from GstTags, and the nominal bitrate value is zero
 * in those tags, then we update this value to zero in GvStreaminfo, rather
 * than considering than zero is unset, and not updating the value in
 * GvStreaminfo.
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

struct _GvStreaminfo {
	guint  bitrate;
	guint  channels;
	gchar *codec;
	guint  maximum_bitrate;
	guint  minimum_bitrate;
	guint  nominal_bitrate;
	guint  sample_rate;

	/*< private >*/
	volatile guint ref_count;
};

/*
 * Public methods
 */

guint
gv_streaminfo_get_bitrate(GvStreaminfo *self)
{
	return self->bitrate;
}

guint
gv_streaminfo_get_channels(GvStreaminfo *self)
{
	return self->channels;
}

const gchar *
gv_streaminfo_get_codec(GvStreaminfo *self)
{
	return self->codec;
}

guint
gv_streaminfo_get_maximum_bitrate(GvStreaminfo *self)
{
	return self->maximum_bitrate;
}

guint
gv_streaminfo_get_minimum_bitrate(GvStreaminfo *self)
{
	return self->minimum_bitrate;
}

guint
gv_streaminfo_get_nominal_bitrate(GvStreaminfo *self)
{
	return self->nominal_bitrate;
}

guint
gv_streaminfo_get_sample_rate(GvStreaminfo *self)
{
	return self->sample_rate;
}

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
gv_streaminfo_ref(GvStreaminfo *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(g_atomic_int_get(&self->ref_count) > 0, NULL);

	g_atomic_int_inc(&self->ref_count);

	return self;
}

GvStreaminfo *
gv_streaminfo_new(void)
{
	GvStreaminfo *self;

	self = g_new0(GvStreaminfo, 1);
	self->ref_count = 1;

	return self;
}
