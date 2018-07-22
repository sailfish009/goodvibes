/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2018 Arnaud Rebillout
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

#include "framework/gv-framework.h"

#ifdef GV_FEAT_CONSOLE_OUTPUT
#include "feat/gv-console-output.h"
#endif
#ifdef GV_FEAT_DBUS_SERVER
#include "feat/gv-dbus-server-native.h"
#include "feat/gv-dbus-server-mpris2.h"
#endif
#ifdef GV_FEAT_HOTKEYS
#include "feat/gv-hotkeys.h"
#endif
#ifdef GV_FEAT_INHIBITOR
#include "feat/gv-inhibitor.h"
#endif
#ifdef GV_FEAT_NOTIFICATIONS
#include "feat/gv-notifications.h"
#endif

static GList *feat_objects;

/*
 * Public functions
 */

GvFeature *
gv_feat_find(const gchar *name_to_find)
{
	GList *item;

	for (item = feat_objects; item; item = item->next) {
		GvFeature *feature = GV_FEATURE(item->data);
		const gchar *feature_name = gv_feature_get_name(feature);

		if (!g_strcmp0(feature_name, name_to_find))
			return feature;
	}

	return NULL;
}

void
gv_feat_configure_early(void)
{
	GList *item;

	for (item = feat_objects; item; item = item->next) {
		GvFeature *feature = GV_FEATURE(item->data);
		GvFeatureFlags flags = gv_feature_get_flags(feature);

		if (flags & GV_FEATURE_EARLY)
			gv_configurable_configure(GV_CONFIGURABLE(feature));
	}
}

void
gv_feat_configure_late(void)
{
	GList *item;

	for (item = feat_objects; item; item = item->next) {
		GvFeature *feature = GV_FEATURE(item->data);
		GvFeatureFlags flags = gv_feature_get_flags(feature);

		if (flags & GV_FEATURE_EARLY)
			continue;

		gv_configurable_configure(GV_CONFIGURABLE(feature));
	}
}

void
gv_feat_cleanup(void)
{
	feat_objects = g_list_reverse(feat_objects);
	g_list_foreach(feat_objects, (GFunc) g_object_unref, NULL);
	g_list_free(feat_objects);
}

void
gv_feat_init(void)
{
	GvFeature *feature;
	GList *item;

	/*
	 * Create every feature enabled at compile-time.
	 *
	 * Notice that some features don't really make sense if the program
	 * has been compiled without ui. However we don't bother about that
	 * here: all features are equals !
	 * The distinction between 'core' features and 'ui' features is done
	 * in the build system, see the `configure.ac` for more details.
	 */

#ifdef GV_FEAT_CONSOLE_OUTPUT
	feature = gv_console_output_new();
	feat_objects = g_list_append(feat_objects, feature);
#endif
#ifdef GV_FEAT_DBUS_SERVER
	feature = gv_dbus_server_native_new();
	feat_objects = g_list_append(feat_objects, feature);

	feature = gv_dbus_server_mpris2_new();
	feat_objects = g_list_append(feat_objects, feature);
#endif
#ifdef GV_FEAT_INHIBITOR
	feature = gv_inhibitor_new();
	feat_objects = g_list_append(feat_objects, feature);
#endif
#ifdef GV_FEAT_HOTKEYS
	feature = gv_hotkeys_new();
	feat_objects = g_list_append(feat_objects, feature);
#endif
#ifdef GV_FEAT_NOTIFICATIONS
	feature = gv_notifications_new();
	feat_objects = g_list_append(feat_objects, feature);
#endif

	/* Register objects in the framework */
	for (item = feat_objects; item; item = item->next) {
		feature = GV_FEATURE(item->data);
		gv_framework_register_object(feature);

		/* Drop a line */
		INFO("Feature compiled in: '%s'", gv_feature_get_name(feature));
	}
}
