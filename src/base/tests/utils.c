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
#include "base/utils.h"

static void
is_uri_scheme_supported(mutest_spec_t *spec G_GNUC_UNUSED)
{
	const gchar *uri;
	gboolean res;

	uri = "foobar://foo.bar";
	res = gv_is_uri_scheme_supported(uri);
	mutest_expect("foobar is not supported", mutest_bool_value(res),
			mutest_to_be, false, NULL);

	uri = "http://pbbradio.com:8443/pbb128";
	res = gv_is_uri_scheme_supported(uri);
	mutest_expect("http is supported", mutest_bool_value(res),
			mutest_to_be, true, NULL);

	uri = "https://pbbradio.com:8443/pbb128";
	res = gv_is_uri_scheme_supported(uri);
	mutest_expect("https is supported", mutest_bool_value(res),
			mutest_to_be, true, NULL);

	uri = "HTTP://pbbradio.com:8443/pbb128";
	res = gv_is_uri_scheme_supported(uri);
	mutest_expect("case doesn't matter", mutest_bool_value(res),
			mutest_to_be, true, NULL);
}

static void
test_get_uri_extension(const gchar *description_prefix, const gchar *uri,
		gboolean expected_result, const gchar *expected_extension)
{
	gchar *description;
	gboolean res;
	gchar *ext;

	res = gv_get_uri_extension_lowercase(uri, &ext, NULL);

	description = g_strdup_printf("%s: parsing uri %s", description_prefix,
			expected_result == TRUE ? "succeeds" : "fails");
	mutest_expect(description, mutest_bool_value(res),
			mutest_to_be, expected_result, NULL);
	g_free(description);

	if (expected_extension == NULL) {
		description = g_strdup_printf("%s: extension is set to NULL",
				description_prefix);
		mutest_expect(description, mutest_pointer(ext),
			      mutest_to_be_null, NULL);
		g_free(description);
	} else {
		description = g_strdup_printf("%s: extension is set to: %s",
				description_prefix, expected_extension);
		mutest_expect(description, mutest_string_value(ext),
			      mutest_to_be, expected_extension, NULL);
		g_free(description);
	}

	g_free(ext);
}

static void
get_uri_extension_lowercase(mutest_spec_t *spec G_GNUC_UNUSED)
{
	const gchar *uri;

	uri = "this-is-not-a-valid-uri";
	test_get_uri_extension("invalid uri", uri, false, NULL);

	uri = "https://ice2.somafm.com/metal-128-aac";
	test_get_uri_extension("no extension", uri, true, NULL);

	uri = "https://subfm.radioca.st/Sub.FM";
	test_get_uri_extension("uppercase extension", uri, true, "fm");

	uri = "https://broadcast.radioponiente.org:8034/;listen.pls";
	test_get_uri_extension("semi-colon in path", uri, true, "pls");
}

static void
utils_suite(mutest_suite_t *suite G_GNUC_UNUSED)
{
	mutest_it("is_uri_scheme_supported", is_uri_scheme_supported);
	mutest_it("get_uri_extension_lowercase", get_uri_extension_lowercase);
}

MUTEST_MAIN(
	log_init(NULL, TRUE, NULL);
	mutest_describe("utils", utils_suite);
)
