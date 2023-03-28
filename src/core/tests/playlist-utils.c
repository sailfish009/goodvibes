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

#include <glib.h>
#include <mutest.h>

#include "base/log.h"
#include "core/playlist-utils.h"

static GSList*
parse_playlist(const gchar *filename, GvPlaylistFormat fmt)
{
	GSList *streams;
	GError *err = NULL;
	gchar *contents = NULL;
	gsize length = 0;
	gchar *fn;

	fn = g_build_filename("playlists", filename, NULL);
	g_file_get_contents(fn, &contents, &length, &err);
	g_assert_no_error(err);
	g_free(fn);

	streams = gv_playlist_parse(fmt, contents, length);
	g_free(contents);

	return streams;
}

static bool
match_streams(mutest_expect_t *e, mutest_expect_res_t *check)
{
	mutest_expect_res_t *value = mutest_expect_value(e);
	GSList *uris = (GSList *) mutest_get_pointer(value);
	gchar **expected_uris = (gchar **) mutest_get_pointer(check);
	guint expected_uris_length = g_strv_length(expected_uris);
	gboolean ret = TRUE;
	GSList *item;
	guint i;

	item = uris;
	for (i = 0; i < expected_uris_length; i++) {
		const gchar *exp_uri = expected_uris[i];
		const gchar *uri;

		if (item == NULL) {
			ret = FALSE;
			break;
		}

		uri = (const gchar *) item->data;
		if (g_strcmp0(uri, exp_uri) != 0) {
			ret = FALSE;
			break;
		}

		item = g_slist_next(item);
	}

	if (item != NULL)
		ret = FALSE;

	return ret;
}

static void
run_pls_test(const gchar *test_filename, const gchar *expected_uris[])
{
	GSList *streams;

	streams = parse_playlist(test_filename, GV_PLAYLIST_FORMAT_PLS);
	mutest_expect(test_filename, mutest_pointer(streams), match_streams,
		      mutest_pointer(expected_uris), NULL);
	g_slist_free_full(streams, g_free);
}

static void
playlist_utils_parse_pls(mutest_spec_t *spec G_GNUC_UNUSED)
{
	/* http://gyusyabu.ddo.jp:8000/listen.pls
	 * Correct case, correct fields, all good
	 */
	const gchar *gyusyabu[] = {
		"http://gyusyabu.ddo.jp:8000/",
		NULL,
	};
	run_pls_test("gyusyabu.pls", gyusyabu);

	/* http://www.abc.net.au/res/streaming/audio/mp3/local_adelaide.pls
	 * Wrong field: NumberOfEvents instead of NumberOfEntries, cf. #67
	 */
	const gchar *abc_adelaide[] = {
		"http://live-radio01.mediahubaustralia.com/5LRW/mp3/",
		"http://live-radio02.mediahubaustralia.com/5LRW/mp3/",
		NULL,
	};
	run_pls_test("abc-adelaide.pls", abc_adelaide);

	/* https://somafm.com/metal130.pls
	 * Wrong case: numberofentries instead of NumberOfEntries, cf. #88
	 */
	const gchar *somafm_metal130[] = {
		"https://ice2.somafm.com/metal-128-aac",
		"https://ice5.somafm.com/metal-128-aac",
		"https://ice4.somafm.com/metal-128-aac",
		"https://ice6.somafm.com/metal-128-aac",
		"https://ice1.somafm.com/metal-128-aac",
		NULL,
	};
	run_pls_test("somafm-metal130.pls", somafm_metal130);

	/* http://www.wnyc.org/stream/wnyc-fm939/mp3.pls
	 * Wrong case: [Playlist] instead of [playlist]
	 */
	const gchar *wnyc_fm[] = {
		"https://fm939.wnyc.org/wnycfm",
		NULL,
	};
	run_pls_test("wnyc-fm.pls", wnyc_fm);
}

static void
playlist_utils_suite(mutest_suite_t *suite G_GNUC_UNUSED)
{
	mutest_it("parse PLS playlists", playlist_utils_parse_pls);
}

MUTEST_MAIN(
	log_init(NULL, TRUE, NULL);
	mutest_describe("playlist-utils", playlist_utils_suite);
)
