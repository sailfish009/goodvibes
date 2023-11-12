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

#include <glib-object.h>
#include <glib.h>

#include "base/glib-object-additions.h"
#include "base/log.h"
#include "core/gv-playback.h"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	// TODO fill with your properties
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

/*
 * GObject definitions
 */

struct _GvPlaybackPrivate {
	// TODO fill with your data
};

typedef struct _GvPlaybackPrivate GvPlaybackPrivate;

struct _GvPlayback {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvPlaybackPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvPlayback, gv_playback, G_TYPE_OBJECT)

/*
 * Helpers
 */

/*
 * Signal handlers & callbacks
 */

/*
 * Property accessors
 */

static void
gv_playback_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GvPlayback *self = GV_PLAYBACK(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	// TODO handle properties
	(void) self;
	(void) value;

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_playback_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GvPlayback *self = GV_PLAYBACK(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	// TODO handle properties
	(void) self;
	(void) value;

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

GvPlayback *
gv_playback_new(void)
{
	return g_object_new(GV_TYPE_PLAYBACK, NULL);
}

/*
 * GObject methods
 */

static void
gv_playback_finalize(GObject *object)
{
	GvPlayback *self = GV_PLAYBACK(object);
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p", object);

	// TODO job to be done
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_playback, object);
}

static void
gv_playback_constructed(GObject *object)
{
	GvPlayback *self = GV_PLAYBACK(object);
	GvPlaybackPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Initialize properties */
	// TODO
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_playback, object);
}

static void
gv_playback_init(GvPlayback *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_playback_get_instance_private(self);
}

static void
gv_playback_class_init(GvPlaybackClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_playback_finalize;
	object_class->constructed = gv_playback_constructed;

	/* Properties */
	object_class->get_property = gv_playback_get_property;
	object_class->set_property = gv_playback_set_property;

	// TODO define your properties here
	//      use GV_PARAM_{READABLE,WRITABLE,READWRITE}

	g_object_class_install_properties(object_class, PROP_N, properties);
}
