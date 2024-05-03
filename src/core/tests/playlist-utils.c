/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2023-2024 Arnaud Rebillout
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
parse_playlist(GvPlaylistParser parser, const gchar *filename)
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

	streams = parser(contents, length);
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
run_test(GvPlaylistParser parser, const gchar *test_filename, const gchar *expected_uris[])
{
	GSList *streams;

	streams = parse_playlist(parser, test_filename);
	mutest_expect(test_filename, mutest_pointer(streams), match_streams,
		      mutest_pointer(expected_uris), NULL);
	g_slist_free_full(streams, g_free);
}

#define run_pls_test(fn, uris) run_test(gv_parse_pls_playlist, fn, uris)
#define run_m3u_test(fn, uris) run_test(gv_parse_m3u_playlist, fn, uris)
#define run_asx_test(fn, uris) run_test(gv_parse_asx_playlist, fn, uris)
#define run_xspf_test(fn, uris) run_test(gv_parse_xspf_playlist, fn, uris)

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
playlist_utils_parse_m3u(mutest_spec_t *spec G_GNUC_UNUSED)
{
	/* http://stream.levillage.org/canalb.m3u
	 * One line, one stream, nothing else. EOL: '\r\n'.
	 */
	const gchar *canalb[] = {
		"http://stream.levillage.org:80/canalb",
		NULL,
	};
	run_m3u_test("levillage-canalb.m3u", canalb);

	/* https://outpostradio.com/acousticoutpost/opost.m3u
	 * 4 lines, the same stream 4 times. EOL: '\n'.
	 */
	const gchar *outpost_acoustic[] = {
		"http://outpostradio.com/listen/acop",
		NULL,
	};
	run_m3u_test("outpost-acoustic.m3u", outpost_acoustic);

	/* https://somafm.com/m3u/reggae256.m3u
	 * So-called "extended m3u" (ie. it has comments with directives),
	 * with a bunch of streams, and trailing empty lines. EOL: '\n'.
	 */
	const gchar *somafm_reggae256[] = {
		"http://ice5.somafm.com/reggae-256-mp3",
		"http://ice6.somafm.com/reggae-256-mp3",
		"http://ice2.somafm.com/reggae-256-mp3",
		"http://ice4.somafm.com/reggae-256-mp3",
		"http://ice1.somafm.com/reggae-256-mp3",
		NULL,
	};
	run_m3u_test("somafm-reggae256.m3u", somafm_reggae256);

	/* https://radiofabrik.at/rafab_stream_low.m3u
	 * Extended m3u, one stream. EOL: none.
	 */
	const gchar *radiofabrik[] = {
		"http://stream.radiofabrik.at/rf_low.mp3",
		NULL,
	};
	run_m3u_test("radiofabrik.m3u", radiofabrik);

	/* https://www.nonstopplay.com/site/media/ram/midband.ram
	 * Two lines, two streams. EOL: '\r\n'.
	 */
	const gchar *nonstopplay_midband[] = {
		"http://stream.nonstopplay.co.uk/nsp-64k-mp3",
		"http://stream.nonstopplay.co.uk/nsp-32k-mp3",
		NULL,
	};
	run_m3u_test("nonstopplay-midband.ram", nonstopplay_midband);
}

static void
playlist_utils_parse_asx(mutest_spec_t *spec G_GNUC_UNUSED)
{
	/* https://listen.trancebase.fm/dsl.asx */
	const gchar *trancebase[] = {
		"https://listen.trancebase.fm/tunein-mp3-asx",
		"http://listen.trancebase.fm/tunein-mp3-asx",
		NULL,
	};
	run_asx_test("trancebase.asx", trancebase);

	/* https://radio-nostalgia.nl/nostalgia.asx
	 * This one's all uppercase.
	 */
	const gchar *nostalgia[] = {
		"https://kathy.torontocast.com:2250/stream",
		NULL,
	};
	run_asx_test("nostalgia.asx", nostalgia);
}

static void
playlist_utils_parse_xspf(mutest_spec_t *spec G_GNUC_UNUSED)
{
	/* https://radiometalon.com/radio/8020/radio.mp3.xspf */
	const gchar *metalon[] = {
		"http://radiometalon.com:8020/radio.mp3",
		NULL,
	};
	run_xspf_test("metalon.xspf", metalon);
}

static void
playlist_utils_suite(mutest_suite_t *suite G_GNUC_UNUSED)
{
	mutest_it("parse M3U playlists", playlist_utils_parse_m3u);
	mutest_it("parse PLS playlists", playlist_utils_parse_pls);
	mutest_it("parse ASX playlists", playlist_utils_parse_asx);
	mutest_it("parse XSPF playlists", playlist_utils_parse_xspf);
}

MUTEST_MAIN(
	log_init(NULL, TRUE, NULL);
	mutest_describe("playlist-utils", playlist_utils_suite);
)
