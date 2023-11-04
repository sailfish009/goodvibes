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

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "base/glib-object-additions.h"
#include "base/gv-base.h"
#include "core/gv-core.h"
#include "ui/gtk-additions.h"

#include "ui/gv-station-view.h"
#include "ui/gv-ui-internal.h"

#define UI_RESOURCE_PATH GV_APPLICATION_PATH "/Ui/station-view.glade"

/* Hide playlist streams: I don't see a nice way to make it
 * fit in the UI, and who cares really, so just drop it.
 */
//#define SHOW_PLAYLIST_STREAMS

/*
 * Signal
 */

enum {
	SIGNAL_GO_BACK_CLICKED,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvProp {
	GtkWidget *title_label;
	GtkWidget *value_label;
};

typedef struct _GvProp GvProp;

struct _GvStationViewPrivate {

	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *station_view_box;
	/* Current station */
	GtkWidget *station_grid;
	GtkWidget *station_name_label;
	GtkWidget *playback_status_label;
	GtkWidget *go_back_button;
	/* Properties */
	GtkWidget *properties_grid;
	/* Playback errors */
	GtkWidget *error_label;
	GvProp error_message_prop;
	GvProp error_details_prop;
	/* Station properties */
	GtkWidget *stainfo_label;
	GvProp station_uri_prop;
	GvProp playlist_uri_prop;
	GvProp playlist_redirection_uri_prop;
	GvProp playlist_streams_prop;
	GvProp stream_uri_prop;
	GvProp stream_redirection_uri_prop;
	GvProp user_agent_prop;
	GvProp stream_type_prop;
	GvProp codec_prop;
	GvProp channels_prop;
	GvProp sample_rate_prop;
	GvProp bitrate_prop;
	/* Metadata */
	GtkWidget *metadata_label;
	GvProp title_prop;
	GvProp artist_prop;
	GvProp album_prop;
	GvProp genre_prop;
	GvProp year_prop;
	GvProp comment_prop;
};

typedef struct _GvStationViewPrivate GvStationViewPrivate;

struct _GvStationView {
	/* Parent instance structure */
	GtkBox parent_instance;
	/* Private data */
	GvStationViewPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStationView, gv_station_view, GTK_TYPE_BOX)

/*
 * GvProp
 *
 * A simple abstraction to represent a property.
 */

static void
gv_prop_init(GvProp *prop, GtkBuilder *builder, const gchar *propname)
{
	gchar *widget_name;
	GObject *object;
	GtkWidget *widget;
	GtkLabel *label;
	GtkStyleContext *style_context;

	widget_name = g_strdup_printf("%s_%s", propname, "title");
	object = gtk_builder_get_object(builder, widget_name);
	prop->title_label = GTK_WIDGET(object);
	g_free(widget_name);

	label = GTK_LABEL(object);
	gtk_label_set_xalign(label, 1);
	widget = GTK_WIDGET(object);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	style_context = gtk_widget_get_style_context(widget);
	gtk_style_context_add_class(style_context, "dim-label");

	widget_name = g_strdup_printf("%s_%s", propname, "value");
	object = gtk_builder_get_object(builder, widget_name);
	prop->value_label = GTK_WIDGET(object);
	g_free(widget_name);

	label = GTK_LABEL(object);
	gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
	gtk_label_set_xalign(label, 0);
	gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
}

static void
gv_prop_set(GvProp *prop, const gchar *text)
{
	gboolean visible = (text != NULL);

	gtk_label_set_text(GTK_LABEL(prop->value_label), text);
	gtk_widget_set_visible(prop->title_label, visible);
	gtk_widget_set_visible(prop->value_label, visible);
}

/*
 * Helpers
 */

static gchar *
make_bitrate_string(guint bitrate, guint maximum_bitrate, guint minimum_bitrate,
		    guint nominal_bitrate)
{
	GString *str;

	if (bitrate == 0 && maximum_bitrate == 0 &&
	    minimum_bitrate == 0 && nominal_bitrate == 0)
		return NULL;

	/* We want to display kilobits per second */
	bitrate /= 1000;
	maximum_bitrate /= 1000;
	minimum_bitrate /= 1000;
	nominal_bitrate /= 1000;

	str = g_string_new(NULL);

	if (bitrate > 0)
		g_string_printf(str, "%u %s", bitrate, _("kbps"));
	else
		g_string_printf(str, "%s", _("unknown"));

	if (nominal_bitrate > 0 && maximum_bitrate == 0 && minimum_bitrate == 0)
		/* TRANSLATORS: we talk about nominal bitrate here. */
		g_string_append_printf(str, " (%s: %u)", _("nominal"),
				       nominal_bitrate);
	else if (nominal_bitrate == 0 && (minimum_bitrate > 0 || maximum_bitrate > 0))
		/* TRANSLATORS: we talk about minimum and maximum bitrate here. */
		g_string_append_printf(str, " (%s: %u, %s: %u)",
				       _("min"), minimum_bitrate,
				       _("max"), maximum_bitrate);
	else if (nominal_bitrate > 0 && (minimum_bitrate > 0 || maximum_bitrate > 0))
		/* TRANSLATORS: we talk about nominal, minimum and maximum bitrate here. */
		g_string_append_printf(str, " (%s: %u, %s: %u, %s: %u)",
				       _("nominal"), nominal_bitrate,
				       _("min"), minimum_bitrate,
				       _("max"), maximum_bitrate);

	return g_string_free(str, FALSE);
}

static gchar *
make_channels_string(guint channels)
{
	gchar *str;

	switch (channels) {
	case 0:
		str = NULL;
		break;
	case 1:
		str = g_strdup(_("Mono"));
		break;
	case 2:
		str = g_strdup(_("Stereo"));
		break;
	default:
		str = g_strdup_printf("%u", channels);
		break;
	}

	return str;
}

static gchar *
make_stream_type_string(GvStreamType stream_type)
{
	const gchar *str;

	switch (stream_type) {
	case GV_STREAM_TYPE_HTTP:
		str = "HTTP";
		break;
	case GV_STREAM_TYPE_HTTP_ICY:
		str = "HTTP+ICY";
		break;
	case GV_STREAM_TYPE_HLS:
		str = "HLS";
		break;
	case GV_STREAM_TYPE_DASH:
		str = "MPEG-DASH";
		break;
	default:
		str = NULL;
		break;
	}

	return g_strdup(str);
}

static gchar *
make_sample_rate_string(guint sample_rate)
{
	gdouble rate;

	if (sample_rate == 0)
		return NULL;

	rate = sample_rate / 1000.0;

	return g_strdup_printf("%g %s", rate, _("kHz"));
}

#ifdef SHOW_PLAYLIST_STREAMS
static gchar *
make_stream_uris_string(GSList *stream_uris)
{
	GString *string;
	GSList *item;

	string = g_string_new(NULL);

	for (item = stream_uris; item; item = item->next) {
		const gchar *stream_uri = item->data;
		const gchar *prefix;

		/* We assume that the first stream is being played */
		if (item == stream_uris)
			prefix = "‣";
		else
			prefix = "\n";

		g_string_append_printf(string, "%s %s", prefix, stream_uri);
	}

	return g_string_free(string, FALSE);
}
#endif

static void
set_station(GvStationViewPrivate *priv, GvStation *station)
{
	const gchar *text;
	GvPlaylist *playlist;

	// XXX Binding properties would probably work better?

	g_return_if_fail(station != NULL);

	text = gv_station_get_name(station);
	if (text == NULL)
		text = "???";
	gtk_label_set_text(GTK_LABEL(priv->station_name_label), text);

	playlist = gv_station_get_playlist(station);
	if (playlist != NULL) {
		text = gv_playlist_get_uri(playlist);
		gv_prop_set(&priv->playlist_uri_prop, text);

		text = gv_playlist_get_redirection_uri(playlist);
		gv_prop_set(&priv->playlist_redirection_uri_prop, text);

#ifdef SHOW_PLAYLIST_STREAMS
		GSList *stream_uris;
		gchar *str;
		stream_uris = gv_playlist_get_stream_uris(playlist);
		str = make_stream_uris_string(stream_uris);
		gv_prop_set(&priv->playlist_streams_prop, str);
		g_free(str);
#else
		gv_prop_set(&priv->playlist_streams_prop, NULL);
#endif
	} else {
		gv_prop_set(&priv->playlist_uri_prop, NULL);
		gv_prop_set(&priv->playlist_redirection_uri_prop, NULL);
		gv_prop_set(&priv->playlist_streams_prop, NULL);
	}

	text = gv_station_get_stream_uri(station);
	gv_prop_set(&priv->stream_uri_prop, text);

	GvPlayer *player = gv_core_player;
	text = gv_player_get_redirection_uri(player);
	gv_prop_set(&priv->stream_redirection_uri_prop, text);

	text = gv_station_get_user_agent(station);
	gv_prop_set(&priv->user_agent_prop, text);

	/* We show the station uri as defined by user only when the station is
	 * not playing: at this stage, we don't know yet if this uri is for a
	 * playlist or a stream. However when the station is playing, we
	 * already show this uri, either as playlist uri, either as stream uri.
	 */
	if (playlist == NULL && gv_station_get_stream_uri(station) == NULL) {
		text = gv_station_get_uri(station);
		gv_prop_set(&priv->station_uri_prop, text);
	} else {
		gv_prop_set(&priv->station_uri_prop, NULL);
	}
}

static void
unset_station(GvStationViewPrivate *priv)
{
	gtk_label_set_text(GTK_LABEL(priv->station_name_label),
			   _("No station selected"));
	gv_prop_set(&priv->station_uri_prop, NULL);
	gv_prop_set(&priv->playlist_uri_prop, NULL);
	gv_prop_set(&priv->playlist_redirection_uri_prop, NULL);
	gv_prop_set(&priv->playlist_streams_prop, NULL);
	gv_prop_set(&priv->stream_uri_prop, NULL);
	gv_prop_set(&priv->stream_redirection_uri_prop, NULL);
	gv_prop_set(&priv->user_agent_prop, NULL);
}

static void
set_playback_status(GvStationViewPrivate *priv, GvPlaybackState state)
{
	GtkLabel *label = GTK_LABEL(priv->playback_status_label);
	const gchar *playback_state_str;

	playback_state_str = gv_playback_state_to_string(state);
	gtk_label_set_text(label, playback_state_str);
}

static void
set_playback_error(GvStationViewPrivate *priv, const gchar *message,
		const gchar *details)
{
	gv_prop_set(&priv->error_message_prop, message);
	gv_prop_set(&priv->error_details_prop, details);
}

static void
unset_playback_error(GvStationViewPrivate *priv)
{
	gv_prop_set(&priv->error_message_prop, NULL);
	gv_prop_set(&priv->error_details_prop, NULL);
}

static void
set_streaminfo(GvStationViewPrivate *priv, GvStreaminfo *streaminfo)
{
	GvStreamBitrate bitrate = { 0 };
	gchar *str;

	g_return_if_fail(streaminfo != NULL);

	gv_streaminfo_get_bitrate(streaminfo, &bitrate);
	str = make_bitrate_string(bitrate.current, bitrate.maximum,
				  bitrate.minimum, bitrate.nominal);
	gv_prop_set(&priv->bitrate_prop, str);
	g_free(str);

	str = make_channels_string(gv_streaminfo_get_channels(streaminfo));
	gv_prop_set(&priv->channels_prop, str);
	g_free(str);

	gv_prop_set(&priv->codec_prop, gv_streaminfo_get_codec(streaminfo));

	str = make_sample_rate_string(gv_streaminfo_get_sample_rate(streaminfo));
	gv_prop_set(&priv->sample_rate_prop, str);
	g_free(str);

	str = make_stream_type_string(gv_streaminfo_get_stream_type(streaminfo));
	gv_prop_set(&priv->stream_type_prop, str);
	g_free(str);
}

static void
unset_streaminfo(GvStationViewPrivate *priv)
{
	gv_prop_set(&priv->bitrate_prop, NULL);
	gv_prop_set(&priv->codec_prop, NULL);
	gv_prop_set(&priv->channels_prop, NULL);
	gv_prop_set(&priv->sample_rate_prop, NULL);
	gv_prop_set(&priv->stream_type_prop, NULL);
}

static void
set_metadata(GvStationViewPrivate *priv, GvMetadata *metadata)
{
	const gchar *text;

	g_return_if_fail(metadata != NULL);

	text = gv_metadata_get_title(metadata);
	gv_prop_set(&priv->title_prop, text);

	text = gv_metadata_get_artist(metadata);
	gv_prop_set(&priv->artist_prop, text);

	text = gv_metadata_get_album(metadata);
	gv_prop_set(&priv->album_prop, text);

	text = gv_metadata_get_genre(metadata);
	gv_prop_set(&priv->genre_prop, text);

	text = gv_metadata_get_year(metadata);
	gv_prop_set(&priv->year_prop, text);

	text = gv_metadata_get_comment(metadata);
	gv_prop_set(&priv->comment_prop, text);
}

static void
unset_metadata(GvStationViewPrivate *priv)
{
	gv_prop_set(&priv->title_prop, NULL);
	gv_prop_set(&priv->artist_prop, NULL);
	gv_prop_set(&priv->album_prop, NULL);
	gv_prop_set(&priv->genre_prop, NULL);
	gv_prop_set(&priv->year_prop, NULL);
	gv_prop_set(&priv->comment_prop, NULL);
}

/*
 * Private methods
 */

static void
gv_station_view_update_station(GvStationView *self, GvPlayer *player)
{
	GvStationViewPrivate *priv = self->priv;
	GvStation *station = gv_player_get_station(player);

	if (station)
		set_station(priv, station);
	else
		unset_station(priv);
}

static void
gv_station_view_update_playback_status(GvStationView *self, GvPlayer *player)
{
	GvStationViewPrivate *priv = self->priv;
	GvPlaybackState state = gv_player_get_playback_state(player);

	set_playback_status(priv, state);
}

static void
gv_station_view_update_playback_error(GvStationView *self, GvPlayer *player)
{
	GvStationViewPrivate *priv = self->priv;
	GvPlaybackError *error = gv_player_get_playback_error(player);

	if (error != NULL) {
		set_playback_error(priv, error->message, error->details);
		gtk_widget_set_visible(priv->error_label, TRUE);
	} else {
		unset_playback_error(priv);
		gtk_widget_set_visible(priv->error_label, FALSE);
	}
}

static void
gv_station_view_update_streaminfo(GvStationView *self, GvPlayer *player)
{
	GvStationViewPrivate *priv = self->priv;
	GvStreaminfo *streaminfo = gv_player_get_streaminfo(player);

	if (streaminfo)
		set_streaminfo(priv, streaminfo);
	else
		unset_streaminfo(priv);
}

static void
gv_station_view_update_metadata(GvStationView *self, GvPlayer *player)
{
	GvStationViewPrivate *priv = self->priv;
	GvMetadata *metadata = gv_player_get_metadata(player);

	if (metadata) {
		set_metadata(priv, metadata);
		gtk_widget_set_visible(priv->metadata_label, TRUE);
	} else {
		unset_metadata(priv);
		gtk_widget_set_visible(priv->metadata_label, FALSE);
	}
}

/*
 * Signal handlers & callbacks
 */

static void
on_player_notify(GvPlayer *player, GParamSpec *pspec,
		 GvStationView *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", player, property_name, self);

	if (!g_strcmp0(property_name, "station"))
		gv_station_view_update_station(self, player);
	else if (!g_strcmp0(property_name, "playback-state"))
		gv_station_view_update_playback_status(self, player);
	else if (!g_strcmp0(property_name, "playback-error"))
		gv_station_view_update_playback_error(self, player);
	else if (!g_strcmp0(property_name, "redirection-uri"))
		gv_station_view_update_station(self, player);
	else if (!g_strcmp0(property_name, "streaminfo"))
		gv_station_view_update_streaminfo(self, player);
	else if (!g_strcmp0(property_name, "metadata"))
		gv_station_view_update_metadata(self, player);
}

static void
on_go_back_button_clicked(GtkButton *button G_GNUC_UNUSED, GvStationView *self)
{
	g_signal_emit(self, signals[SIGNAL_GO_BACK_CLICKED], 0);
}

static void
on_map(GvStationView *self, gpointer user_data)
{
	GvPlayer *player = gv_core_player;

	TRACE("%p, %p", self, user_data);

	/* Connect player signal handlers */
	g_signal_connect_object(player, "notify",
				G_CALLBACK(on_player_notify), self, 0);

	/* Update widgets */
	gv_station_view_update_station(self, player);
	gv_station_view_update_playback_status(self, player);
	gv_station_view_update_playback_error(self, player);
	gv_station_view_update_streaminfo(self, player);
	gv_station_view_update_metadata(self, player);
}

static void
on_unmap(GvStationView *self, gpointer user_data G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;

	TRACE("%p, %p", self, user_data);

	/* Disconnect player signal handlers */
	g_signal_handlers_disconnect_by_data(player, self);
}

/*
 * Public methods
 */

GtkWidget *
gv_station_view_new(void)
{
	return g_object_new(GV_TYPE_STATION_VIEW, NULL);
}

/*
 * Construct helpers
 */

static void
gv_station_view_populate_widgets(GvStationView *self)
{
	GvStationViewPrivate *priv = self->priv;
	GtkBuilder *builder;

	/* Build the ui */
	builder = gtk_builder_new_from_resource(UI_RESOURCE_PATH);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_view_box);

	/* Station */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_name_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, playback_status_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, go_back_button);

	/* Properties */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, properties_grid);

	/* Playback errors */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, error_label);
	gv_prop_init(&priv->error_message_prop, builder, "error_message");
	gv_prop_init(&priv->error_details_prop, builder, "error_details");

	/* Station Properties */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, stainfo_label);
	gv_prop_init(&priv->station_uri_prop, builder, "station_uri");
	gv_prop_init(&priv->playlist_uri_prop, builder, "playlist_uri");
	gv_prop_init(&priv->playlist_redirection_uri_prop, builder, "playlist_redirection_uri");
	gv_prop_init(&priv->playlist_streams_prop, builder, "playlist_streams");
	gv_prop_init(&priv->stream_uri_prop, builder, "stream_uri");
	gv_prop_init(&priv->stream_redirection_uri_prop, builder, "stream_redirection_uri");
	gv_prop_init(&priv->user_agent_prop, builder, "user_agent");
	gv_prop_init(&priv->stream_type_prop, builder, "stream_type");
	gv_prop_init(&priv->codec_prop, builder, "codec");
	gv_prop_init(&priv->channels_prop, builder, "channels");
	gv_prop_init(&priv->sample_rate_prop, builder, "sample_rate");
	gv_prop_init(&priv->bitrate_prop, builder, "bitrate");

	/* Metadata */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, metadata_label);
	gv_prop_init(&priv->title_prop, builder, "title");
	gv_prop_init(&priv->artist_prop, builder, "artist");
	gv_prop_init(&priv->album_prop, builder, "album");
	gv_prop_init(&priv->genre_prop, builder, "genre");
	gv_prop_init(&priv->year_prop, builder, "year");
	gv_prop_init(&priv->comment_prop, builder, "comment");

	/* Pack that within the box */
	gtk_container_add(GTK_CONTAINER(self), priv->station_view_box);

	/* Cleanup */
	g_object_unref(builder);
}

static void
gv_station_view_setup_appearance(GvStationView *self)
{
	GvStationViewPrivate *priv = self->priv;

	g_object_set(priv->station_view_box,
		     "margin", GV_UI_MAIN_WINDOW_MARGIN,
		     "spacing", GV_UI_GROUP_SPACING,
		     NULL);
	g_object_set(priv->station_grid,
		     "column-spacing", GV_UI_ELEM_SPACING,
		     NULL);
	g_object_set(priv->properties_grid,
		     "column-spacing", GV_UI_COLUMN_SPACING,
		     "row-spacing", GV_UI_ELEM_SPACING,
		     "margin-start", GV_UI_WINDOW_MARGIN,
		     "margin-end", GV_UI_WINDOW_MARGIN,
		     "margin-bottom", GV_UI_WINDOW_MARGIN,
		     NULL);

	gtk_label_set_xalign(GTK_LABEL(priv->error_label), 1.0);
	gtk_label_set_xalign(GTK_LABEL(priv->stainfo_label), 1.0);
	gtk_label_set_xalign(GTK_LABEL(priv->metadata_label), 1.0);
}

static void
gv_station_view_setup_widgets(GvStationView *self)
{
	GvStationViewPrivate *priv = self->priv;

	/* Connect widgets signal handlers */
	g_signal_connect_object(priv->go_back_button, "clicked",
				G_CALLBACK(on_go_back_button_clicked), self, 0);

	/* Connect self signal handlers */
	g_signal_connect_object(self, "map", G_CALLBACK(on_map), NULL, 0);
	g_signal_connect_object(self, "unmap", G_CALLBACK(on_unmap), NULL, 0);
}

/*
 * GObject methods
 */

static void
gv_station_view_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station_view, object);
}

static void
gv_station_view_constructed(GObject *object)
{
	GvStationView *self = GV_STATION_VIEW(object);

	TRACE("%p", object);

	/* Build widget */
	gv_station_view_populate_widgets(self);
	gv_station_view_setup_appearance(self);
	gv_station_view_setup_widgets(self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station_view, object);
}

static void
gv_station_view_init(GvStationView *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_view_get_instance_private(self);
}

static void
gv_station_view_class_init(GvStationViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_view_finalize;
	object_class->constructed = gv_station_view_constructed;

	/* Signals */
	signals[SIGNAL_GO_BACK_CLICKED] =
		g_signal_new("go-back-clicked", G_OBJECT_CLASS_TYPE(class),
			     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);
}
