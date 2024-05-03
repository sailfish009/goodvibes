/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2024 Arnaud Rebillout
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
#include <glib.h>

#include "config.h"
#include "glib-additions.h"
#include "glib-object-additions.h"
#include "gv-base-enum-types.h"
#include "gv-configurable.h"
#include "gv-feature.h"
#include "gv-param-specs.h"
#include "log.h"
#include "utils.h"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_NAME,
	PROP_FLAGS,
	PROP_SETTINGS,
	PROP_ENABLED,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvFeaturePrivate {
	/* Properties */
	gchar *name;
	GvFeatureFlags flags;
	GSettings *settings;
	gboolean enabled;
};

typedef struct _GvFeaturePrivate GvFeaturePrivate;

static void gv_feature_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GvFeature, gv_feature, G_TYPE_OBJECT,
				 G_ADD_PRIVATE(GvFeature)
				 G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
						       gv_feature_configurable_interface_init))

#define GET_PRIV(x) ((GvFeaturePrivate *) gv_feature_get_instance_private(x))

/*
 * Property accessors
 */

const gchar *
gv_feature_get_name(GvFeature *self)
{
	g_return_val_if_fail(GV_IS_FEATURE(self), NULL);

	return GET_PRIV(self)->name;
}

static void
gv_feature_set_name(GvFeature *self, const gchar *name)
{
	GvFeaturePrivate *priv = GET_PRIV(self);

	/* Construct-only property */
	g_return_if_fail(priv->name == NULL);
	g_return_if_fail(name != NULL);

	priv->name = g_strdup(name);
}

GvFeatureFlags
gv_feature_get_flags(GvFeature *self)
{
	g_return_val_if_fail(GV_IS_FEATURE(self), 0);

	return GET_PRIV(self)->flags;
}

static void
gv_feature_set_flags(GvFeature *self, GvFeatureFlags flags)
{
	GvFeaturePrivate *priv = GET_PRIV(self);

	/* Construct-only property */
	g_return_if_fail(priv->flags == GV_FEATURE_DEFAULT);

	priv->flags = flags;
}

GSettings *
gv_feature_get_settings(GvFeature *self)
{
	g_return_val_if_fail(GV_IS_FEATURE(self), NULL);

	return GET_PRIV(self)->settings;
}

gboolean
gv_feature_get_enabled(GvFeature *self)
{
	g_return_val_if_fail(GV_IS_FEATURE(self), FALSE);

	return GET_PRIV(self)->enabled;
}

void
gv_feature_set_enabled(GvFeature *self, gboolean enabled)
{
	GvFeatureClass *feature_class;
	GvFeaturePrivate *priv;

	g_return_if_fail(GV_IS_FEATURE(self));

	feature_class = GV_FEATURE_GET_CLASS(self);
	priv = GET_PRIV(self);

	if (priv->enabled == enabled)
		return;

	priv->enabled = enabled;

	/* Call the virtual method */
	if (enabled == TRUE) {
		INFO("Enabling feature '%s'...", priv->name);
		feature_class->enable(self);
	} else {
		INFO("Disabling feature '%s'...", priv->name);
		feature_class->disable(self);
	}

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ENABLED]);
}

static void
gv_feature_get_property(GObject *object,
			guint property_id,
			GValue *value,
			GParamSpec *pspec)
{
	GvFeature *self = GV_FEATURE(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NAME:
		g_value_set_string(value, gv_feature_get_name(self));
		break;
	case PROP_FLAGS:
		g_value_set_flags(value, gv_feature_get_flags(self));
		break;
	case PROP_SETTINGS:
		g_value_set_object(value, gv_feature_get_settings(self));
		break;
	case PROP_ENABLED:
		g_value_set_boolean(value, gv_feature_get_enabled(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_feature_set_property(GObject *object,
			guint property_id,
			const GValue *value,
			GParamSpec *pspec)
{
	GvFeature *self = GV_FEATURE(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_NAME:
		gv_feature_set_name(self, g_value_get_string(value));
		break;
	case PROP_FLAGS:
		gv_feature_set_flags(self, g_value_get_flags(value));
		break;
	case PROP_ENABLED:
		gv_feature_set_enabled(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

GvFeature *
gv_feature_new(GType object_type, const gchar *name, GvFeatureFlags flags)
{
	return g_object_new(object_type,
			    "name", name,
			    "flags", flags,
			    NULL);
}

/*
 * GvConfigurable interface
 */

static void
gv_feature_configure(GvConfigurable *configurable)
{
	GvFeature *self = GV_FEATURE(configurable);
	GvFeaturePrivate *priv = GET_PRIV(self);

	TRACE("%p", self);

	g_assert(priv->settings != NULL);
	g_settings_bind(priv->settings, "enabled", self, "enabled",
			G_SETTINGS_BIND_DEFAULT);
}

static void
gv_feature_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_feature_configure;
}

/*
 * GObject methods
 */

static void
gv_feature_finalize(GObject *object)
{
	GvFeature *self = GV_FEATURE(object);
	GvFeaturePrivate *priv = GET_PRIV(self);

	TRACE("%p", object);

	/* Disable */
	if (priv->enabled)
		gv_feature_set_enabled(self, FALSE);

	/* Free resources */
	g_object_unref(priv->settings);
	g_free(priv->name);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_feature, object);
}

static void
gv_feature_constructed(GObject *object)
{
	GvFeature *self = GV_FEATURE(object);
	GvFeatureClass *class = GV_FEATURE_GET_CLASS(self);
	GvFeaturePrivate *priv = GET_PRIV(self);
	gchar *schema_id_suffix;

	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_feature, object);

	/* Ensure virtual methods are implemented */
	g_assert(class->enable != NULL);
	g_assert(class->disable != NULL);

	/* Ensure construct-only properties have been set */
	g_assert(priv->name != NULL);

	/* Create settings */
	schema_id_suffix = g_strjoin(".", "Feat", priv->name, NULL);
	priv->settings = gv_get_settings(schema_id_suffix);
	g_free(schema_id_suffix);
}

static void
gv_feature_init(GvFeature *self)
{
	TRACE("%p", self);
}

static void
gv_feature_class_init(GvFeatureClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_feature_finalize;
	object_class->constructed = gv_feature_constructed;

	/* Properties */
	object_class->get_property = gv_feature_get_property;
	object_class->set_property = gv_feature_set_property;

	properties[PROP_NAME] =
		g_param_spec_string("name", "Name", NULL,
				    NULL,
				    GV_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_FLAGS] =
		g_param_spec_flags("flags", "Feature flags", NULL,
				   GV_TYPE_FEATURE_FLAGS,
				   GV_FEATURE_DEFAULT,
				   GV_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	properties[PROP_SETTINGS] =
		g_param_spec_object("settings", "Settings", NULL,
				    G_TYPE_SETTINGS,
				    GV_PARAM_READABLE);

	properties[PROP_ENABLED] =
		g_param_spec_boolean("enabled", "Enabled", NULL,
				     FALSE,
				     GV_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
