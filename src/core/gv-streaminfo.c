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

/**
 * SECTION:gv-streaminfo
 * @title: GvStreaminfo
 * @short_description: Technical information about a stream
 *
 * Streaminfo is a term that appears in the GStreamer documentation, and that
 * we reuse here in Goodvibes. So it might be useful to read the GStreamer
 * documentation first:
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
 * In practice, I'm not always sure how to best handle incoming information
 * from GStreamer. For example, for a field of type string, when it's NULL, do
 * we unset whatever we used to know about this field? Or do we consider that
 * NULL means 'no information', and then skip this field?
 *
 * Another example: when GvStreaminfo is updated from GstTags, and the nominal
 * bitrate value is zero in those tags, what do we do? Answer: we update the
 * value to zero in GvStreaminfo, therefore considering zero a valid value,
 * rather than meaning 'no information'. Is it the right thing to do? Maybe.
 */

#include <glib-object.h>
#include <glib.h>
#include <gst/gst.h>

#include "base/gv-base.h"

#include "core/gv-streaminfo.h"

/*
 * GObject definitions
 */

G_DEFINE_BOXED_TYPE(GvStreaminfo, gv_streaminfo,
		    gv_streaminfo_ref, gv_streaminfo_unref)

struct _GvStreaminfo {
	GvStreamBitrate bitrate;
	guint channels;
	gchar *codec;
	guint sample_rate;
	GvStreamType stream_type;

	/*< private >*/
	volatile guint ref_count;
};

/*
 * Public methods
 */

void
gv_streaminfo_get_bitrate(GvStreaminfo *self, GvStreamBitrate *bitrate)
{
	g_return_if_fail(bitrate != NULL);

	*bitrate = self->bitrate;
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
gv_streaminfo_get_sample_rate(GvStreaminfo *self)
{
	return self->sample_rate;
}

GvStreamType
gv_streaminfo_get_stream_type(GvStreaminfo *self)
{
	return self->stream_type;
}

gboolean
gv_streaminfo_update_from_element_setup(GvStreaminfo *self, GstElement *element)
{
	GList *pads;
	GvStreamType type = GV_STREAM_TYPE_UNKNOWN;
	gboolean changed = FALSE;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(element != NULL, FALSE);

	pads = gst_element_get_pad_template_list(element);
	for (; pads != NULL; pads = g_list_next(pads)) {
		GstPadTemplate *pad_template;
		GstCaps *caps;
		guint i;

		pad_template = (GstPadTemplate *) (pads->data);
		if (pad_template->direction != GST_PAD_SINK)
			continue;

		caps = gst_pad_template_get_caps(pad_template);

		/* Too noisy */
		//gchar *caps_str = gst_caps_to_string(caps);
		//DEBUG("Caps: %s", caps_str);
		//g_free(caps_str);

		for (i = 0; i < gst_caps_get_size(caps); i++) {
			GstStructure *structure;
			const gchar *struct_name;

			structure = gst_caps_get_structure(caps, i);
			struct_name = gst_structure_get_name(structure);

			if (!g_strcmp0(struct_name, "application/x-icy")) {
				//DEBUG("HTTP+Icy stream");
				type = GV_STREAM_TYPE_HTTP_ICY;
				break;
			} else if (!g_strcmp0(struct_name, "application/x-hls")) {
				//DEBUG("HLS stream");
				type = GV_STREAM_TYPE_HLS;
				break;
			} else if (!g_strcmp0(struct_name, "application/dash+xml")) {
				//DEBUG("DASH stream");
				type = GV_STREAM_TYPE_DASH;
				break;
			}
		}
		gst_caps_unref(caps);
	}

	if (type != GV_STREAM_TYPE_UNKNOWN && type != self->stream_type) {
		self->stream_type = type;
		changed = TRUE;
	}

	return changed;
}

gboolean
gv_streaminfo_update_from_gst_caps(GvStreaminfo *self, GstCaps *caps)
{
	GstStructure *s;
	gchar *caps_str;
	gint channels = 0;
	gint sample_rate = 0;
	gboolean changed = FALSE;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(caps != NULL, FALSE);

	caps_str = gst_caps_to_string(caps);
	DEBUG("Caps: %s", caps_str);
	g_free(caps_str);

	s = gst_caps_get_structure(caps, 0);
	gst_structure_get_int(s, "channels", &channels);
	gst_structure_get_int(s, "rate", &sample_rate);

	if ((guint) channels != self->channels) {
		self->channels = channels;
		changed = TRUE;
	}

	if ((guint) sample_rate != self->sample_rate) {
		self->sample_rate = sample_rate;
		changed = TRUE;
	}

	/* We don't have a good way to detect if the stream type is HTTP,
	 * so what we do instead is to assume that if the stream type is
	 * still unknown at this point, then let it be HTTP. It might be
	 * updated later if we know better.
	 */
	if (self->stream_type == GV_STREAM_TYPE_UNKNOWN) {
		self->stream_type = GV_STREAM_TYPE_HTTP;
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

	/* If codec is NULL, skip it, do NOT set self->codec to NULL. I have a
	 * opus stream here, for which we pass by this function twice, and
	 * codec is NULL the second time. Here's the URL of the stream:
	 * http://stream.radioreklama.bg/nrj.opus
	 */
	if (codec != NULL && g_strcmp0(codec, self->codec) != 0) {
		g_free(self->codec);
		self->codec = g_strdup(codec);
		changed = TRUE;
	}

	if (bitrate != self->bitrate.current) {
		self->bitrate.current = bitrate;
		changed = TRUE;
	}

	if (maximum_bitrate != self->bitrate.maximum) {
		self->bitrate.maximum = maximum_bitrate;
		changed = TRUE;
	}

	if (minimum_bitrate != self->bitrate.minimum) {
		self->bitrate.minimum = minimum_bitrate;
		changed = TRUE;
	}

	if (nominal_bitrate != self->bitrate.nominal) {
		self->bitrate.nominal = nominal_bitrate;
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
