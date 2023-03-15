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

#pragma once

#include <glib.h>

/*
 * String Utilities Functions
 */

gchar *g_strjoin_null(const gchar *separator, unsigned int n_strings, ...);

/*
 * GVariant
 */

#define g_variant_builder_add_dictentry_string(b, key, val)             \
        g_variant_builder_add(b, "{sv}", key, g_variant_new_string(val))

#define g_variant_builder_add_dictentry_object_path(b, key, val)        \
        g_variant_builder_add(b, "{sv}", key, g_variant_new_object_path(val))

void g_variant_builder_add_dictentry_array_string(GVariantBuilder *b,
                                                  const gchar     *key,
                                                  ...) G_GNUC_NULL_TERMINATED;
