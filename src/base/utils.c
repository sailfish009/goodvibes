/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2018-2021 Arnaud Rebillout
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

#include <gio/gio.h>
#include <glib.h>

#include "config.h"
#include "log.h"

/*
 * URI utils
 */

const gchar *GV_SUPPORTED_URI_SCHEMES[] = {
	"http", "https", NULL
};

const gchar *GV_SUPPORTED_MIME_TYPES[] = {
	"audio/*", NULL
};

gboolean
gv_is_uri_scheme_supported(const gchar *uri)
{
	const gchar **schemes = GV_SUPPORTED_URI_SCHEMES;
	const gchar *scheme;
	const gchar *uri_scheme;

	uri_scheme = g_uri_peek_scheme(uri);

	while (schemes && (scheme = *schemes++))
		if (g_strcmp0(scheme, uri_scheme) == 0)
			return TRUE;

	return FALSE;
}

/*
 * XDG utils
 */

const gchar *
gv_get_app_user_config_dir(void)
{
	static gchar *dir;

	if (dir == NULL) {
		const gchar *user_dir;

		user_dir = g_get_user_config_dir();
		dir = g_build_filename(user_dir, PACKAGE_NAME, NULL);
	}

	return dir;
}

const gchar *
gv_get_app_user_data_dir(void)
{
	static gchar *dir;

	if (dir == NULL) {
		const gchar *user_dir;

		user_dir = g_get_user_data_dir();
		dir = g_build_filename(user_dir, PACKAGE_NAME, NULL);
	}

	return dir;
}

const gchar *const *
gv_get_app_system_config_dirs(void)
{
	static gchar **dirs;

	if (dirs == NULL) {
		const gchar *const *system_dirs;
		guint i, n_dirs;

		system_dirs = g_get_system_config_dirs();
		n_dirs = g_strv_length((gchar **) system_dirs);
		dirs = g_malloc0_n(n_dirs + 1, sizeof(gchar *));
		for (i = 0; i < n_dirs; i++)
			dirs[i] = g_build_filename(system_dirs[i], PACKAGE_NAME, NULL);
	}

	return (const gchar *const *) dirs;
}

const gchar *const *
gv_get_app_system_data_dirs(void)
{
	static gchar **dirs;

	if (dirs == NULL) {
		const gchar *const *system_dirs;
		guint i, n_dirs;

		system_dirs = g_get_system_data_dirs();
		n_dirs = g_strv_length((gchar **) system_dirs);
		dirs = g_malloc0_n(n_dirs + 1, sizeof(gchar *));
		for (i = 0; i < n_dirs; i++)
			dirs[i] = g_build_filename(system_dirs[i], PACKAGE_NAME, NULL);
	}

	return (const gchar *const *) dirs;
}

/*
 * Misc utils
 */

GSettings *
gv_get_settings(const gchar *component)
{
	gchar *schema_id;
	GSettings *settings;

	schema_id = g_strdup_printf("%s.%s", GV_APPLICATION_ID, component);
	settings = g_settings_new(schema_id);
	g_free(schema_id);

	return settings;
}

gboolean
gv_in_test_suite(void)
{
	gchar **env, **ptr;
	gboolean ret = FALSE;

	env = g_listenv();
	for (ptr = env; ptr && *ptr; ptr++) {
		if (!g_strcmp0(*ptr, "GOODVIBES_IN_TEST_SUITE")) {
			ret = TRUE;
			break;
		}
	}
	g_strfreev(env);
	return ret;
}
