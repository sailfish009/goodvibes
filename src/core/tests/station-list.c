/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2019-2020 Arnaud Rebillout
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
#include <glib/gstdio.h>
#include <glib-object.h>
#include <mutest.h>

#include "default-stations.h"
#include "base/log.h"
#include "core/gv-station-list.h"

#define TOUCHTMP(tmpl) g_close(g_mkstemp(tmpl), NULL)

static void
station_list_load_default(mutest_spec_t *spec G_GNUC_UNUSED)
{
	GvStationList *s;

	s = gv_station_list_new(DEFAULT_STATIONS);
	g_object_add_weak_pointer(G_OBJECT(s), (gpointer *) &s);

	mutest_expect("new() does not return null",
			mutest_pointer(s),
			mutest_not, mutest_to_be_null,
			NULL);

	gv_station_list_load(s);

	mutest_expect("length() is not zero",
			mutest_int_value(gv_station_list_length(s)),
			mutest_not, mutest_to_be, 0,
			NULL);

	g_object_unref(s);

	mutest_expect("finalize() was called",
			mutest_pointer(s),
			mutest_to_be_null,
			NULL);
}

static gsize
get_file_length(const gchar *path)
{
	GError *err = NULL;
	gchar *contents = NULL;
	gsize length;

	g_file_get_contents(path, &contents, &length, &err);
	g_assert_no_error(err);
	g_free(contents);

	return length;
}

static void
station_list_load_save_empty(mutest_spec_t *spec G_GNUC_UNUSED)
{
	GvStationList *s;
	gchar *input, *output;
	gchar template[] = "/tmp/gv-station-list-XXXXXX.xml";

	TOUCHTMP(template);

	/* First, test with empty file */

	input = "/dev/null";
	output = template;

	mutest_expect("input file is empty",
			mutest_int_value(get_file_length(input)),
			mutest_to_be, 0,
			NULL);

	s = gv_station_list_new_with_paths(input, output);
	g_object_add_weak_pointer(G_OBJECT(s), (gpointer *) &s);

	mutest_expect("new_with_paths() does not return null",
			mutest_pointer(s),
			mutest_not, mutest_to_be_null,
			NULL);

	gv_station_list_load(s);

	mutest_expect("length() is zero",
			mutest_int_value(gv_station_list_length(s)),
			mutest_to_be, 0,
			NULL);
	mutest_expect("first() is null",
			mutest_pointer(gv_station_list_first(s)),
			mutest_to_be_null,
			NULL);
	mutest_expect("last() is null",
			mutest_pointer(gv_station_list_last(s)),
			mutest_to_be_null,
			NULL);
	mutest_expect("prev() is null",
			mutest_pointer(gv_station_list_prev(s, NULL, FALSE, FALSE)),
			mutest_to_be_null,
			NULL);
	mutest_expect("next() is null",
			mutest_pointer(gv_station_list_next(s, NULL, FALSE, FALSE)),
			mutest_to_be_null,
			NULL);

	gv_station_list_save(s);
	g_object_unref(s);

	mutest_expect("finalize() was called",
			mutest_pointer(s),
			mutest_to_be_null,
			NULL);
	mutest_expect("station list was saved to file",
			mutest_bool_value(g_file_test(output, G_FILE_TEST_EXISTS)),
			mutest_to_be_true,
			NULL);

	/* Second, test with file that contains an empty station list */

	input = output;

	mutest_expect("input file is NOT empty",
			mutest_int_value(get_file_length(input)),
			mutest_not, mutest_to_be, 0,
			NULL);

	s = gv_station_list_new_with_paths(input, output);
	g_object_add_weak_pointer(G_OBJECT(s), (gpointer *) &s);

	mutest_expect("new_with_paths() does not return null",
			mutest_pointer(s),
			mutest_not, mutest_to_be_null,
			NULL);

	gv_station_list_load(s);

	mutest_expect("length() is zero",
			mutest_int_value(gv_station_list_length(s)),
			mutest_to_be, 0,
			NULL);

	g_object_unref(s);

	mutest_expect("finalize() was called",
			mutest_pointer(s),
			mutest_to_be_null,
			NULL);

	g_unlink(output);
}

/* Match a GvStationList against an array. Consume the array */
static bool
match_station_list_against_array(mutest_expect_t *e,
	                         mutest_expect_res_t *check)
{
	mutest_expect_res_t *value = mutest_expect_value(e);
	GvStationList *s = (GvStationList *) mutest_get_pointer(value);
	GPtrArray *array = (GPtrArray *) mutest_get_pointer(check);
	gboolean ret = TRUE;
	guint i;

	for (i = 0; i < array->len; i++) {
		GvStation *a = (GvStation *) g_ptr_array_index(array, i);
		GvStation *b = gv_station_list_at(s, i);

		if (a != b) {
			ret = FALSE;
			break;
		}
	}

	if (i != gv_station_list_length(s))
		ret = FALSE;

	g_ptr_array_free(array, FALSE);

	return ret;
}

/* Create a GPtrArray of GvStations */
static GPtrArray *
make_station_array(GvStation *stations[], ...)
{
	GPtrArray *array;
	va_list args;
	gint idx;

	array = g_ptr_array_new();
	va_start(args, stations);
	while ((idx = va_arg(args, gint)) != -1) {
		g_ptr_array_add(array, stations[idx]);
	}
	va_end(args);

	return array;
}

static void
station_list_add_move_remove(mutest_spec_t *spec G_GNUC_UNUSED)
{
	GvStationList *s;
	GvStation *ss[7];
	gchar *input, *output;
	guint i;

	input = "/dev/null";
	output = "/dev/null";

	s = gv_station_list_new_with_paths(input, output);
	g_object_add_weak_pointer(G_OBJECT(s), (gpointer *) &s);

	/* Create stations. Due to floating reference, stations are finalized
	 * when removed from the station list. And due to the weak pointers that
	 * we add here, when it happens, the pointer value is set to NULL in the
	 * array, so that at the end of the test, we can check that the array
	 * is completely NULL.
	 */
	for (i = 0; i < 7; i++) {
		gchar *name = g_strdup_printf("s%u", i);
		gchar *url = g_strdup_printf("http://sta%u.com", i);
		ss[i] = gv_station_new(name, url);
		g_object_add_weak_pointer(G_OBJECT(ss[i]), (gpointer *) &ss[i]);
		g_free(name);
		g_free(url);
	}

	/* Station list is empty to start with */
	mutest_expect("list is []",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, -1)),
			NULL);

	/* Populate with 3 stations, using append() and prepend() */
	gv_station_list_append(s, ss[1]);
	mutest_expect("list is [1]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 1, -1)),
			NULL);
	gv_station_list_append(s, ss[2]);
	mutest_expect("list is [1, 2]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 1, 2, -1)),
			NULL);
	gv_station_list_prepend(s, ss[0]);
	mutest_expect("list is [0, 1, 2]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 0, 1, 2, -1)),
			NULL);

	/* Insert ss[3] after first, then remove first */
	gv_station_list_insert_after(s, ss[3], gv_station_list_first(s));
	mutest_expect("list is [0, 3, 1, 2]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 0, 3, 1, 2, -1)),
			NULL);
	gv_station_list_remove(s, gv_station_list_first(s));
	mutest_expect("list is [3, 1, 2]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 3, 1, 2, -1)),
			NULL);

	/* Insert ss[4] after last */
	gv_station_list_insert_after(s, ss[4], gv_station_list_last(s));
	mutest_expect("list is [3, 1, 2, 4]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 3, 1, 2, 4, -1)),
			NULL);

	/* Insert ss[5] before first */
	gv_station_list_insert_before(s, ss[5], gv_station_list_first(s));
	mutest_expect("list is [5, 3, 1, 2, 4]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 5, 3, 1, 2, 4, -1)),
			NULL);

	/* Insert ss[6] before last, then remove last */
	gv_station_list_insert_before(s, ss[6], gv_station_list_last(s));
	mutest_expect("list is [5, 3, 1, 2, 6, 4]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 5, 3, 1, 2, 6, 4, -1)),
			NULL);
	gv_station_list_remove(s, gv_station_list_last(s));
	mutest_expect("list is [5, 3, 1, 2, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 5, 3, 1, 2, 6, -1)),
			NULL);

	/* Move ss[5] before last */
	gv_station_list_move_before(s, ss[5], gv_station_list_last(s));
	mutest_expect("list is [3, 1, 2, 5, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 3, 1, 2, 5, 6, -1)),
			NULL);

	/* Move ss[3] (ie. first) before first */
	gv_station_list_move_after(s, ss[3], gv_station_list_first(s));
	mutest_expect("list is [3, 1, 2, 5, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 3, 1, 2, 5, 6, -1)),
			NULL);

	/* Move ss[3] (ie. first) after first */
	gv_station_list_move_before(s, ss[3], gv_station_list_first(s));
	mutest_expect("list is [3, 1, 2, 5, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 3, 1, 2, 5, 6, -1)),
			NULL);

	/* Move ss[2] at last */
	gv_station_list_move_last(s, ss[2]);
	mutest_expect("list is [3, 1, 5, 6, 2]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 3, 1, 5, 6, 2, -1)),
			NULL);

	/* Move ss[2] at first */
	gv_station_list_move_first(s, ss[2]);
	mutest_expect("list is [2, 3, 1, 5, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 2, 3, 1, 5, 6, -1)),
			NULL);

	/* Move ss[1] at position 0 */
	gv_station_list_move(s, ss[1], 0);
	mutest_expect("list is [1, 2, 3, 5, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 1, 2, 3, 5, 6, -1)),
			NULL);

	/* Time to remove stations one by one */
	gv_station_list_remove(s, ss[1]);
	mutest_expect("list is [2, 3, 5, 6]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 2, 3, 5, 6, -1)),
			NULL);
	gv_station_list_remove(s, ss[6]);
	mutest_expect("list is [2, 3, 5]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 2, 3, 5, -1)),
			NULL);
	gv_station_list_remove(s, ss[3]);
	mutest_expect("list is [2, 5]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 2, 5, -1)),
			NULL);
	gv_station_list_remove(s, ss[2]);
	mutest_expect("list is [5]",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, 5, -1)),
			NULL);
	gv_station_list_remove(s, ss[5]);
	mutest_expect("list is []",
			mutest_pointer(s),
			match_station_list_against_array,
			mutest_pointer(make_station_array(ss, -1)),
			NULL);

	for (i = 0; i < 7; i++)
		g_assert_null(ss[i]);

	g_object_unref(s);
	g_assert_null(s);
}

static void
station_list_suite(mutest_suite_t *suite G_GNUC_UNUSED)
{
	gchar *tmpdir;

	/* When a station list is created without giving it paths,
	 * then it relies on XDG directories to find an existing
	 * station list to load, and to save it in the user home
	 * directory. For unit tests, this is not nice, we want to
	 * be sure that we DON'T use anything that might already
	 * exist in the filesystem.
	 */

	tmpdir = g_dir_make_tmp("gv-station-list-XXXXXX", NULL);
	g_assert_nonnull(tmpdir);

	g_setenv("XDG_DATA_HOME", tmpdir, TRUE);
	g_setenv("XDG_DATA_DIRS", tmpdir, TRUE);
	g_setenv("XDG_CONFIG_HOME", tmpdir, TRUE);
	g_setenv("XDG_CONFIG_DIRS", tmpdir, TRUE);

	mutest_it("load the default station list", station_list_load_default);
	mutest_it("load and save an empty station list", station_list_load_save_empty);
	mutest_it("add, move and remove stations", station_list_add_move_remove);

	g_assert_true(g_rmdir(tmpdir) == 0);
	g_free(tmpdir);
}

MUTEST_MAIN(
	log_init(NULL, TRUE, NULL);
	g_setenv("GOODVIBES_IN_TEST_SUITE", "1", TRUE);
	mutest_describe("gv-station-list", station_list_suite);
)
