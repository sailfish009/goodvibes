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
#include <gtk/gtk.h>

#include "framework/glib-object-additions.h"
#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gtk-additions.h"

#include "ui/gv-station-properties-box.h"

#define UI_RESOURCE_PATH GV_APPLICATION_PATH "/Ui/station-properties-box.glade"

/*
 * GObject definitions
 */

struct _GvProp {
	GtkWidget *title_label;
	GtkWidget *value_label;
	gboolean show_when_empty;
};

typedef struct _GvProp GvProp;

struct _GvStationPropertiesBoxPrivate {
	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *station_properties_box;
	/* Station */
	GtkWidget *stainfo_label;
	GtkWidget *stainfo_grid;
	GvProp name_prop;
	GvProp uri_prop;
	GvProp streams_prop;
	GvProp user_agent_prop;
	GvProp codec_prop;
	GvProp bitrate_prop;
	/* Metadata */
	GtkWidget *metadata_label;
	GtkWidget *metadata_grid;
	GvProp title_prop;
	GvProp artist_prop;
	GvProp album_prop;
	GvProp genre_prop;
	GvProp year_prop;
	GvProp comment_prop;
};

typedef struct _GvStationPropertiesBoxPrivate GvStationPropertiesBoxPrivate;

struct _GvStationPropertiesBox {
	/* Parent instance structure */
	GtkBox parent_instance;
	/* Private data */
	GvStationPropertiesBoxPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvStationPropertiesBox, gv_station_properties_box, GTK_TYPE_BOX)

/*
 * GvProp
 *
 * A simple abstraction to represent a property.
 */

static void
gv_prop_init(GvProp *prop, GtkBuilder *builder, const gchar *propname, gboolean show_when_empty)
{
	gchar *buf;

	buf = g_strdup_printf("%s_%s", propname, "title");
	prop->title_label = GTK_WIDGET(gtk_builder_get_object(builder, buf));
	g_free(buf);

	buf = g_strdup_printf("%s_%s", propname, "value");
	prop->value_label = GTK_WIDGET(gtk_builder_get_object(builder, buf));
	g_free(buf);

	prop->show_when_empty = show_when_empty;
}

static void
gv_prop_set(GvProp *prop, const gchar *text)
{
	gboolean visible = (text != NULL) || prop->show_when_empty;

	gtk_label_set_text(GTK_LABEL(prop->value_label), text);
	gtk_widget_set_visible(prop->title_label, visible);
	gtk_widget_set_visible(prop->value_label, visible);
}

/*
 * Helpers
 */

static gchar *
make_bitrate_string(guint bitrate, guint nominal_bitrate)
{
	GString *str;

	if (bitrate == 0 && nominal_bitrate == 0)
		return NULL;

	str = g_string_new(NULL);

	if (bitrate > 0)
		g_string_printf(str, "%u %s", bitrate, _("kpbs"));
	else
		g_string_printf(str, "%s", _("unknown"));

	if (nominal_bitrate > 0)
		/* TRANSLATORS: we are talking about 'nominal bitrate' here. */
		g_string_append_printf(str, " (%s: %u %s)", _("nominal"),
				nominal_bitrate, _("kbps"));

	return g_string_free(str, FALSE);
}

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

static void
set_stainfo(GvStationPropertiesBoxPrivate *priv, GvStation *station, guint bitrate)
{
	const gchar *text;
	gchar *str;
	guint nominal_bitrate;

	text = gv_station_get_name(station);
	gv_prop_set(&priv->name_prop, text);

	text = gv_station_get_uri(station);
	gv_prop_set(&priv->uri_prop, text);

	text = gv_station_get_user_agent(station);
	gv_prop_set(&priv->user_agent_prop, text);

	text = gv_station_get_codec(station);
	gv_prop_set(&priv->codec_prop, text);

	nominal_bitrate = gv_station_get_nominal_bitrate(station);
	str = make_bitrate_string(bitrate, nominal_bitrate);
	gv_prop_set(&priv->bitrate_prop, str);
	g_free(str);

	text = gv_station_get_first_stream_uri(station);
	if (text == NULL) {
		/* There's no stream uris to display */
		gv_prop_set(&priv->streams_prop, NULL);
	} else if (!(g_strcmp0(text, gv_station_get_uri(station)))) {
		/* If station uri and stream uri are the same, there's
		 * no need to display it. */
		gv_prop_set(&priv->streams_prop, NULL);
	} else {
		/* Let's display the list of stream uris then */
		GSList *stream_uris;
		stream_uris = gv_station_get_stream_uris(station);
		str = make_stream_uris_string(stream_uris);
		gv_prop_set(&priv->streams_prop, str);
		g_free(str);
	}
}

static void
unset_stainfo(GvStationPropertiesBoxPrivate *priv)
{
	gv_prop_set(&priv->name_prop, NULL);
	gv_prop_set(&priv->uri_prop, NULL);
	gv_prop_set(&priv->user_agent_prop, NULL);
	gv_prop_set(&priv->codec_prop, NULL);
	gv_prop_set(&priv->bitrate_prop, NULL);
	gv_prop_set(&priv->streams_prop, NULL);
}

static void
set_metadata(GvStationPropertiesBoxPrivate *priv, GvMetadata *metadata)
{
	const gchar *text;

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
unset_metadata(GvStationPropertiesBoxPrivate *priv)
{
	gv_prop_set(&priv->title_prop, NULL);
	gv_prop_set(&priv->artist_prop, NULL);
	gv_prop_set(&priv->album_prop, NULL);
	gv_prop_set(&priv->genre_prop, NULL);
	gv_prop_set(&priv->year_prop, NULL);
	gv_prop_set(&priv->comment_prop, NULL);
}

static void
gv_station_properties_update_stainfo(GvStationPropertiesBox *self, GvPlayer *player)
{
	GvStationPropertiesBoxPrivate *priv = self->priv;
	GvStation *station = gv_player_get_station(player);
	guint bitrate = gv_player_get_bitrate(player);

	if (station)
		set_stainfo(priv, station, bitrate);
	else
		unset_stainfo(priv);
}

static void
gv_station_properties_update_metadata(GvStationPropertiesBox *self, GvPlayer *player)
{
	GvStationPropertiesBoxPrivate *priv = self->priv;
	GvMetadata *metadata = gv_player_get_metadata(player);

	if (metadata) {
		set_metadata(priv, metadata);
		gtk_widget_set_visible(priv->metadata_label, TRUE);
		gtk_widget_set_visible(priv->metadata_grid, TRUE);
	} else {
		unset_metadata(priv);
		gtk_widget_set_visible(priv->metadata_label, FALSE);
		gtk_widget_set_visible(priv->metadata_grid, FALSE);
	}
}

/*
 * Signal handlers & callbacks
 */

static void
on_player_notify(GvPlayer *player, GParamSpec *pspec,
                 GvStationPropertiesBox *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%p, %s, %p", player, property_name, self);

	if (!g_strcmp0(property_name, "station") ||
	    !g_strcmp0(property_name, "bitrate"))
		gv_station_properties_update_stainfo(self, player);
	else if (!g_strcmp0(property_name, "metadata"))
		gv_station_properties_update_metadata(self, player);
}

static void
on_realize(GvStationPropertiesBox *self, gpointer user_data G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;

	g_signal_connect_object(player, "notify",
				G_CALLBACK(on_player_notify), self, 0);

	gv_station_properties_update_stainfo(self, player);
	gv_station_properties_update_metadata(self, player);
}

static void
on_unrealize(GvStationPropertiesBox *self, gpointer user_data G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;

	g_signal_handlers_disconnect_by_data(player, self);
}

/*
 * Public methods
 */

GtkWidget *
gv_station_properties_box_new(void)
{
	return g_object_new(GV_TYPE_STATION_PROPERTIES_BOX, NULL);
}

/*
 * Construct helpers
 */

static void
gv_station_properties_box_populate_widgets(GvStationPropertiesBox *self)
{
	GvStationPropertiesBoxPrivate *priv = self->priv;
	GtkBuilder *builder;

	/* Build the ui */
	builder = gtk_builder_new_from_resource(UI_RESOURCE_PATH);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, station_properties_box);

	/* Station */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, stainfo_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, stainfo_grid);
	gv_prop_init(&priv->name_prop, builder, "name", TRUE);
	gv_prop_init(&priv->uri_prop, builder, "uri", TRUE);
	gv_prop_init(&priv->streams_prop, builder, "streams", FALSE);
	gv_prop_init(&priv->user_agent_prop, builder, "user_agent", FALSE);
	gv_prop_init(&priv->codec_prop, builder, "codec", FALSE);
	gv_prop_init(&priv->bitrate_prop, builder, "bitrate", FALSE);

	/* Metadata */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, metadata_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, metadata_grid);
	gv_prop_init(&priv->title_prop, builder, "title", FALSE);
	gv_prop_init(&priv->artist_prop, builder, "artist", FALSE);
	gv_prop_init(&priv->album_prop, builder, "album", FALSE);
	gv_prop_init(&priv->genre_prop, builder, "genre", FALSE);
	gv_prop_init(&priv->year_prop, builder, "year", FALSE);
	gv_prop_init(&priv->comment_prop, builder, "comment", FALSE);

	/* Pack that within the box */
	gtk_container_add(GTK_CONTAINER(self), priv->station_properties_box);

	/* Cleanup */
	g_object_unref(builder);
}

/*
 * GObject methods
 */

static void
gv_station_properties_box_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station_properties_box, object);
}

static void
gv_station_properties_box_constructed(GObject *object)
{
	GvStationPropertiesBox *self = GV_STATION_PROPERTIES_BOX(object);

	TRACE("%p", object);

	/* Build the top-level widget */
	gv_station_properties_box_populate_widgets(self);

	/* Signals */
	g_signal_connect_object(self, "realize",
			        G_CALLBACK(on_realize), NULL, 0);
	g_signal_connect_object(self, "unrealize",
			        G_CALLBACK(on_unrealize), NULL, 0);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station_properties_box, object);
}

static void
gv_station_properties_box_init(GvStationPropertiesBox *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_properties_box_get_instance_private(self);
}

static void
gv_station_properties_box_class_init(GvStationPropertiesBoxClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_properties_box_finalize;
	object_class->constructed = gv_station_properties_box_constructed;
}
