/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023 Arnaud Rebillout
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

/*
 * This code was initially inspired by other media players such as Parole or
 * Totem (Playlist Parser). Big thanks to them.
 *
 * A definitive guide to playlist formats (a must-read, though I didn't):
 * http://gonze.com/playlists/playlist-format-survey.html
 *
 * WISHED Test with a lot, really a lot of different stations.  I'm pretty sure
 * this playlist implementation is not robust enough.  TODO   Validate URI,
 * send an error message if it's invalid?  But then, shouldn't that be done in
 * GvStation instead? Or not?
 */

#include <string.h>
#include <glib.h>

#include "base/gv-base.h"

#include "core/playlist-utils.h"


/* Parse a M3U playlist, which is a simple text file, each line being a URI.
 *
 * Ref: https://en.wikipedia.org/wiki/M3U
 * Content-Types: audio/x-mpegurl
 * Extra-Content-Types (seen in the wild): application/x-mpegurl, audio/mpegurl
 *
 * Also works for real-audio playlists.
 *
 * Ref: https://en.wikipedia.org/wiki/RealAudio
 * Content-Types: audio/x-pn-realaudio
 */

GSList *
gv_parse_m3u_playlist(const gchar *text, gsize text_size G_GNUC_UNUSED)
{
	GSList *list = NULL;
	const gchar *eol;
	gchar **lines;
	guint i;

	/* Guess which delimiter is used to separate each lines.
	 * Traditionnaly, UNIX would only use `\n`, Windows would use `\r\n`.
	 */
	if (strstr(text, "\r\n"))
		eol = "\r\n";
	else
		eol = "\n";

	/* Split into different lines */
	lines = g_strsplit(text, eol, -1);
	if (lines == NULL) {
		WARNING("Empty m3u playlist");
		return NULL;
	}

	/* Iterate on lines */
	for (i = 0; lines[i] != NULL; i++) {
		gchar *line = lines[i];

		/* Remove leading & trailing whitespaces */
		line = g_strstrip(line);

		/* Ignore emtpy lines and comments */
		if (line[0] == '\0' || line[0] == '#')
			continue;

		/* If it's not an URI, we discard it */
		if (!strstr(line, "://"))
			continue;

		/* Add to stream list */
		list = g_slist_append(list, g_strdup(line));
	}

	g_strfreev(lines);
	return list;
}

/* Parse a PLS playlist, which is a "Desktop Entry File" in the Unix world,
 * or an "INI File" in the Windows realm.
 *
 * Examples in the wild showed that it must be treated as case-insensitive,
 * even though the format is supposed to be case-sensitive.
 *
 * Ref: https://en.wikipedia.org/wiki/PLS_(file_format)
 * Content-Types: audio/x-scpls
 * Extra-Content-Types (seen in the wild): application/pls+xml, audio/scpls
 */

static gchar *
pls_get_playlist_group_name(GKeyFile *key_file)
{
	gchar **groups;
	gchar **ptr;
	gchar *ret;

	if (g_key_file_has_group(key_file, "playlist"))
		return g_strdup("playlist");

	groups = g_key_file_get_groups(key_file, NULL);

	ret = NULL;
	for (ptr = groups; *ptr != NULL; ptr++) {
		if (g_ascii_strcasecmp(*ptr, "playlist") == 0) {
			ret = g_strdup(*ptr);
			break;
		}
	}

	g_strfreev(groups);

	return ret;
}

static gchar *
pls_get_number_of_entries_key_name(GKeyFile *key_file, const gchar *group_name)
{
	GError *err = NULL;
	gchar **keys;
	gchar **ptr;
	gchar *ret;

	if (g_key_file_has_key(key_file, group_name, "NumberOfEntries", NULL))
		return g_strdup("NumberOfEntries");

	keys = g_key_file_get_keys(key_file, group_name, NULL, &err);
	if (err != NULL) {
		WARNING("Failed to get keys for group %s: %s", group_name, err->message);
		g_clear_error(&err);
		return NULL;
	}

	ret = NULL;
	for (ptr = keys; *ptr != NULL; ptr ++) {
		if (g_ascii_strcasecmp(*ptr, "numberofentries") == 0) {
			ret = g_strdup(*ptr);
			break;
		} else if (g_ascii_strcasecmp(*ptr, "numberofevents") == 0) {
			ret = g_strdup(*ptr);
			break;
		}
	}

	g_strfreev(keys);

	return ret;
}

static GSList *
pls_get_streams(GKeyFile *keyfile, const gchar *playlist, guint n_entries)
{
	GSList *list = NULL;
	guint i;

	for (i = 0; i < n_entries; i++) {
		GError *err = NULL;
		gchar key[8];
		gchar *str;

		g_snprintf(key, sizeof key, "File%u", i + 1);

		if (!g_key_file_has_key(keyfile, playlist, key, NULL))
			g_snprintf(key, sizeof key, "file%u", i + 1);

		if (!g_key_file_has_key(keyfile, playlist, key, NULL)) {
			WARNING("Failed to get key %s: %s", key);
			continue;
		}

		str = g_key_file_get_string(keyfile, playlist, key, &err);
		if (err != NULL) {
			WARNING("Failed to get string for key %s: %s", key, err->message);
			g_clear_error(&err);
			continue;
		}

		 /* No need for strdup, it's already an allocated string */
		list = g_slist_append(list, str);
	}

	return list;
}

GSList *
gv_parse_pls_playlist(const gchar *text, gsize text_size)
{
	GKeyFile *keyfile;
	GError *err = NULL;
	GSList *list = NULL;
	gchar *playlist = NULL;
	gchar *number_of_entries = NULL;
	gint n_entries;

	keyfile = g_key_file_new();

	g_key_file_load_from_data(keyfile, text, text_size, 0, &err);
	if (err != NULL) {
		WARNING("Failed to parse pls playlist: %s", err->message);
		g_clear_error(&err);
		goto fail;
	}

	playlist = pls_get_playlist_group_name(keyfile);
	if (playlist == NULL) {
		WARNING("Failed to get the playlist group name");
		goto fail;
	}

	number_of_entries = pls_get_number_of_entries_key_name(keyfile, playlist);
	if (number_of_entries == NULL) {
		WARNING("Failed to get the number of entries key name");
		goto fail;
	}

	n_entries = g_key_file_get_integer(keyfile, playlist, number_of_entries, &err);
	if (err != NULL) {
		WARNING("Failed to get integer for key %s: %s", number_of_entries, err->message);
		g_clear_error(&err);
		goto fail;
	}
	if (n_entries < 0) {
		WARNING("Number of entries is negative: %d", n_entries);
		goto fail;
	}

	list = pls_get_streams(keyfile, playlist, (guint) n_entries);

fail:
	g_free(number_of_entries);
	g_free(playlist);
	g_key_file_free(keyfile);

	return list;
}

/* Parse an ASX (Advanced Stream Redirector) playlist.
 * Ref: https://en.wikipedia.org/wiki/Advanced_Stream_Redirector
 * Content-Types: video/x-ms-asf
 * Extra-Content-Types (seen in the wild): audio/x-ms-wax, video/x-ms-asx
 */

static void
asx_parse_element_cb(GMarkupParseContext *context G_GNUC_UNUSED,
		     const gchar *element_name,
		     const gchar **attribute_names,
		     const gchar **attribute_values,
		     gpointer user_data,
		     GError **err G_GNUC_UNUSED)
{
	GSList **llink = (GSList **) user_data;
	const gchar *href;
	guint i;

	/* We're only interested in the 'ref' element */
	if (g_ascii_strcasecmp(element_name, "ref"))
		return;

	/* Get 'href' attribute */
	href = NULL;
	for (i = 0; attribute_names[i]; i++) {
		if (!g_ascii_strcasecmp(attribute_names[i], "href")) {
			href = attribute_values[i];
			break;
		}
	}

	/* Add to stream list */
	if (href)
		*llink = g_slist_append(*llink, g_strdup(href));
}

static void
asx_error_cb(GMarkupParseContext *context G_GNUC_UNUSED,
	     GError *err G_GNUC_UNUSED,
	     gpointer user_data)
{
	GSList **llink = (GSList **) user_data;

	g_slist_free_full(*llink, g_free);
	*llink = NULL;
}

GSList *
gv_parse_asx_playlist(const gchar *text, gsize text_size)
{
	GMarkupParseContext *context;
	GMarkupParser parser = {
		asx_parse_element_cb,
		NULL,
		NULL,
		NULL,
		asx_error_cb,
	};
	GError *err = NULL;
	GSList *list = NULL;

	context = g_markup_parse_context_new(&parser, 0, &list, NULL);

	if (!g_markup_parse_context_parse(context, text, text_size, &err)) {
		WARNING("Failed to parse context: %s", err->message);
		g_error_free(err);
	}

	g_markup_parse_context_free(context);

	return list;
}

/* Parse an XSPF (XML Shareable Playlist Format) playlist.
 * Ref: https://en.wikipedia.org/wiki/XML_Shareable_Playlist_Format
 * Content-Types: application/xspf+xml
 */

static void
xspf_text_cb(GMarkupParseContext *context,
	     const gchar *text,
	     gsize text_len G_GNUC_UNUSED,
	     gpointer user_data,
	     GError **err G_GNUC_UNUSED)
{
	GSList **llink = (GSList **) user_data;
	const gchar *element_name;

	element_name = g_markup_parse_context_get_element(context);

	/* We're only interested in the 'location' element */
	if (g_ascii_strcasecmp(element_name, "location"))
		return;

	/* Add to stream list */
	*llink = g_slist_append(*llink, g_strdup(text));
}

static void
xspf_error_cb(GMarkupParseContext *context G_GNUC_UNUSED,
	      GError *err G_GNUC_UNUSED,
	      gpointer user_data)
{
	GSList **llink = (GSList **) user_data;

	g_slist_free_full(*llink, g_free);
	*llink = NULL;
}

GSList *
gv_parse_xspf_playlist(const gchar *text, gsize text_size)
{
	GMarkupParseContext *context;
	GMarkupParser parser = {
		NULL,
		NULL,
		xspf_text_cb,
		NULL,
		xspf_error_cb,
	};
	GError *err = NULL;
	GSList *list = NULL;

	context = g_markup_parse_context_new(&parser, 0, &list, NULL);

	if (!g_markup_parse_context_parse(context, text, text_size, &err)) {
		WARNING("Failed to parse context: %s", err->message);
		g_error_free(err);
	}

	g_markup_parse_context_free(context);

	return list;
}
