/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2018 Arnaud Rebillout
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

#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "framework/glib-object-additions.h"
#include "framework/gv-framework.h"

#include "core/gv-station-list.h"

// WISHED Try with a huge number of stations to see how it behaves.
//        It might be slow. The implementation never tried to be fast.

/*
 * FIP <https://www.fip.fr/>
 * Just the best radios you'll ever listen to.
 */

#define FIP_STATIONS \
        "<Station>" \
        "  <name>FIP Paris</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-midfi.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Autour du Rock</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio1.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Autour du Jazz</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio2.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Autour du Groove</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio3.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Autour du Monde</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio4.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Tout nouveau, tout FIP</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio5.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Autour du Reggae</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio6.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>FIP Autour de l'Électro</name>" \
        "  <uri>https://direct.fipradio.fr/live/fip-webradio8.mp3</uri>" \
        "</Station>"

/*
 * Nova <http://www.nova.fr/>
 * Another killer radio from France.
 */

#define NOVA_STATIONS \
        "<Station>" \
        "  <name>Nova</name>" \
        "  <uri>http://novazz.ice.infomaniak.ch/novazz-128.mp3</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>Nova Vintage</name>" \
        "  <uri>http://nova-vnt.ice.infomaniak.ch/nova-vnt-128</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>Nova la Nuit</name>" \
        "  <uri>http://nova-ln.ice.infomaniak.ch/nova-ln-128</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>Nova V.F.</name>" \
        "  <uri>http://nova-vf.ice.infomaniak.ch/nova-vf-128</uri>" \
        "</Station>"

/*
 * More of my favorite french radios.
 * - Grenouille <http://www.radiogrenouille.com/>
 */

#define FRENCH_STATIONS   \
        "<Station>" \
        "  <name>Radio Grenouille</name>" \
        "  <uri>http://live.radiogrenouille.com/live</uri>" \
        "</Station>"

/*
 * Broken stations.
 */

#define TESTING_BROKEN_STATIONS   \
        "<Station>" \
        "  <name>Broken - FIP old url</name>" \
        "  <uri>http://audio.scdn.arkena.com/11016/fip-midfi128.mp3</uri>" \
        "</Station>"

/*
 * Various playlist formats.
 * - Swiss Internet Radio (Public Domain Radio) <http://www.swissradio.ch/>
 */

#define TESTING_PLAYLIST_STATIONS         \
        "<Station>" \
        "  <name>M3U - Swiss Internet Radio Classical</name>" \
        "  <uri>http://www.swissradio.ch/streams/6034.m3u</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>ASX - Swiss Internet Radio Classical</name>" \
        "  <uri>http://www.swissradio.ch/streams/6034.asx</uri>" \
        "</Station>" \
        "<Station>" \
        "  <name>RAM - Swiss Internet Radio Classical</name>" \
        "  <uri>http://www.swissradio.ch/streams/6034.ram</uri>" \
        "</Station>"

/*
 * More radios, for testing.
 */

#define TESTING_MORE_STATIONS     \
        "<Station>" \
        "  <uri>http://www.netradio.fr:8000/A0RemzouilleRadio.xspf</uri>" \
        "</Station>" \
        "<Station>" \
        "  <uri>http://player.100p.nl/livestream.asx</uri>" \
        "</Station>" \
        "<Station>" \
        "  <uri>http://vt-net.org/WebRadio/live/8056.m3u</uri>" \
        "</Station>" \
        "<Station>" \
        "  <uri>'http://www.neradio.se/listen.pls</uri>" \
        "</Station>"

/*
 * Default station list, loaded if no station list file is found
 */

#define DEFAULT_STATIONS_DEV \
        FIP_STATIONS \
	NOVA_STATIONS \
        FRENCH_STATIONS \
        TESTING_BROKEN_STATIONS \
        TESTING_PLAYLIST_STATIONS \
        TESTING_MORE_STATIONS

#define DEFAULT_STATIONS_PROD \
        FIP_STATIONS \
	NOVA_STATIONS \
        FRENCH_STATIONS

#define DEFAULT_STATION_LIST \
        "<Stations>" \
        DEFAULT_STATIONS_PROD \
        "</Stations>"

/*
 * More defines...
 */

#define SAVE_DELAY 1 // how long to wait before writing changes to disk
#define STATION_LIST_FILE "stations.xml" // where to write the stations

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_LOAD_PATH,
	PROP_SAVE_PATH,
	/* Number of properties */
	PROP_N,
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

enum {
	SIGNAL_LOADED,
	SIGNAL_STATION_ADDED,
	SIGNAL_STATION_REMOVED,
	SIGNAL_STATION_MODIFIED,
	SIGNAL_STATION_MOVED,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

/*
 * GObject definitions
 */

struct _GvStationListPrivate {
	/* Paths */
	gchar **default_load_paths;
	gchar  *default_save_path;
	gchar  *load_path;
	gchar  *save_path;
	/* Timeout id, > 0 if a save operation is scheduled */
	guint   save_source_id;
	/* Ordered list of stations */
	GList  *stations;
	/* Shuffled list of stations, automatically created
	 * and destroyed when needed.
	 */
	GList  *shuffled;
};

typedef struct _GvStationListPrivate GvStationListPrivate;

struct _GvStationList {
	/* Parent instance structure */
	GObject               parent_instance;
	/* Private data */
	GvStationListPrivate *priv;
};

G_DEFINE_TYPE_WITH_CODE(GvStationList, gv_station_list, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(GvStationList)
                        G_IMPLEMENT_INTERFACE(GV_TYPE_ERRORABLE, NULL))

/*
 * Helpers
 */

static gpointer
copy_func_object_ref(gconstpointer src, gpointer data G_GNUC_UNUSED)
{
	return g_object_ref((gpointer) src);
}

/*
 * Markup handling
 */

struct _GvMarkupParsing {
	/* Persistent during the whole parsing process */
	GList  *list;
	/* Current iteration */
	gchar **cur;
	gchar  *name;
	gchar  *uri;
	gchar  *user_agent;
};

typedef struct _GvMarkupParsing GvMarkupParsing;

static void
markup_on_text(GMarkupParseContext  *context G_GNUC_UNUSED,
               const gchar          *text,
               gsize                 text_len G_GNUC_UNUSED,
               gpointer              user_data,
               GError              **error G_GNUC_UNUSED)
{
	GvMarkupParsing *parsing = user_data;

	/* Happens all the time */
	if (parsing->cur == NULL)
		return;

	/* Should never happen */
	g_assert_null(*parsing->cur);

	/* Save text */
	*parsing->cur = g_strdup(text);

	/* Cleanup */
	parsing->cur = NULL;
}

static void
markup_on_end_element(GMarkupParseContext  *context G_GNUC_UNUSED,
                      const gchar          *element_name G_GNUC_UNUSED,
                      gpointer              user_data,
                      GError              **error G_GNUC_UNUSED)
{
	GvMarkupParsing *parsing = user_data;
	GvStation *station;

	/* We only care when we leave a station node */
	if (g_strcmp0(element_name, "Station"))
		return;

	/* Discard stations with no uri */
	if (parsing->uri == NULL || parsing->uri[0] == '\0') {
		DEBUG("Encountered station without uri (named '%s')", parsing->name);
		goto cleanup;
	}

	/* Create a new station */
	station = gv_station_new(parsing->name, parsing->uri);
	if (parsing->user_agent)
		gv_station_set_user_agent(station, parsing->user_agent);

	/* We must take ownership right now */
	g_object_ref_sink(station);

	/* Add to list, use prepend for efficiency */
	parsing->list = g_list_prepend(parsing->list, station);

cleanup:
	/* Cleanup */
	g_free(parsing->name);
	g_free(parsing->uri);
	g_free(parsing->user_agent);
	parsing->name = NULL;
	parsing->uri = NULL;
	parsing->user_agent = NULL;
}

static void
markup_on_start_element(GMarkupParseContext  *context G_GNUC_UNUSED,
                        const gchar          *element_name,
                        const gchar         **attribute_names G_GNUC_UNUSED,
                        const gchar         **attribute_values G_GNUC_UNUSED,
                        gpointer              user_data,
                        GError              **error G_GNUC_UNUSED)
{
	GvMarkupParsing *parsing = user_data;

	/* Expected top node */
	if (!g_strcmp0(element_name, "Stations"))
		return;

	/* Entering a station node */
	if (!g_strcmp0(element_name, "Station")) {
		g_assert_null(parsing->name);
		g_assert_null(parsing->uri);
		g_assert_null(parsing->user_agent);
		return;
	}

	/* Name property */
	if (!g_strcmp0(element_name, "name")) {
		g_assert_null(parsing->cur);
		parsing->cur = &parsing->name;
		return;
	}

	/* Uri property */
	if (!g_strcmp0(element_name, "uri")) {
		g_assert_null(parsing->cur);
		parsing->cur = &parsing->uri;
		return;
	}

	/* User-agent property */
	if (!g_strcmp0(element_name, "user-agent")) {
		g_assert_null(parsing->user_agent);
		parsing->cur = &parsing->user_agent;
		return;
	}

	WARNING("Unexpected element: '%s'", element_name);
}

static void
markup_on_error(GMarkupParseContext *context G_GNUC_UNUSED,
                GError              *error   G_GNUC_UNUSED,
                gpointer             user_data)
{
	GvMarkupParsing *parsing = user_data;

	parsing->cur = NULL;
	g_free(parsing->name);
	parsing->name = NULL;
	g_free(parsing->uri);
	parsing->uri = NULL;
	g_free(parsing->user_agent);
	parsing->user_agent = NULL;
}

static gboolean
parse_markup(const gchar *text, GList **list, GError **err)
{
	GMarkupParseContext *context;
	GMarkupParser parser = {
		markup_on_start_element,
		markup_on_end_element,
		markup_on_text,
		NULL,
		markup_on_error,
	};
	GvMarkupParsing parsing = {
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};
	gboolean ret;

	g_return_val_if_fail(list != NULL, FALSE);
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	context = g_markup_parse_context_new(&parser, 0, &parsing, NULL);
	ret = g_markup_parse_context_parse(context, text, -1, err);
	g_markup_parse_context_free(context);

	if (ret == FALSE) {
		g_assert(err == NULL || *err != NULL);
		return FALSE;
	}

	*list = g_list_reverse(parsing.list);
	return TRUE;
}

static GString *
g_string_append_markup_tag_escaped(GString *string, const gchar *tag, const gchar *value)
{
	gchar *escaped;

	escaped = g_markup_printf_escaped("    <%s>%s</%s>\n", tag, value, tag);
	g_string_append(string, escaped);
	g_free(escaped);

	return string;
}

static gchar *
print_markup_station(GvStation *station)
{
	const gchar *name = gv_station_get_name(station);
	const gchar *uri = gv_station_get_uri(station);
	const gchar *user_agent = gv_station_get_user_agent(station);
	GString *string;

	/* A station is supposed to have an uri */
	if (uri == NULL) {
		WARNING("Station (%s) has no uri !", name);
		return NULL;
	}

	/* Write station in markup fashion */
	string = g_string_new("  <Station>\n");

	if (uri)
		g_string_append_markup_tag_escaped(string, "uri", uri);

	if (name)
		g_string_append_markup_tag_escaped(string, "name", name);

	if (user_agent)
		g_string_append_markup_tag_escaped(string, "user-agent", user_agent);

	g_string_append(string, "  </Station>\n");

	/* Return */
	return g_string_free(string, FALSE);
}

static gboolean
print_markup(GList *list, gchar **markup, GError **err)
{
	GList *item;
	GString *string;

	g_return_val_if_fail(markup != NULL, FALSE);
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	string = g_string_new(NULL);
	g_string_append(string, "<Stations>\n");

	for (item = list; item; item = item->next) {
		GvStation *station = GV_STATION(item->data);
		gchar *text;

		text = print_markup_station(station);
		if (text == NULL)
			continue;

		g_string_append(string, text);
		g_free(text);
	}

	g_string_append(string, "</Stations>");
	*markup = g_string_free(string, FALSE);

	return TRUE;
}

/*
 * File I/O
 */

static gboolean
load_station_list_from_string(const gchar *text, GList **list, GError **error)
{
	return parse_markup(text, list, error);
}

static gboolean
load_station_list_from_file(const gchar *path, GList **list, GError **error)
{
	gchar *text = NULL;
	gboolean ret;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	ret = g_file_get_contents(path, &text, NULL, error);
	if (ret == FALSE) {
		g_assert (error == NULL || *error != NULL);
		goto end;
	}

	ret = load_station_list_from_string(text, list, error);
	if (ret == FALSE) {
		g_assert(error == NULL || *error != NULL);
		goto end;
	}

end:
	g_free(text);
	return ret;
}

static gboolean
save_station_list_to_string(GList *list, gchar **text, GError **error)
{
	return print_markup(list, text, error);
}

static gboolean
save_station_list_to_file(GList *list, const gchar *path, GError **error)
{
	gboolean ret;
	gchar *text = NULL;
	gchar *dirname = NULL;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	ret = save_station_list_to_string(list, &text, error);
	if (ret == FALSE) {
		g_assert(error == NULL || *error != NULL);
		goto end;
	}

	/* g_file_set_contents() doesn't work with /dev/null, due to
	 * the way it's implemented (see documentation for details).
	 * However it's convenient to set the save path to /dev/null
	 * when we actually don't want to save (ie. test suite).
	 * So we handle this special case explicitly here.
	 */
	if (!g_strcmp0(path, "/dev/null")) {
		ret = TRUE;
		goto end;
	}

	dirname = g_path_get_dirname(path);
	if (g_mkdir_with_parents(dirname, S_IRWXU) != 0) {
		g_set_error(error, G_FILE_ERROR,
		            g_file_error_from_errno(errno),
		            "Failed to make directory: %s", g_strerror(errno));
		ret = FALSE;
		goto end;
	}

	ret = g_file_set_contents(path, text, -1, error);
	if (ret == FALSE) {
		g_assert(error == NULL || *error != NULL);
		goto end;
	}

end:
	g_free(dirname);
	g_free(text);
	return ret;
}

static void
relocate_station_list_file_for_4_1(const gchar *new_file)
{
	const gchar *user_config_dir;
	gchar *old_file = NULL;
	gchar *dirname = NULL;

	user_config_dir = gv_get_app_user_config_dir();
	old_file = g_build_filename(user_config_dir, "stations", NULL);

	if (!g_file_test(old_file, G_FILE_TEST_EXISTS))
		goto cleanup;

	INFO("Station list migration: '%s' > '%s'", old_file, new_file);

	dirname = g_path_get_dirname(new_file);
	if (g_mkdir_with_parents(dirname, S_IRWXU) != 0) {
		WARNING("Failed to make directory '%s': %s",
			dirname, strerror(errno));
		goto cleanup;
	}
	g_free(dirname);

	if (g_rename(old_file, new_file) != 0) {
		WARNING("Failed to rename file '%s' to '%s': %s",
			old_file, new_file, strerror(errno));
		goto cleanup;
	}

	dirname = g_path_get_dirname(old_file);
	g_rmdir(dirname);

 cleanup:
	g_free(dirname);
	g_free(old_file);
}

/*
 * Iterator implementation
 */

struct _GvStationListIter {
	GList *head, *item;
};

GvStationListIter *
gv_station_list_iter_new(GvStationList *self)
{
	GList *list = self->priv->stations;
	GvStationListIter *iter;

	iter = g_new0(GvStationListIter, 1);
	iter->head = g_list_copy_deep(list, copy_func_object_ref, NULL);
	iter->item = iter->head;

	return iter;
}

void
gv_station_list_iter_free(GvStationListIter *iter)
{
	g_return_if_fail(iter != NULL);

	g_list_free_full(iter->head, g_object_unref);
	g_free(iter);
}

gboolean
gv_station_list_iter_loop(GvStationListIter *iter, GvStation **station)
{
	g_return_val_if_fail(iter != NULL, FALSE);
	g_return_val_if_fail(station != NULL, FALSE);

	*station = NULL;

	if (iter->item == NULL)
		return FALSE;

	*station = iter->item->data;
	iter->item = iter->item->next;

	return TRUE;
}

/*
 * GList additions
 */

static gint
glist_sortfunc_random(gconstpointer a G_GNUC_UNUSED, gconstpointer b G_GNUC_UNUSED)
{
	return g_random_boolean() ? 1 : -1;
}

static GList *
g_list_shuffle(GList *list)
{
	/* Shuffle twice. Credits goes to:
	 * http://www.linuxforums.org/forum/programming-scripting/
	 * 202125-fastest-way-shuffle-linked-list.html
	 */
	list = g_list_sort(list, glist_sortfunc_random);
	list = g_list_sort(list, glist_sortfunc_random);
	return list;
}

static GList *
g_list_copy_deep_shuffle(GList *list, GCopyFunc func, gpointer user_data)
{
	list = g_list_copy_deep(list, func, user_data);
	list = g_list_shuffle(list);
	return list;
}

/*
 * Helpers
 */

static gint
are_stations_similar(GvStation *s1, GvStation *s2)
{
	if (s1 == s2) {
		WARNING("Stations %p and %p are the same", s1, s2);
		return 0;
	}

	if (s1 == NULL || s2 == NULL)
		return -1;

	/* Compare uids */
	const gchar *s1_uid, *s2_uid;
	s1_uid = gv_station_get_uid(s1);
	s2_uid = gv_station_get_uid(s2);

	if (!g_strcmp0(s1_uid, s2_uid)) {
		WARNING("Stations %p and %p have the same uid '%s'", s1, s2, s1_uid);
		return 0;
	}

	/* Compare names.
	 * Two stations who don't have name are different.
	 */
	const gchar *s1_name, *s2_name;
	s1_name = gv_station_get_name(s1);
	s2_name = gv_station_get_name(s2);

	if (s1_name == NULL && s2_name == NULL)
		return -1;

	if (!g_strcmp0(s1_name, s2_name)) {
		DEBUG("Stations %p and %p have the same name '%s'", s1, s2, s1_name);
		return 0;
	}

	/* Compare uris */
	const gchar *s1_uri, *s2_uri;
	s1_uri = gv_station_get_uri(s1);
	s2_uri = gv_station_get_uri(s2);

	if (!g_strcmp0(s1_uri, s2_uri)) {
		DEBUG("Stations %p and %p have the same uri '%s'", s1, s2, s1_uri);
		return 0;
	}

	return -1;
}

/*
 * Signal handlers
 */

static gboolean
when_timeout_save_station_list(gpointer data)
{
	GvStationList *self = GV_STATION_LIST(data);
	GvStationListPrivate *priv = self->priv;

	gv_station_list_save(self);

	priv->save_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_station_list_save_delayed(GvStationList *self)
{
	GvStationListPrivate *priv = self->priv;

	if (priv->save_source_id > 0)
		g_source_remove(priv->save_source_id);

	priv->save_source_id =
	        g_timeout_add_seconds(SAVE_DELAY, when_timeout_save_station_list, self);
}

static void
on_station_notify(GvStation     *station,
                  GParamSpec     *pspec,
                  GvStationList *self)
{
	const gchar *property_name = g_param_spec_get_name(pspec);

	TRACE("%s, %s, %p", gv_station_get_uid(station), property_name, self);

	/* We might want to save changes */
	if (!g_strcmp0(property_name, "uri") ||
	    !g_strcmp0(property_name, "name")) {
		gv_station_list_save_delayed(self);
	}

	/* Emit signal */
	g_signal_emit(self, signals[SIGNAL_STATION_MODIFIED], 0, station);
}

/*
 * Property accessors
 */

const gchar *
gv_station_list_get_load_path(GvStationList *self)
{
	return self->priv->load_path;
}

static void
gv_station_list_set_load_path(GvStationList *self, const gchar *path)
{
	GvStationListPrivate *priv = self->priv;

        if (!g_strcmp0(priv->load_path, path))
                return;

        g_free(priv->load_path);
        priv->load_path = g_strdup(path);
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_LOAD_PATH]);
}

const gchar *
gv_station_list_get_save_path(GvStationList *self)
{
	return self->priv->save_path;
}

static void
gv_station_list_set_save_path(GvStationList *self, const gchar *path)
{
	GvStationListPrivate *priv = self->priv;

        if (!g_strcmp0(priv->save_path, path))
                return;

        g_free(priv->save_path);
        priv->save_path = g_strdup(path);
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SAVE_PATH]);
}

static void
gv_station_list_get_property(GObject    *object,
                             guint       property_id,
                             GValue     *value G_GNUC_UNUSED,
                             GParamSpec *pspec)
{
	GvStationList *self = GV_STATION_LIST(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_LOAD_PATH:
		g_value_set_string(value, gv_station_list_get_load_path(self));
		break;
	case PROP_SAVE_PATH:
		g_value_set_string(value, gv_station_list_get_save_path(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_station_list_set_property(GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	GvStationList *self = GV_STATION_LIST(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_LOAD_PATH:
		gv_station_list_set_load_path(self, g_value_get_string(value));
		break;
	case PROP_SAVE_PATH:
		gv_station_list_set_save_path(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public functions
 */

void
gv_station_list_remove(GvStationList *self, GvStation *station)
{
	GvStationListPrivate *priv = self->priv;
	GList *item;

	/* Ensure a valid station was given */
	if (station == NULL) {
		WARNING("Attempting to remove NULL station");
		return;
	}

	/* Give info */
	INFO("Removing station '%s'", gv_station_get_name_or_uri(station));

	/* Check that we own this station at first. If we don't find it
	 * in our internal list, it's probably a programming error.
	 */
	item = g_list_find(priv->stations, station);
	if (item == NULL) {
		WARNING("GvStation %p (%s) not found in list",
		        station, gv_station_get_uid(station));
		return;
	}

	/* Disconnect signal handlers */
	g_signal_handlers_disconnect_by_data(station, self);

	/* Remove from list */
	priv->stations = g_list_remove_link(priv->stations, item);
	g_list_free(item);

	/* Unown the station */
	g_object_unref(station);

	/* Rebuild the shuffled station list */
	if (priv->shuffled) {
		g_list_free_full(priv->shuffled, g_object_unref);
		priv->shuffled = g_list_copy_deep_shuffle(priv->stations,
		                 copy_func_object_ref, NULL);
	}

	/* Emit a signal */
	g_signal_emit(self, signals[SIGNAL_STATION_REMOVED], 0, station);

	/* Save */
	gv_station_list_save_delayed(self);
}

void
gv_station_list_insert(GvStationList *self, GvStation *station, gint pos)
{
	GvStationListPrivate *priv = self->priv;
	GList *similar_item;

	/* Ensure a valid station was given */
	if (station == NULL) {
		WARNING("Attempting to insert NULL station");
		return;
	}

	/* Give info */
	INFO("Inserting station '%s'", gv_station_get_name_or_uri(station));

	/* Check that the station is not already part of the list.
	 * Duplicates are a programming error, we must warn about that.
	 * Identical fields are an user error.
	 * Warnings and such are encapsulated in the GCompareFunc used
	 * here, this is messy but temporary (hopefully).
	 */
	similar_item = g_list_find_custom(priv->stations, station,
	                                  (GCompareFunc) are_stations_similar);
	if (similar_item)
		return;

	/* Take ownership of the station */
	g_object_ref_sink(station);

	/* Add to the list at the right position */
	priv->stations = g_list_insert(priv->stations, station, pos);

	/* Connect to notify signal */
	g_signal_connect_object(station, "notify", G_CALLBACK(on_station_notify), self, 0);

	/* Rebuild the shuffled station list */
	if (priv->shuffled) {
		g_list_free_full(priv->shuffled, g_object_unref);
		priv->shuffled = g_list_copy_deep_shuffle(priv->stations,
		                 copy_func_object_ref, NULL);
	}

	/* Emit a signal */
	g_signal_emit(self, signals[SIGNAL_STATION_ADDED], 0, station);

	/* Save */
	gv_station_list_save_delayed(self);
}

/* Insert a station before another.
 * If 'before' is NULL or is not found, the station is appended at the end of the list.
 */
void
gv_station_list_insert_before(GvStationList *self, GvStation *station, GvStation *before)
{
	GvStationListPrivate *priv = self->priv;
	gint pos = -1;

	pos = g_list_index(priv->stations, before);
	gv_station_list_insert(self, station, pos);
}

/* Insert a station after another.
 * If 'after' is NULL or not found, the station is appended at the beginning of the list.
 */
void
gv_station_list_insert_after(GvStationList *self, GvStation *station, GvStation *after)
{
	GvStationListPrivate *priv = self->priv;
	gint pos = 0;

	pos = g_list_index(priv->stations, after);
	pos += 1; /* tricky but does what we want even for pos == -1 */
	gv_station_list_insert(self, station, pos);
}

void
gv_station_list_prepend(GvStationList *self, GvStation *station)
{
	gv_station_list_insert_after(self, station, NULL);
}

void
gv_station_list_append(GvStationList *self, GvStation *station)
{
	gv_station_list_insert_before(self, station, NULL);
}

void
gv_station_list_move(GvStationList *self, GvStation *station, gint pos)
{
	GvStationListPrivate *priv = self->priv;
	GList *item;

	/* Find the station */
	item = g_list_find(priv->stations, station);
	if (item == NULL) {
		WARNING("GvStation %p (%s) not found in list",
		        station, gv_station_get_uid(station));
		return;
	}

	/* Move it */
	priv->stations = g_list_remove_link(priv->stations, item);
	g_list_free(item);
	priv->stations = g_list_insert(priv->stations, station, pos);

	/* Emit a signal */
	g_signal_emit(self, signals[SIGNAL_STATION_MOVED], 0, station);

	/* Save */
	gv_station_list_save_delayed(self);
}

/* Move a station before another.
 * If 'before' is NULL or not found, the station is inserted at the end of the list.
 */
void
gv_station_list_move_before(GvStationList *self, GvStation *station, GvStation *before)
{
	GvStationListPrivate *priv = self->priv;
	gint pos = -1;

	pos = g_list_index(priv->stations, before);
	gv_station_list_move(self, station, pos);
}

void
gv_station_list_move_after(GvStationList *self, GvStation *station, GvStation *after)
{
	GvStationListPrivate *priv = self->priv;
	gint pos = 0;

	pos = g_list_index(priv->stations, after);
	pos += 1; /* tricky but does what we want even for pos == -1 */
	gv_station_list_move(self, station, pos);
}

void
gv_station_list_move_first(GvStationList *self, GvStation *station)
{
	gv_station_list_move_after(self, station, NULL);
}

void
gv_station_list_move_last(GvStationList *self, GvStation *station)
{
	gv_station_list_move_before(self, station, NULL);
}

GvStation *
gv_station_list_prev(GvStationList *self, GvStation *station,
                     gboolean repeat, gboolean shuffle)
{
	GvStationListPrivate *priv = self->priv;
	GList *stations, *item;

	/* Pickup the right station list, create shuffle list if needed */
	if (shuffle) {
		if (priv->shuffled == NULL) {
			priv->shuffled = g_list_copy_deep_shuffle(priv->stations,
			                 copy_func_object_ref, NULL);
		}
		stations = priv->shuffled;
	} else {
		if (priv->shuffled) {
			g_list_free_full(priv->shuffled, g_object_unref);
			priv->shuffled = NULL;
		}
		stations = priv->stations;
	}

	/* If the station list is empty, bail out */
	if (stations == NULL)
		return NULL;

	/* Return last station for NULL argument */
	if (station == NULL)
		return g_list_last(stations)->data;

	/* Try to find station in station list */
	item = g_list_find(stations, station);
	if (item == NULL)
		return NULL;

	/* Return previous station if any */
	item = item->prev;
	if (item)
		return item->data;

	/* Without repeat, there's no more station */
	if (!repeat)
		return NULL;

	/* With repeat, we may re-shuffle, then return the last station */
	if (shuffle) {
		GList *last_item;

		stations = g_list_shuffle(priv->shuffled);

		/* In case the last station (that we're about to return) happens to be
		 * the same as the current station, we do a little a magic trick.
		 */
		last_item = g_list_last(stations);
		if (last_item->data == station) {
			stations = g_list_remove_link(stations, last_item);
			stations = g_list_prepend(stations, last_item->data);
			g_list_free(last_item);
		}

		priv->shuffled = stations;
	}

	return g_list_last(stations)->data;
}

GvStation *
gv_station_list_next(GvStationList *self, GvStation *station,
                     gboolean repeat, gboolean shuffle)
{
	GvStationListPrivate *priv = self->priv;
	GList *stations, *item;

	/* Pickup the right station list, create shuffle list if needed */
	if (shuffle) {
		if (priv->shuffled == NULL) {
			priv->shuffled = g_list_copy_deep_shuffle(priv->stations,
			                 copy_func_object_ref, NULL);
		}
		stations = priv->shuffled;
	} else {
		if (priv->shuffled) {
			g_list_free_full(priv->shuffled, g_object_unref);
			priv->shuffled = NULL;
		}
		stations = priv->stations;
	}

	/* If the station list is empty, bail out */
	if (stations == NULL)
		return NULL;

	/* Return first station for NULL argument */
	if (station == NULL)
		return stations->data;

	/* Try to find station in station list */
	item = g_list_find(stations, station);
	if (item == NULL)
		return NULL;

	/* Return next station if any */
	item = item->next;
	if (item)
		return item->data;

	/* Without repeat, there's no more station */
	if (!repeat)
		return NULL;

	/* With repeat, we may re-shuffle, then return the first station */
	if (shuffle) {
		GList *first_item;

		stations = g_list_shuffle(priv->shuffled);

		/* In case the first station (that we're about to return) happens to be
		 * the same as the current station, we do a little a magic trick.
		 */
		first_item = g_list_first(stations);
		if (first_item->data == station) {
			stations = g_list_remove_link(stations, first_item);
			stations = g_list_append(stations, first_item->data);
			g_list_free(first_item);
		}

		priv->shuffled = stations;
	}

	return stations->data;
}

GvStation *
gv_station_list_first(GvStationList *self)
{
	GList *stations = self->priv->stations;

	if (stations == NULL)
		return NULL;

	return g_list_first(stations)->data;
}

GvStation *
gv_station_list_last(GvStationList *self)
{
	GList *stations = self->priv->stations;

	if (stations == NULL)
		return NULL;

	return g_list_last(stations)->data;
}

GvStation *
gv_station_list_at(GvStationList *self, guint n)
{
	GList *stations = self->priv->stations;
	GList *item;

	if (stations == NULL)
		return NULL;

	item = g_list_nth(stations, n);
	if (item == NULL)
		return NULL;

	return item->data;
}

GvStation *
gv_station_list_find(GvStationList *self, GvStation *station)
{
	GList *stations = self->priv->stations;
	GList *item;

	item = g_list_find(stations, station);

	return item ? item->data : NULL;
}

GvStation *
gv_station_list_find_by_name(GvStationList *self, const gchar *name)
{
	GList *item;

	/* Ensure station name is valid */
	if (name == NULL) {
		WARNING("Attempting to find a station with NULL name");
		return NULL;
	}

	/* Forbid empty names */
	if (!g_strcmp0(name, ""))
		return NULL;

	/* Iterate on station list */
	for (item = self->priv->stations; item; item = item->next) {
		GvStation *station = item->data;
		if (!g_strcmp0(name, gv_station_get_name(station)))
			return station;
	}

	return NULL;
}

GvStation *
gv_station_list_find_by_uri(GvStationList *self, const gchar *uri)
{
	GList *item;

	/* Ensure station name is valid */
	if (uri == NULL) {
		WARNING("Attempting to find a station with NULL uri");
		return NULL;
	}

	/* Iterate on station list */
	for (item = self->priv->stations; item; item = item->next) {
		GvStation *station = item->data;
		if (!g_strcmp0(uri, gv_station_get_uri(station)))
			return station;
	}

	return NULL;
}

GvStation *
gv_station_list_find_by_uid(GvStationList *self, const gchar *uid)
{
	GList *item;

	/* Ensure station name is valid */
	if (uid == NULL) {
		WARNING("Attempting to find a station with NULL uid");
		return NULL;
	}

	/* Iterate on station list */
	for (item = self->priv->stations; item; item = item->next) {
		GvStation *station = item->data;
		if (!g_strcmp0(uid, gv_station_get_uid(station)))
			return station;
	}

	return NULL;
}

GvStation  *
gv_station_list_find_by_guessing(GvStationList *self, const gchar *string)
{
	if (is_uri_scheme_supported(string))
		return gv_station_list_find_by_uri(self, string);
	else
		/* Assume it's the station name */
		return gv_station_list_find_by_name(self, string);
}

void
gv_station_list_save(GvStationList *self)
{
	GvStationListPrivate *priv = self->priv;
	const gchar *path = priv->save_path;
	GError *err = NULL;
	gboolean ret;

	/* Save the station list */
	ret = save_station_list_to_file(priv->stations, path, &err);
	if (ret == TRUE) {
		INFO("Station list saved to '%s'", path);
	} else {
		WARNING("Failed to save station list: %s", err->message);
		gv_errorable_emit_error(GV_ERRORABLE(self), _("%s: %s"),
		                        _("Failed to save station list"), err->message);

		g_error_free(err);
	}
}

void
gv_station_list_load(GvStationList *self)
{
	GvStationListPrivate *priv = self->priv;
	GList *item;

	TRACE("%p", self);

	/* This should be called only once at startup */
	g_assert_null(priv->stations);

	/* Try to load the station list, either from a user-provided path
	 * (ie. during tests), either from the list of default locations.
	 */
	if (priv->load_path) {
		GError *error = NULL;
		const gchar *path = priv->load_path;
		gboolean ret;

		ret = load_station_list_from_file(path, &priv->stations, &error);
		if (ret == FALSE) {
			ERROR("Failed to load station list from '%s': %s",
			       path, error->message);
			/* Program execution stops here */
		}

	} else if (priv->default_load_paths) {
		gchar **paths = priv->default_load_paths;
		guint n_paths = g_strv_length(paths);
		guint i;

		for (i = 0; i < n_paths; i++) {
			const gchar *path = paths[i];
			GError *err = NULL;
			gboolean ret;

			ret = load_station_list_from_file(path, &priv->stations, &err);
			if (ret == FALSE) {
				if (err->code != G_FILE_ERROR_NOENT)
					WARNING("Failed to load station list from '%s': %s",
						path, err->message);
				g_clear_error(&err);
				continue;
			}

			gv_station_list_set_load_path(self, path);
			break;
		}
	}

	/* Check if we got something */
	if (priv->load_path) {
		INFO("Station list loaded from file '%s'", priv->load_path);
	} else {
		gboolean ret;

		INFO("No valid station list file found, using hard-coded default");
		ret = load_station_list_from_string(DEFAULT_STATION_LIST, &priv->stations, NULL);
		if (ret == FALSE) {
			ERROR("Failed to load station list from hard-coded default");
			/* Program execution stops here */
		}
	}

	/* Dump the number of stations */
	DEBUG("Station list has %u stations", gv_station_list_length(self));

	/* Register a notify handler for each station */
	for (item = priv->stations; item; item = item->next) {
		GvStation *station = item->data;

		g_signal_connect_object(station, "notify", G_CALLBACK(on_station_notify), self, 0);
	}

	/* Emit a signal to indicate that the list has been loaded */
	g_signal_emit(self, signals[SIGNAL_LOADED], 0);
}

guint
gv_station_list_length(GvStationList *self)
{
	GvStationListPrivate *priv = self->priv;

	return g_list_length(priv->stations);
}

GvStationList *
gv_station_list_new(void)
{
	return g_object_new(GV_TYPE_STATION_LIST, NULL);
}

GvStationList *
gv_station_list_new_with_paths(const gchar *load_path, const gchar *save_path)
{
	return g_object_new(GV_TYPE_STATION_LIST,
			    "load-path", load_path,
			    "save-path", save_path,
			    NULL);
}

/*
 * GObject methods
 */

static gchar **
make_station_list_load_paths(void)
{
	const gchar *const *system_dirs;
	const gchar *user_dir;
	guint i, n_dirs;
	gchar **dirs;

	system_dirs = gv_get_app_system_data_dirs();
	user_dir = gv_get_app_user_data_dir();

	n_dirs = g_strv_length((gchar **) system_dirs) + 1;
	dirs = g_malloc0_n(n_dirs + 1, sizeof(gchar *));
	dirs[0] = g_build_filename(user_dir, STATION_LIST_FILE, NULL);
	for (i = 1; i < n_dirs; i++) {
		const gchar *dir;
		dir = system_dirs[i - 1];
		dirs[i] = g_build_filename(dir, STATION_LIST_FILE, NULL);
	}

	return dirs;
}

static gchar *
make_station_list_save_path(void)
{
	const gchar *user_dir;
	gchar *dir;

	user_dir = gv_get_app_user_data_dir();
	dir = g_build_filename(user_dir, STATION_LIST_FILE, NULL);

	return dir;
}

static void
gv_station_list_finalize(GObject *object)
{
	GvStationList *self = GV_STATION_LIST(object);
	GvStationListPrivate *priv = self->priv;
	GList *item;

	TRACE("%p", object);

	/* Run any pending save operation */
	if (priv->save_source_id > 0)
		when_timeout_save_station_list(self);

	/* Free shuffled station list */
	g_list_free_full(priv->shuffled, g_object_unref);

	/* Free station list and ensure no memory is leaked. This works only if the
	 * station list is the last object to hold references to stations. In other
	 * words, the station list must be the last object finalized.
	 */
	for (item = priv->stations; item; item = item->next) {
		g_object_add_weak_pointer(G_OBJECT(item->data), &(item->data));
		g_object_unref(item->data);
		if (item->data != NULL)
			WARNING("Station '%s' has not been finalized !",
			        gv_station_get_name_or_uri(GV_STATION(item->data)));
	}
	g_list_free(priv->stations);

	/* Free paths */
	g_free(priv->save_path);
	g_free(priv->load_path);
	g_free(priv->default_save_path);
	g_strfreev(priv->default_load_paths);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_station_list, object);
}

static void
gv_station_list_constructed(GObject *object)
{
	GvStationList *self = GV_STATION_LIST(object);
	GvStationListPrivate *priv = self->priv;

	/* Initialize default paths */
	priv->default_load_paths = make_station_list_load_paths();
	priv->default_save_path = make_station_list_save_path();

	/* Initialize save path if not set */
	if (priv->save_path == NULL)
		priv->save_path = g_strdup(priv->default_save_path);
	/* DON'T initialize load path, this will be done just in time */

	/* In version 4.1, the station file moved */
	relocate_station_list_file_for_4_1(priv->save_path);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_station_list, object);
}

static void
gv_station_list_init(GvStationList *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_station_list_get_instance_private(self);
}

static void
gv_station_list_class_init(GvStationListClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_station_list_finalize;
	object_class->constructed = gv_station_list_constructed;

	/* Properties */
	object_class->get_property = gv_station_list_get_property;
	object_class->set_property = gv_station_list_set_property;

	properties[PROP_LOAD_PATH] =
	        g_param_spec_string("load-path", "Load Path", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE |
				    G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_SAVE_PATH] =
	        g_param_spec_string("save-path", "Save Path", NULL, NULL,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE |
				    G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(object_class, PROP_N, properties);

	/* Signals */
	signals[SIGNAL_LOADED] =
	        g_signal_new("loaded", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 0);

	signals[SIGNAL_STATION_ADDED] =
	        g_signal_new("station-added", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_OBJECT);

	signals[SIGNAL_STATION_REMOVED] =
	        g_signal_new("station-removed", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_OBJECT);

	signals[SIGNAL_STATION_MODIFIED] =
	        g_signal_new("station-modified", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_OBJECT);

	signals[SIGNAL_STATION_MOVED] =
	        g_signal_new("station-moved", G_TYPE_FROM_CLASS(class),
	                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_OBJECT);
}
