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
#include <libsoup/soup.h>

#include "base/gv-base.h"

#include "core/playlist-utils.h"

typedef GSList *(*PlaylistParser)(const gchar *, gsize);

/* Parse a M3U playlist, which is a simple text file, each line being a URI.
 * Ref: https://en.wikipedia.org/wiki/M3U
 */

static GSList *
parse_playlist_m3u(const gchar *text, gsize text_size G_GNUC_UNUSED)
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
 * Ref: https://en.wikipedia.org/wiki/PLS_(file_format)
 */

static guint
pls_get_n_items(GKeyFile *keyfile)
{
	const gchar *keys[] = { "NumberOfEntries", "numberofentries",
				"NumberOfEvents", "numberofevents",
				NULL };
	const gchar **ptr;
	const gchar *key = NULL;
	GError *err = NULL;
	gint n_items;

	for (ptr = keys; *ptr; ptr++) {
		if (g_key_file_has_key(keyfile, "playlist", *ptr, NULL)) {
			key = *ptr;
			break;
		}
	}

	if (key == NULL) {
		WARNING("Failed to get the number of items in pls playlist");
		return 0;
	}

	n_items = g_key_file_get_integer(keyfile, "playlist", key, &err);
	if (err) {
		WARNING("Failed to get key '%s': %s", key, err->message);
		g_error_free(err);
		return 0;
	}

	return n_items;
}

static GSList *
parse_playlist_pls(const gchar *text, gsize text_size)
{
	GSList *list = NULL;
	GKeyFile *keyfile;
	GError *err = NULL;
	guint i, n_items;

	keyfile = g_key_file_new();

	/* Parse the file */
	g_key_file_load_from_data(keyfile, text, text_size, 0, &err);
	if (err) {
		WARNING("Failed to parse pls playlist: %s", err->message);
		g_error_free(err);
		goto end;
	}

	/* Get the number of items */
	n_items = pls_get_n_items(keyfile);
	if (n_items == 0)
		goto end;

	/* Get all stream uris */
	for (i = 0; i < n_items; i++) {
		gchar key[8];
		gchar *str;

		g_snprintf(key, sizeof key, "File%u", i + 1);

		str = g_key_file_get_string(keyfile, "playlist", key, &err);
		if (err) {
			WARNING("Failed to get '%s': %s", key, err->message);
			g_error_free(err);
			err = NULL;
			continue;
		}

		/* Add to stream list.
		 * No need to duplicate str, it's already an allocated string.
		 */
		list = g_slist_append(list, str);
	}

end:
	g_key_file_free(keyfile);

	return list;
}

/* Parse an ASX (Advanced Stream Redirector) playlist.
 * Ref: https://en.wikipedia.org/wiki/Advanced_Stream_Redirector
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

static GSList *
parse_playlist_asx(const gchar *text, gsize text_size)
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

static GSList *
parse_playlist_xspf(const gchar *text, gsize text_size)
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

/*
 * Public functions
 */

GvPlaylistFormat
gv_playlist_get_format(const gchar *uri_string)
{
	GvPlaylistFormat fmt = GV_PLAYLIST_FORMAT_UNKNOWN;
	SoupURI *uri;
	const gchar *path;
	const gchar *ext;

	/* Parse the uri */
	uri = soup_uri_new(uri_string);
	if (uri == NULL) {
		INFO("Invalid uri '%s'", uri_string);
		return GV_PLAYLIST_FORMAT_UNKNOWN;
	}

	/* Get the path */
	path = soup_uri_get_path(uri);

	/* Get the extension of the path */
	ext = strrchr(path, '.');
	if (ext)
		ext++;
	else
		ext = "\0";

	/* Match with supported extensions */
	if (!g_ascii_strcasecmp(ext, "m3u"))
		fmt = GV_PLAYLIST_FORMAT_M3U;
	else if (!g_ascii_strcasecmp(ext, "ram"))
		fmt = GV_PLAYLIST_FORMAT_M3U;
	else if (!g_ascii_strcasecmp(ext, "pls"))
		fmt = GV_PLAYLIST_FORMAT_PLS;
	else if (!g_ascii_strcasecmp(ext, "asx"))
		fmt = GV_PLAYLIST_FORMAT_ASX;
	else if (!g_ascii_strcasecmp(ext, "xspf"))
		fmt = GV_PLAYLIST_FORMAT_XSPF;

	/* Cleanup */
	soup_uri_free(uri);

	return fmt;
}

GSList *
gv_playlist_parse(GvPlaylistFormat format, const gchar *content, gsize content_length)
{
	PlaylistParser parser;
	GSList *streams;
	GSList *item;

	switch (format) {
	case GV_PLAYLIST_FORMAT_M3U:
		parser = parse_playlist_m3u;
		break;
	case GV_PLAYLIST_FORMAT_PLS:
		parser = parse_playlist_pls;
		break;
	case GV_PLAYLIST_FORMAT_ASX:
		parser = parse_playlist_asx;
		break;
	case GV_PLAYLIST_FORMAT_XSPF:
		parser = parse_playlist_xspf;
		break;
	default:
		WARNING("No parser for playlist format: %d", format);
		return NULL;
	}

	streams = parser(content, content_length);
	if (streams == NULL) {
		WARNING("Failed to parse playlist, or was empty");
		return NULL;
	}

	DEBUG("%d streams found:", g_slist_length(streams));
	for (item = streams; item; item = item->next) {
		DEBUG(". %s", item->data);
	}

	return streams;
}
