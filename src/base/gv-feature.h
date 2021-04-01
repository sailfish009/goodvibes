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

#include <glib-object.h>
#include <gio/gio.h>

/* GObject declarations */

#define GV_TYPE_FEATURE gv_feature_get_type()

G_DECLARE_DERIVABLE_TYPE(GvFeature, gv_feature, GV, FEATURE, GObject)

/* Chain up macros */

#define GV_FEATURE_CHAINUP_ENABLE(type_name, obj)        \
        do { \
                if (GV_FEATURE_CLASS(type_name##_parent_class)->enable) \
                        GV_FEATURE_CLASS(type_name##_parent_class)->enable(obj); \
        } while (0)

#define GV_FEATURE_CHAINUP_DISABLE(type_name, obj)       \
        do { \
                if (GV_FEATURE_CLASS(type_name##_parent_class)->disable) \
                        GV_FEATURE_CLASS(type_name##_parent_class)->disable(obj); \
        } while (0)

/* Data types */

typedef enum { /*< flags >*/
	GV_FEATURE_DEFAULT,
	GV_FEATURE_EARLY,
} GvFeatureFlags;

struct _GvFeatureClass {
	/* Parent class */
	GObjectClass parent_class;
	/* Virtual methods */
	void (*enable) (GvFeature *);
	void (*disable)(GvFeature *);
};

/* Public methods */

GvFeature *gv_feature_new(GType object_type, const gchar *name, GvFeatureFlags flags);

/* Property accessors */

const gchar   *gv_feature_get_name    (GvFeature *self);
GvFeatureFlags gv_feature_get_flags   (GvFeature *self);
GSettings     *gv_feature_get_settings(GvFeature *self);
gboolean       gv_feature_get_enabled (GvFeature *self);
void           gv_feature_set_enabled (GvFeature *self, gboolean enabled);
