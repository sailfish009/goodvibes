/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
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

#include <signal.h>

#include <glib.h>
#include <glib-object.h>

#include "libcaphe/caphe.h"

#include "framework/gv-framework.h"
#include "core/gv-core.h"

#include "feat/gv-inhibitor.h"

/*
 * GObject definitions
 */

struct _GvInhibitorPrivate {
	guint    check_playback_status_source_id;
	gboolean error_emited;
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
/*
 * Private method
 */

static void
gv_inhibitor_check_playback_status_now(GvInhibitor *self G_GNUC_UNUSED)
{
	GvPlayerState player_state = gv_player_get_state(gv_core_player);

	/* Not interested about the transitional states */
	if (player_state == GV_PLAYER_STATE_PLAYING)
		caphe_cup_inhibit(caphe_cup_get_default(), "Playing");

	else if (player_state == GV_PLAYER_STATE_STOPPED)
		caphe_cup_uninhibit(caphe_cup_get_default());
}

static gboolean
when_timeout_check_playback_status(GvInhibitor *self)
{
	GvInhibitorPrivate *priv = self->priv;

	gv_inhibitor_check_playback_status_now(self);

	priv->check_playback_status_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_inhibitor_check_playback_status_delayed(GvInhibitor *self)
{
	GvInhibitorPrivate *priv = self->priv;

	if (priv->check_playback_status_source_id > 0)
		g_source_remove(priv->check_playback_status_source_id);

	priv->check_playback_status_source_id =
	        g_timeout_add_seconds(1, (GSourceFunc) when_timeout_check_playback_status, self);
}

/*
 * Signal handlers & callbacks
 */

static void
on_caphe_cup_inhibit_failure(CapheCup    *caphe_cup G_GNUC_UNUSED,
                             GvInhibitor *self)
{
	GvInhibitorPrivate *priv = self->priv;

	WARNING("Failed to inhibit system sleep");

	if (priv->error_emited)
		return;

	gv_errorable_emit_error(GV_ERRORABLE(self), _("Failed to inhibit system sleep"));
	priv->error_emited = TRUE;
}

static void
on_caphe_cup_notify_inhibitor(CapheCup    *caphe_cup,
                              GParamSpec  *pspec G_GNUC_UNUSED,
                              GvInhibitor *self G_GNUC_UNUSED)
{
	CapheInhibitor *inhibitor = caphe_cup_get_inhibitor(caphe_cup);

	if (inhibitor)
		INFO("System sleep inhibited (%s)", caphe_inhibitor_get_name(inhibitor));
	else
		INFO("System sleep uninhibited");
}

static void
on_player_notify_state(GvPlayer    *player,
                       GParamSpec  *pspec G_GNUC_UNUSED,
                       GvInhibitor *self)
{
	GvPlayerState player_state = gv_player_get_state(player);

	/* Not interested about the transitional states */
	if (player_state != GV_PLAYER_STATE_PLAYING &&
	    player_state != GV_PLAYER_STATE_STOPPED)
		return;

	/* We might take action now, however we delay our decision a bit,
	 * just in case player state is changing fast and getting crazy.
	 */
	gv_inhibitor_check_playback_status_delayed(self);
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

	/* Remove pending operation */
	if (priv->check_playback_status_source_id) {
		g_source_remove(priv->check_playback_status_source_id);
		priv->check_playback_status_source_id = 0;
	}

	/* Reset error emission */
	priv->error_emited = FALSE;

	/* Signal handlers */
	g_signal_handlers_disconnect_by_data(player, feature);

	/* Cleanup libcaphe */
	g_signal_handlers_disconnect_by_data(caphe_cup_get_default(), self);
	caphe_cleanup();

	/* Chain up */
	GV_FEATURE_CHAINUP_DISABLE(gv_inhibitor, feature);
}

static void
gv_inhibitor_enable(GvFeature *feature)
{
	GvInhibitor *self = GV_INHIBITOR(feature);
	GvInhibitorPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	/* Chain up */
	GV_FEATURE_CHAINUP_ENABLE(gv_inhibitor, feature);

	/* Init libcaphe */
	caphe_init();
	g_signal_connect_object(caphe_cup_get_default(), "notify::inhibitor",
	                        G_CALLBACK(on_caphe_cup_notify_inhibitor), self, 0);
	g_signal_connect_object(caphe_cup_get_default(), "inhibit-failure",
	                        G_CALLBACK(on_caphe_cup_inhibit_failure), self, 0);

	/* Connect to signal handlers */
	g_signal_connect_object(player, "notify::state", G_CALLBACK(on_player_notify_state),
	                        self, 0);

	/* Schedule a check for the current playback status */
	g_assert(priv->check_playback_status_source_id == 0);
	priv->check_playback_status_source_id =
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
