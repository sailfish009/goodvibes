/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2018 Arnaud Rebillout
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

#include <gio/gio.h>

#include "config.h"
#include "log.h"

#define GV_OLD_APPLICATION_ID "com.elboulangero.Goodvibes"

static GSettings *
get_current_settings(const gchar *component)
{
	gchar *schema_id;
	GSettings *settings;

	schema_id = g_strdup_printf("%s.%s", GV_APPLICATION_ID, component);
	settings = g_settings_new(schema_id);
	g_free(schema_id);

	return settings;
}

static GSettings *
get_old_settings(const gchar *component)
{
	gchar *schema_id;
	GSettingsSchema *schema;
	GSettingsSchemaSource *source;

	source = g_settings_schema_source_get_default();
	if (source == NULL)
		return NULL;

	schema_id = g_strdup_printf("%s.%s", GV_OLD_APPLICATION_ID, component);
	schema = g_settings_schema_source_lookup(source, schema_id, TRUE);
	g_free(schema_id);

	if (schema == NULL)
		return NULL;

	return g_settings_new_full(schema, NULL, NULL);
}

static void
merge_settings(const gchar *component, GSettings *from, GSettings *to)
{
	gint i;
	gchar **keys;
	gboolean changed;
	GSettingsSchema *schema;

	g_object_get(from, "settings-schema", &schema, NULL);
	keys = g_settings_schema_list_keys(schema);
	changed = FALSE;

	for (i = 0; keys[i]; i++) {
		const gchar *key;
		GVariant *value;

		key = keys[i];
		value = g_settings_get_user_value(from, key);
		if (value == NULL)
			continue;

		INFO("Settings migration: %s: moving key '%s'", component, key);
		g_settings_set_value(to, key, value);
		g_settings_reset(from, key);
		g_variant_unref(value);
		changed = TRUE;
	}

	if (changed) {
		INFO("Settings migration: %s: syncing", component);
		g_settings_sync();
	}

	g_strfreev(keys);
	g_settings_schema_unref(schema);
}

GSettings *
gv_get_settings(const gchar *component)
{
	GSettings *settings, *old_settings;

	settings = get_current_settings(component);
	old_settings = get_old_settings(component);

	if (old_settings) {
		merge_settings(component, old_settings, settings);
		g_object_unref(old_settings);
	}

	return settings;
}
