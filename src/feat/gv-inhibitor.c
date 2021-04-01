/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2021 Arnaud Rebillout
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <signal.h>

#include <glib.h>
#include <glib-object.h>

#include "base/gv-base.h"
#include "core/gv-core.h"

#include "feat/gv-inhibitor.h"
#include "feat/gv-inhibitor-impl.h"

/*
 * GObject definitions
 */

const gchar *gv_inhibitor_implementations[] = { "gtk", "pm", NULL };

struct _GvInhibitorPrivate {
	GvInhibitorImpl *impl;
	gboolean         no_impl_available;
	guint            check_playback_status_timeout_id;
};

typedef struct _GvInhibitorPrivate GvInhibitorPrivate;

struct _GvInhibitor {
	/* Parent instance structure */
	GvFeature           parent_instance;
	/* Private data */
	GvInhibitorPrivate *priv;
};

G_DEFINE_TYPE_WITH_CODE(GvInhibitor, gv_inhibitor, GV_TYPE_FEATURE,
                        G_ADD_PRIVATE(GvInhibitor)
                        G_IMPLEMENT_INTERFACE(GV_TYPE_ERRORABLE, NULL))

static void
gv_inhibitor_inhibit(GvInhibitor *self, const gchar *reason)
{
	GvInhibitorPrivate *priv = self->priv;

	TRACE("%p, %s", self, reason);

	if (priv->no_impl_available) {
		/* We tried everything already */
		DEBUG("No implementation available");
		return;
	}

	if (priv->impl) {
		/* We have a known-working implementation */
		GError *err = NULL;
		gboolean ret;

		ret = gv_inhibitor_impl_inhibit(priv->impl, reason, &err);

		if (ret == TRUE) {
			DEBUG("Inhibited system sleep (%s)",
				gv_inhibitor_impl_get_name(priv->impl));
		} else {
			DEBUG("Failed to inhibit system sleep (%s): %s",
				gv_inhibitor_impl_get_name(priv->impl),
				err ? err->message : "unknown error");
			g_clear_error(&err);
		}

		return;
	}

	/* This is the init case, let's iterate over the known implementation
	 * until we find one that works. The only way to know is to try.
	 */
	const gchar **impls = gv_inhibitor_implementations;
	const gchar *impl;
	while ((impl = *impls++) != NULL) {
		GError *err = NULL;
		gboolean ret;

		DEBUG("Trying to inhibit with the '%s' implementation", impl);

		priv->impl = gv_inhibitor_impl_make(impl);
		ret = gv_inhibitor_impl_inhibit(priv->impl, reason, &err);

		if (ret == TRUE) {
			DEBUG("Inhibited system sleep (%s)",
				gv_inhibitor_impl_get_name(priv->impl));
			break;
		} else {
			DEBUG("Failed to inhibit system sleep (%s): %s",
				gv_inhibitor_impl_get_name(priv->impl),
				err ? err->message : "unknown error");
			g_clear_object(&priv->impl);
			g_clear_error(&err);
		}
	}

	if (impl == NULL) {
		priv->no_impl_available = TRUE;
		gv_errorable_emit_error(GV_ERRORABLE(self),
				_("Failed to inhibit system sleep"));
	}
}

static void
gv_inhibitor_uninhibit(GvInhibitor *self)
{
	GvInhibitorPrivate *priv = self->priv;

	TRACE("%p", self);

	if (priv->impl)
		gv_inhibitor_impl_uninhibit(priv->impl);
}

static void
gv_inhibitor_check_playback_status_now(GvInhibitor *self)
{
	GvPlayerState player_state = gv_player_get_state(gv_core_player);

	if (player_state == GV_PLAYER_STATE_PLAYING)
		gv_inhibitor_inhibit(self, _("Playing"));
	else
		gv_inhibitor_uninhibit(self);
}

static gboolean
when_timeout_check_playback_status(GvInhibitor *self)
{
	GvInhibitorPrivate *priv = self->priv;

	gv_inhibitor_check_playback_status_now(self);
	priv->check_playback_status_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_inhibitor_check_playback_status_delayed(GvInhibitor *self, guint delay)
{
	GvInhibitorPrivate *priv = self->priv;

	g_clear_handle_id(&priv->check_playback_status_timeout_id, g_source_remove);
	priv->check_playback_status_timeout_id =
	        g_timeout_add_seconds(delay, (GSourceFunc) when_timeout_check_playback_status, self);
}

/*
 * Signal handlers & callbacks
 */

static void
on_player_notify_state(GvPlayer    *player,
                       GParamSpec  *pspec G_GNUC_UNUSED,
                       GvInhibitor *self)
{
	GvPlayerState player_state = gv_player_get_state(player);

	/* 'connecting' and 'buffering' are intermediary states, so let's wait
	 * a bit handling it, but let's schedule a callback all the same, just
	 * in case the player gets stuck in one of those states for some reason.
	 */
	if (player_state == GV_PLAYER_STATE_CONNECTING ||
	    player_state == GV_PLAYER_STATE_BUFFERING)
		gv_inhibitor_check_playback_status_delayed(self, 10);
	else
		gv_inhibitor_check_playback_status_delayed(self, 1);
}

/*
 * Feature methods
 */

static void
gv_inhibitor_disable(GvFeature *feature)
{
	GvInhibitor *self = GV_INHIBITOR(feature);
	GvInhibitorPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	TRACE("%p", feature);

	/* Remove pending operation */
	g_clear_handle_id(&priv->check_playback_status_timeout_id, g_source_remove);

	/* Cleanup */
	g_clear_object(&priv->impl);
	priv->no_impl_available = FALSE;

	/* Signal handlers */
	g_signal_handlers_disconnect_by_data(player, feature);

	/* Chain up */
	GV_FEATURE_CHAINUP_DISABLE(gv_inhibitor, feature);
}

static void
gv_inhibitor_enable(GvFeature *feature)
{
	GvInhibitor *self = GV_INHIBITOR(feature);
	GvInhibitorPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	TRACE("%p", feature);

	/* Chain up */
	GV_FEATURE_CHAINUP_ENABLE(gv_inhibitor, feature);

	/* Connect to signal handlers */
	g_signal_connect_object(player, "notify::state", G_CALLBACK(on_player_notify_state),
	                        self, 0);

	/* Schedule a check for the current playback status */
	g_assert(priv->check_playback_status_timeout_id == 0);
	priv->check_playback_status_timeout_id =
	        g_timeout_add_seconds(1, (GSourceFunc) when_timeout_check_playback_status, self);
}

/*
 * Public methods
 */

GvFeature *
gv_inhibitor_new(void)
{
	return gv_feature_new(GV_TYPE_INHIBITOR, "Inhibitor", GV_FEATURE_DEFAULT);
}

/*
 * GObject methods
 */

static void
gv_inhibitor_init(GvInhibitor *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_inhibitor_get_instance_private(self);
}

static void
gv_inhibitor_class_init(GvInhibitorClass *class)
{
	GvFeatureClass *feature_class = GV_FEATURE_CLASS(class);

	TRACE("%p", class);

	/* Override GvFeature methods */
	feature_class->enable = gv_inhibitor_enable;
	feature_class->disable = gv_inhibitor_disable;
}
