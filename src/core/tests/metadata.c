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

#include <gst/gst.h>
#include <mutest.h>

#include "base/log.h"
#include "core/gv-metadata.h"

static void
metadata_empty(mutest_spec_t *spec G_GNUC_UNUSED)
{
	GvMetadata *m;
	GstTagList *l;
	gboolean changed;

	m = gv_metadata_new();
	mutest_expect("new() does not return null",
		      mutest_pointer(m),
		      mutest_not, mutest_to_be_null,
		      NULL);
	mutest_expect("new metadata is empty",
		      mutest_bool_value(gv_metadata_is_empty(m)),
		      mutest_to_be_true,
		      NULL);

	l = gst_tag_list_new_empty();

	changed = gv_metadata_update_from_gst_taglist(m, l);
	mutest_expect("update from empty gst taglist returns false",
		      mutest_bool_value(changed),
		      mutest_to_be_false,
		      NULL);
	mutest_expect("metadata is still empty",
		      mutest_bool_value(gv_metadata_is_empty(m)),
		      mutest_to_be_true,
		      NULL);

	gst_tag_list_unref(l);
	gv_metadata_unref(m);
}

static void
metadata_suite(mutest_suite_t *suite G_GNUC_UNUSED)
{
	mutest_it("update from empty gst taglist", metadata_empty);
}

MUTEST_MAIN(
	gst_init(NULL, NULL);
	log_init(NULL, TRUE, NULL);
	g_setenv("GOODVIBES_IN_TEST_SUITE", "1", TRUE);
	mutest_describe("gv-metadata", metadata_suite);
)
