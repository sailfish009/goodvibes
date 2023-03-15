/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
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

#include <string.h>

#include <glib.h>

/*
 * String Utilities Functions
 */

gchar *
g_strjoin_null(const gchar *separator, unsigned int n_strings, ...)
{
	gchar *string, *s;
	va_list args;
	gsize len;
	gsize separator_len;
	gchar *ptr;
	guint i;

	if (separator == NULL)
		separator = "";

	separator_len = strlen(separator);

	/* First part, getting length */
	len = 1;
	va_start(args, n_strings);
	for (i = 0; i < n_strings; i++) {
		s = va_arg(args, gchar *);
		if (s) {
			if (len != 1)
				len += separator_len;
			len += strlen(s);
		}
	}
	va_end(args);

	/* Second part, building string */
	string = g_new(gchar, len);
	ptr = string;
	va_start(args, n_strings);
	for (i = 0; i < n_strings; i++) {
		s = va_arg(args, gchar *);
		if (s) {
			if (ptr != string)
				ptr = g_stpcpy(ptr, separator);
			ptr = g_stpcpy(ptr, s);
		}
	}
	va_end(args);

	if (ptr == string)
		string[0] = '\0';

	return string;
}

/*
 * GVariant
 */

void
g_variant_builder_add_dictentry_array_string(GVariantBuilder *b, const gchar *key, ...)
{
	GVariantBuilder ab;
	va_list ap;
	gchar *s;

	g_variant_builder_init(&ab, G_VARIANT_TYPE_ARRAY);

	va_start(ap, key);
	while ((s = va_arg(ap, gchar *)) != NULL)
		g_variant_builder_add(&ab, "s", s);

	g_variant_builder_add(b, "{sv}", key, g_variant_builder_end(&ab));
}
