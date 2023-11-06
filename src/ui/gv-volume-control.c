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

#include <math.h>

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "base/glib-object-additions.h"
#include "base/log.h"
#include "core/gv-core.h"
#include "ui/gv-volume-control.h"

#define VOLUME_SCALE_WIDTH 90
#define SCROLLING_DELAY 500 // ms

/**
 * GvVolumeControl:
 *
 * A widget to change volume, with a mute button and a volume bar.
 *
 * `GvVolumeControl` is similar to the volume controls that one can find in
 * most web media players out there, eg. YouTube, Vimeo, Bandcamp, etc... It
 * consists of a mute button next to a volume bar (a scale in Gtk-speak). Users
 * click the button to mute or unmute, and click the volume bar to set the
 * volume. So far so good, but there's a bit more to it.
 *
 * When mute is clicked, the volume bar goes to zero, so that (1) it's more
 * explicit, visually, that there's no sound, and (2) it's possible to unmute
 * at any volume, by clicking anywhere on the volume bar. Of course, clicking
 * mute again will unmute at the previous volume value.
 *
 * When the volume is brought to zero (by dragging the slider, or scrolling on
 * the widget), the button is set to to "mute", so that it's possible to
 * restore a non-zero volume value by clicking again on the mute button. This
 * is actually tricky, because it means that we must remember such a non-zero
 * volume value (we call it "fallback volume" in the code). How to exactly keep
 * track of the last non-zero volume value differs depending on how the volume
 * is modified. For a simple click, or when dragging the slider, we can record
 * the volume when the click is released.  However when scrolling, we have to
 * be a bit smarter, and rely on a timeout, so that we record the volume when
 * it settles. If we didn't do that, we'd record the last non-zero volume value
 * before it reaches zero (ie. almost zero), so unmuting would take us to
 * "almost muted".
 *
 * ## Test check list
 *
 * Because of the behavior described above, the list of things to check in
 * order to test this widget is a bit daunting.
 *
 * Test setting the volume with:
 * - a simple click
 * - click then drag the slider (go to zero and back, without releasing)
 * - scroll on the slider (go to zero and back, don't stop scrolling)
 * - externally from cmdline
 *
 * The volume scale must move gracefully (no jittering), the icon for the mute
 * button must change to "muted" when the volume reaches zero.
 *
 * Test mute and unmute:
 * - by clicking the button twice
 * - click then drag the slider down to zero, release click, then click the
 *   button to unmute
 * - scroll on the slider down to zero, then click the button to unmute
 * - externally from cmdline
 *
 * The volume scale must be shown as zero when muted. Then unmuting should
 * restore the correct volume value.
 *
 * Additionally:
 * - test that, when muted, a click on the volume scale is enough to unmute
 * - on fresh start, and when the volume was never modified yet:
 *   - what about mute/unmute, does that work?
 *   - what about dragging/scrolling the slider to zero, and then unmute?
 *
 * Finally, and as long as there are two "views" in GoodVibes (playlist view
 * and station view):
 * - switch the views back and forth, make sure that the volume/mute states
 *   don't change
 * - switch to the station view, change mute/volume via cmdline, switch back,
 *   make sure the states are correct.
 */

/*
 * GObject definitions
 */

struct _GvVolumeControlPrivate {
	/* Widgets */
	GtkWidget *mute_button;
	GtkWidget *volume_scale;
	/* Internal */
	gboolean volume_scale_clicked;
	gboolean volume_scale_scrolling;
	guint scrolling_timeout_id;
	guint fallback_volume;
};

typedef struct _GvVolumeControlPrivate GvVolumeControlPrivate;

struct _GvVolumeControl {
	/* Parent instance structure */
	GtkBox parent_instance;
	/* Private data */
	GvVolumeControlPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvVolumeControl, gv_volume_control, GTK_TYPE_BOX)

static void on_volume_scale_value_changed(GtkRange *range, GvVolumeControl *self);

/*
 * Helpers
 */

static void
set_mute_button(GtkButton *button, gboolean mute, guint volume)
{
	GtkWidget *image;
	const gchar *icon_name;

	if (mute == TRUE || volume == 0)
		icon_name = "audio-volume-muted";
	else if (volume <= 33)
		icon_name = "audio-volume-low";
	else if (volume <= 66)
		icon_name = "audio-volume-medium";
	else
		icon_name = "audio-volume-high";

	image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(button, image);
}

static void
set_volume_scale(GtkScale *scale, guint volume)
{
	GtkRange *range = GTK_RANGE(scale);
	gdouble value = (gdouble) volume;

	gtk_range_set_value(range, value);
}

/*
 * Private methods
 */

static void
gv_volume_control_update(GvVolumeControl *self, gboolean update_mute, gboolean update_volume)
{
	GvVolumeControlPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	gboolean mute;
	guint volume;

	TRACE("%p, %p", self, player);

	mute = gv_player_get_mute(player);
	volume = gv_player_get_volume(player);

	if (update_mute == TRUE) {
		GtkButton *button = GTK_BUTTON(priv->mute_button);

		set_mute_button(button, mute, volume);
	}

	/* When updating the volume scale, make sure to block signal handlers.
	 * Otherwise it will emit signals that will be picked up by our signal
	 * handlers, that will then set the volume. But we're not setting the
	 * volume, we're just moving the slider to reflect the current volume.
	 */
	if (update_volume == TRUE) {
		GtkScale *scale = GTK_SCALE(priv->volume_scale);

		g_signal_handlers_block_by_func(scale, on_volume_scale_value_changed, self);

		/* When muted, we display the volume as zero. This is a special
		 * case where we don't want the GtkScale to reflect the real
		 * volume value.
		 */
		if (mute == TRUE)
			set_volume_scale(scale, 0);
		else
			set_volume_scale(scale, volume);

		g_signal_handlers_unblock_by_func(scale, on_volume_scale_value_changed, self);
	}
}

/*
 * Signal handlers
 */

static void
on_player_notify_mute(GvPlayer *player, GParamSpec *pspec, GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);
	gboolean mute;
	guint volume;

	TRACE("%p, %s, %p", player, property_name, self);

	mute = gv_player_get_mute(player);
	volume = gv_player_get_volume(player);

	/* Handle a corner-case: what if user wants to unmute, however the
	 * volume is zero? Going from "muted" to "unmuted with volume=0"
	 * doesn't get us very far. That's why we have a "fallback volume",
	 * which is basically the last non-zero volume that we know of.
	 *
	 * This situation happens if ever users drags (or scrolls) the slider
	 * down to zero: it results in mute=true and volume=0. That's why we
	 * need to have a fallback volume.
	 *
	 * If even this fallback volume is zero, set the volume to 50%.
	 */
	if (mute == FALSE && volume == 0) {
		volume = priv->fallback_volume;
		if (volume == 0) {
			GtkRange *range = GTK_RANGE(priv->volume_scale);
			GtkAdjustment *adj = gtk_range_get_adjustment(range);
			volume = (guint) (gtk_adjustment_get_upper(adj) / 2);
		}
		DEBUG("Setting volume from fallback: %u", volume);
		gv_player_set_volume(player, volume);
	}

	/* Update widgets */
	gv_volume_control_update(self, TRUE, TRUE);
}

static void
on_player_notify_volume(GvPlayer *player, GParamSpec *pspec, GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);
	gboolean update_mute, update_volume;
	guint volume;

	TRACE("%p, %s, %p", player, property_name, self);

	volume = gv_player_get_volume(player);

	/* We always want to update the mute widget, as the icon depends on
	 * the volume. We might or might not want to update the volume widget,
	 * see below.
	 */
	update_mute = TRUE;
	update_volume = FALSE;

	/* When the volume is set due to a click or scrolling event:
	 * - Don't save the fallback volume now. It will be done later, when
	 *   the operation is finished, ie. click is released, or scrolling is
	 *   over (we have a timeout to decide when it is over).
	 * - Don't update the widget either. Moving the slider programmatically
	 *   while user is moving it at the same time causes some jittering,
	 *   and it's pointless anyway, there's no need for this feedback loop.
	 */
	if (priv->volume_scale_clicked == TRUE) {
		;
	} else if (priv->volume_scale_scrolling == TRUE) {
		;
	} else {
		update_volume = TRUE;
		if (volume != 0)
			priv->fallback_volume = volume;
	}

	/* When the volume goes to zero, we *mute*, so that clicking the mute
	 * button afterwards results in *unmuting*, and restores the fallback
	 * volume.
	 *
	 * When the volume goes to non-zero, make sure to also *unmute*, so
	 * that if ever sound was muted, clicking on the volume scale doesn't
	 * only set the volume, it also unmutes.
	 */
	if (volume == 0)
		gv_player_set_mute(player, TRUE);
	else
		gv_player_set_mute(player, FALSE);

	/* Update widgets */
	gv_volume_control_update(self, update_mute, update_volume);
}

static void
on_mute_button_clicked(GtkButton *button G_GNUC_UNUSED, GvVolumeControl *self G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;

	gv_player_toggle_mute(player);
}

static gboolean
on_volume_scale_button_press_event(GtkWidget *widget G_GNUC_UNUSED, GdkEvent *event G_GNUC_UNUSED, GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;

	priv->volume_scale_clicked = TRUE;

	return GDK_EVENT_PROPAGATE;
}

static gboolean
on_volume_scale_button_release_event(GtkWidget *widget G_GNUC_UNUSED, GdkEvent *event G_GNUC_UNUSED, GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	guint volume;

	priv->volume_scale_clicked = FALSE;

	/* Update the fallback volume */
	volume = gv_player_get_volume(player);
	if (volume != 0)
		priv->fallback_volume = volume;

	return GDK_EVENT_PROPAGATE;
}

static gboolean
when_scrolling_timeout(GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	guint volume;

	priv->volume_scale_scrolling = FALSE;

	/* Update the fallback volume */
	volume = gv_player_get_volume(player);
	if (volume != 0)
		priv->fallback_volume = volume;

	priv->scrolling_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static gboolean
on_volume_scale_scroll_event(GtkWidget *widget G_GNUC_UNUSED, GdkEvent *event G_GNUC_UNUSED, GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;

	priv->volume_scale_scrolling = TRUE;

	/* Schedule a callback to record the fallback volume */
	g_clear_handle_id(&priv->scrolling_timeout_id, g_source_remove);
	priv->scrolling_timeout_id = g_timeout_add(SCROLLING_DELAY,
			(GSourceFunc) when_scrolling_timeout, self);

	return GDK_EVENT_PROPAGATE;
}

static void
on_volume_scale_value_changed(GtkRange *range, GvVolumeControl *self G_GNUC_UNUSED)
{
	GvPlayer *player = gv_core_player;
	gdouble value = round(gtk_range_get_value(range));

	gv_player_set_volume(player, (guint) value);
}

static void
on_map(GvVolumeControl *self, gpointer user_data)
{
	GvVolumeControlPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;
	guint volume;

	TRACE("%p, %p", self, user_data);

	/* Make sure widgets have the right values */
	gv_volume_control_update(self, TRUE, TRUE);

	/* Update the fallback volume */
	volume = gv_player_get_volume(player);
	if (volume != 0)
		priv->fallback_volume = volume;
}

/*
 * Public methods
 */

GtkWidget *
gv_volume_control_new(void)
{
	return g_object_new(GV_TYPE_VOLUME_CONTROL, NULL);
}

/*
 * Construct helpers
 */

static GParamSpecUInt *
get_param_spec_uint(GObject *object, const gchar *property_name)
{
	GObjectClass *object_class;
	GParamSpec *pspec;

	object_class = G_OBJECT_GET_CLASS(object);
	pspec = g_object_class_find_property(object_class, property_name);
	g_assert(pspec != NULL);

	return G_PARAM_SPEC_UINT(pspec);
}

static void
setup_scale_adjustment(GtkScale *scale, GObject *obj, const gchar *obj_prop_name)
{
	GtkAdjustment *adjustment;
	GParamSpecUInt *pspec;
	guint range;

	pspec = get_param_spec_uint(obj, obj_prop_name);
	range = pspec->maximum - pspec->minimum;

	adjustment = gtk_range_get_adjustment(GTK_RANGE(scale));
	gtk_adjustment_set_lower(adjustment, pspec->minimum);
	gtk_adjustment_set_upper(adjustment, pspec->maximum);
	gtk_adjustment_set_step_increment(adjustment, range / 100);
	gtk_adjustment_set_page_increment(adjustment, range / 10);
}

static void
gv_volume_control_populate_widgets(GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	GtkWidget *w;

	/* Add the mute button */
	w = gtk_button_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_relief(GTK_BUTTON(w), GTK_RELIEF_NONE);
	gtk_button_set_always_show_image(GTK_BUTTON(w), TRUE);
	gtk_container_add(GTK_CONTAINER(self), w);
	priv->mute_button = w;

	/* Add the volume scale */
	w = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
	gtk_scale_set_draw_value(GTK_SCALE(w), FALSE);
	gtk_widget_set_size_request(w, VOLUME_SCALE_WIDTH, -1);
	gtk_container_add(GTK_CONTAINER(self), w);
	priv->volume_scale = w;
}

static void
gv_volume_control_setup_appearance(GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	GtkCssProvider *provider;
	GtkStyleContext *context;
	const gchar *css;

	/* For a GTK3 Scale, the default padding is 12px. Let's reduce that a
	 * little on the left, to bring the mute icon and the volume scale
	 * closer to each other, and increase that a bit on the right,
	 * otherwise the button that come next almost touch the knob.
	 */
	css = "scale {padding-left: 6px; padding-right: 24px;}";
	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, css, -1, NULL);

	context = gtk_widget_get_style_context(priv->volume_scale);
	gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(provider);
}

static void
gv_volume_control_setup_widgets(GvVolumeControl *self)
{
	GvVolumeControlPrivate *priv = self->priv;
	GvPlayer *player = gv_core_player;

	/* Setup adjustment for the volume scale */
	setup_scale_adjustment(GTK_SCALE(priv->volume_scale), G_OBJECT(player), "volume");

	/* Connect widgets signal handlers */
	g_signal_connect_object(priv->mute_button, "clicked",
				G_CALLBACK(on_mute_button_clicked), self, 0);
	g_signal_connect_object(priv->volume_scale, "button-press-event",
				G_CALLBACK(on_volume_scale_button_press_event), self, 0);
	g_signal_connect_object(priv->volume_scale, "button-release-event",
				G_CALLBACK(on_volume_scale_button_release_event), self, 0);
	g_signal_connect_object(priv->volume_scale, "scroll-event",
				G_CALLBACK(on_volume_scale_scroll_event), self, 0);
	g_signal_connect_object(priv->volume_scale, "value-changed",
				G_CALLBACK(on_volume_scale_value_changed), self, 0);

	/* Connect self signal handlers */
	g_signal_connect_object(self, "map", G_CALLBACK(on_map), NULL, 0);

	/* Connect player signal handlers */
	g_signal_connect_object(player, "notify::mute",
				G_CALLBACK(on_player_notify_mute), self, 0);
	g_signal_connect_object(player, "notify::volume",
				G_CALLBACK(on_player_notify_volume), self, 0);
}

/*
 * GObject methods
 */

static void
gv_volume_control_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_volume_control, object);
}

static void
gv_volume_control_constructed(GObject *object)
{
	GvVolumeControl *self = GV_VOLUME_CONTROL(object);

	TRACE("%p", object);

	/* Build widget */
	gv_volume_control_populate_widgets(self);
	gv_volume_control_setup_appearance(self);
	gv_volume_control_setup_widgets(self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_volume_control, object);
}

static void
gv_volume_control_init(GvVolumeControl *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_volume_control_get_instance_private(self);
}

static void
gv_volume_control_class_init(GvVolumeControlClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_volume_control_finalize;
	object_class->constructed = gv_volume_control_constructed;
}
