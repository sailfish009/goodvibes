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

#include <glib-object.h>

void
gv_value_transform_enum_string(const GValue *src_value, GValue *dest_value)
{
	GEnumClass *enum_class;
	GEnumValue *enum_value;

	enum_class = g_type_class_ref(G_VALUE_TYPE(src_value));
	enum_value = g_enum_get_value(enum_class, g_value_get_enum(src_value));

	if (enum_value)
		g_value_set_static_string(dest_value, enum_value->value_nick);
	else {
		/* Assume zero holds the invalid value */
		enum_value = g_enum_get_value(enum_class, 0);
		g_value_set_static_string(dest_value, enum_value->value_nick);
	}

	g_type_class_unref(enum_class);
}

void
gv_value_transform_string_enum(const GValue *src_value, GValue *dest_value)
{
	GEnumClass *enum_class;
	GEnumValue *enum_value;

	enum_class = g_type_class_ref(G_VALUE_TYPE(dest_value));
	enum_value = g_enum_get_value_by_nick(enum_class, g_value_get_string(src_value));

	if (enum_value)
		g_value_set_enum(dest_value, enum_value->value);
	else
		/* Assume zero holds the invalid value */
		g_value_set_enum(dest_value, 0);

	g_type_class_unref(enum_class);
}
