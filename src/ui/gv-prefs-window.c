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
#include <glib-object.h>
#include <gtk/gtk.h>

#include "framework/gtk-additions.h"
#include "framework/glib-object-additions.h"
#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "feat/gv-feat.h"
#include "ui/gv-ui-enum-types.h"
#include "ui/gv-ui-helpers.h"
#include "ui/gv-ui-internal.h"

#include "ui/gv-prefs-window.h"

#define UI_FILE "prefs-window.glade"

/*
 * GObject definitions
 */

struct _GvPrefsWindowPrivate {
	/*
	 * Features
	 */

	/* Controls */
	GvFeature *hotkeys_feat;
	GvFeature *dbus_native_feat;
	GvFeature *dbus_mpris2_feat;
	/* Display */
	GvFeature *notifications_feat;
	GvFeature *console_output_feat;
	/* Player */
	GvFeature *inhibitor_feat;

	/*
	 * Widgets
	 */

	/* Top-level */
	GtkWidget *window_vbox;
	/* Misc */
	GtkWidget *misc_vbox;
	GtkWidget *playback_frame;
	GtkWidget *playback_grid;
	GtkWidget *autoplay_check;
	GtkWidget *pipeline_check;
	GtkWidget *pipeline_entry;
	GtkWidget *pipeline_apply_button;
	GtkWidget *system_frame;
	GtkWidget *system_grid;
	GtkWidget *inhibitor_label;
	GtkWidget *inhibitor_switch;
	GtkWidget *dbus_frame;
	GtkWidget *dbus_grid;
	GtkWidget *dbus_native_label;
	GtkWidget *dbus_native_switch;
	GtkWidget *dbus_mpris2_label;
	GtkWidget *dbus_mpris2_switch;
	/* Display */
	GtkWidget *display_vbox;
	GtkWidget *window_frame;
	GtkWidget *window_grid;
	GtkWidget *window_theme_combo;
	GtkWidget *window_autosize_check;
	GtkWidget *notif_frame;
	GtkWidget *notif_grid;
	GtkWidget *notif_enable_label;
	GtkWidget *notif_enable_switch;
	GtkWidget *console_frame;
	GtkWidget *console_grid;
	GtkWidget *console_output_label;
	GtkWidget *console_output_switch;
	/* Controls */
	GtkWidget *controls_vbox;
	GtkWidget *keyboard_frame;
	GtkWidget *keyboard_grid;
	GtkWidget *hotkeys_label;
	GtkWidget *hotkeys_switch;
	GtkWidget *mouse_frame;
	GtkWidget *mouse_grid;
	GtkWidget *middle_click_action_label;
	GtkWidget *middle_click_action_combo;
	GtkWidget *scroll_action_label;
	GtkWidget *scroll_action_combo;
	/* Buttons */
	GtkWidget *close_button;
};

typedef struct _GvPrefsWindowPrivate GvPrefsWindowPrivate;

struct _GvPrefsWindow {
	/* Parent instance structure */
	GtkWindow             parent_instance;
	/* Private data */
	GvPrefsWindowPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvPrefsWindow, gv_prefs_window, GTK_TYPE_WINDOW)

/*
 * Gtk signal handlers
 */

static void
on_pipeline_apply_button_clicked(GtkButton *button G_GNUC_UNUSED,
                                 GvPrefsWindow *self)
{
	GvPrefsWindowPrivate *priv = self->priv;
	GtkEntry *pipeline_entry = GTK_ENTRY(priv->pipeline_entry);
	const gchar *text;

	text = gtk_entry_get_text(pipeline_entry);
	if (text) {
		/* Strip leading and trailing whitespaces */
		gchar *stripped;

		stripped = g_strstrip(g_strdup(text));
		gtk_entry_set_text(pipeline_entry, stripped);
		g_free(stripped);

		text = gtk_entry_get_text(pipeline_entry);
	}

	if (text && strlen(text) > 0) {
		gv_player_set_pipeline_string(gv_core_player, text);
	} else {
		gv_player_set_pipeline_string(gv_core_player, NULL);
	}
}

static void
on_close_button_clicked(GtkButton *button G_GNUC_UNUSED, GvPrefsWindow *self)
{
	GtkWindow *window = GTK_WINDOW(self);

	gtk_window_close(window);
}

static gboolean
on_window_key_press_event(GvPrefsWindow *self, GdkEventKey *event, gpointer data G_GNUC_UNUSED)
{
	GtkWindow *window = GTK_WINDOW(self);

	g_assert(event->type == GDK_KEY_PRESS);

	if (event->keyval == GDK_KEY_Escape)
		gtk_window_close(window);

	return GDK_EVENT_PROPAGATE;
}

/*
 * GBinding transform functions
 */

static gboolean
transform_pipeline_string_to(GBinding *binding G_GNUC_UNUSED,
                             const GValue *from_value,
                             GValue *to_value,
                             gpointer user_data G_GNUC_UNUSED)
{
	const gchar *str;

	/* GtkEntry complains if we set the 'text' property to NULL...
	 * [Gtk] gtk_entry_set_text: assertion 'text != NULL' failed
	 */

	str = g_value_get_string(from_value);

	if (str == NULL)
		g_value_set_static_string(to_value, "");
	else
		g_value_set_string(to_value, str);

	return TRUE;
}

/*
 * Construct helpers
 */

static void
setup_notebook_page_appearance(GtkWidget *vbox)
{
	g_return_if_fail(GTK_IS_BOX(vbox));

	g_object_set(vbox,
	             "margin", GV_UI_WINDOW_BORDER,
	             "spacing", GV_UI_GROUP_SPACING,
	             NULL);
}

static void
setup_section_appearance(GtkWidget *frame, GtkWidget *grid)
{
	static PangoAttrList *bold_attr_list;
	GtkWidget *label;

	g_return_if_fail(GTK_IS_FRAME(frame));
	g_return_if_fail(GTK_IS_GRID(grid));

	if (bold_attr_list == NULL) {
		bold_attr_list = pango_attr_list_new();
		pango_attr_list_insert(bold_attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	}

	label = gtk_frame_get_label_widget(GTK_FRAME(frame));
	if (label) {
		gtk_label_set_attributes(GTK_LABEL(label), bold_attr_list);
		gtk_widget_set_margin_bottom(label, GV_UI_ELEM_SPACING);
	}

	g_object_set(grid,
	             "row-spacing", GV_UI_ELEM_SPACING,
	             "column-spacing", GV_UI_COLUMN_SPACING,
	             NULL);
}

static void
setdown_widget(const gchar *tooltip_text, GtkWidget *widget)
{
	if (tooltip_text)
		gtk_widget_set_tooltip_text(widget, tooltip_text);

	gtk_widget_set_sensitive(widget, FALSE);
}

static void
setup_setting(const gchar *tooltip_text,
              GtkWidget *label, GtkWidget *widget, const gchar *widget_prop,
              GObject *obj, const gchar *obj_prop,
              GBindingTransformFunc transform_to,
              GBindingTransformFunc transform_from)
{
	/* Tooltip */
	if (tooltip_text) {
		if (widget)
			gtk_widget_set_tooltip_text(widget, tooltip_text);
		if (label)
			gtk_widget_set_tooltip_text(label, tooltip_text);
	}

	/* Binding: obj 'prop' <-> widget 'prop'
	 * Order matters, don't mix up source and target here...
	 */
	if (!widget || !obj)
		return;

	g_object_bind_property_full(obj, obj_prop, widget, widget_prop,
	                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
	                            transform_to, transform_from, NULL, NULL);
}

static void
setup_feature(const gchar *tooltip_text, GtkWidget *label, GtkWidget *sw, GvFeature *feat)
{
	/* If feat is NULL, it's because it's been disabled at compile time */
	if (feat == NULL) {
		tooltip_text = _("Feature disabled at compile-time.");

		gtk_widget_set_tooltip_text(sw, tooltip_text);
		gtk_widget_set_tooltip_text(label, tooltip_text);
		gtk_widget_set_sensitive(label, FALSE);
		gtk_widget_set_sensitive(sw, FALSE);

		return;
	}

	/* Tooltip */
	if (tooltip_text) {
		gtk_widget_set_tooltip_text(sw, tooltip_text);
		gtk_widget_set_tooltip_text(label, tooltip_text);
	}

	/* Binding: feature 'enabled' <-> switch 'active'
	 * Order matters, don't mix up source and target here...
	 */
	g_object_bind_property(feat, "enabled", sw, "active",
	                       G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

/*
 * Public methods
 */

GtkWidget *
gv_prefs_window_new(void)
{
	return g_object_new(GV_TYPE_PREFS_WINDOW, NULL);
}

/*
 * Construct helpers
 */

static void
gv_prefs_window_populate_features(GvPrefsWindow *self)
{
	GvPrefsWindowPrivate *priv = self->priv;

	/* Controls */
	priv->hotkeys_feat        = gv_feat_find("Hotkeys");
	priv->dbus_native_feat    = gv_feat_find("DBusServerNative");
	priv->dbus_mpris2_feat    = gv_feat_find("DBusServerMpris2");

	/* Display */
	priv->notifications_feat  = gv_feat_find("Notifications");
	priv->console_output_feat = gv_feat_find("ConsoleOutput");

	/* Player */
	priv->inhibitor_feat      = gv_feat_find("Inhibitor");
}

static void
gv_prefs_window_populate_widgets(GvPrefsWindow *self)
{
	GvPrefsWindowPrivate *priv = self->priv;
	GtkBuilder *builder;
	gchar *uifile;

	/* Build the ui */
	gv_builder_load(UI_FILE, &builder, &uifile);
	DEBUG("Built from ui file '%s'", uifile);

	/* Save widget pointers */

	/* Top-level */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_vbox);

	/* Misc */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, misc_vbox);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, playback_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, playback_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, autoplay_check);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, pipeline_check);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, pipeline_entry);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, pipeline_apply_button);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, system_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, system_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, inhibitor_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, inhibitor_switch);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, dbus_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, dbus_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, dbus_native_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, dbus_native_switch);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, dbus_mpris2_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, dbus_mpris2_switch);

	/* Display */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, display_vbox);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_theme_combo);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, window_autosize_check);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, notif_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, notif_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, notif_enable_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, notif_enable_switch);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, console_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, console_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, console_output_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, console_output_switch);

	/* Controls */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, controls_vbox);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, keyboard_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, keyboard_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, hotkeys_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, hotkeys_switch);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, mouse_frame);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, mouse_grid);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, middle_click_action_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, middle_click_action_combo);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, scroll_action_label);
	GTK_BUILDER_SAVE_WIDGET(builder, priv, scroll_action_combo);

	/* Action area */
	GTK_BUILDER_SAVE_WIDGET(builder, priv, close_button);

	/* Pack that within the window */
	gtk_container_add(GTK_CONTAINER(self), priv->window_vbox);

	/* Cleanup */
	g_object_unref(builder);
	g_free(uifile);
}

static void
gv_prefs_window_setup_widgets(GvPrefsWindow *self)
{
	GvPrefsWindowPrivate *priv = self->priv;
	GObject *main_window_mgr_obj = G_OBJECT(gv_ui_main_window_manager);
	GObject *main_window_obj     = G_OBJECT(gv_ui_main_window);
	GObject *status_icon_obj     = G_OBJECT(gv_ui_status_icon);
	GObject *player_obj          = G_OBJECT(gv_core_player);

	/*
	 * Setup settings and features.
	 * These function calls create a binding between a gtk widget and
	 * an internal object, initializes the widget value, and set the
	 * widgets tooltips (label + setting).
	 */

	/* Misc */
	setup_setting(_("Whether to start playback automatically on startup."),
	              NULL,
	              priv->autoplay_check, "active",
	              player_obj, "autoplay",
	              NULL, NULL);

	setup_setting(_("Whether to use a custom output pipeline."),
	              NULL,
	              priv->pipeline_check, "active",
	              player_obj, "pipeline-enabled",
	              NULL, NULL);

	setup_setting(_("The GStreamer output pipeline used for playback. Refer to the"
	                "online documentation for examples."),
	              NULL,
	              priv->pipeline_entry, NULL,
	              NULL, NULL,
	              NULL, NULL);

	g_object_bind_property_full(player_obj, "pipeline-string",
	                            priv->pipeline_entry, "text",
	                            G_BINDING_SYNC_CREATE,
	                            transform_pipeline_string_to,
	                            NULL, NULL, NULL);

	g_signal_connect_object(priv->pipeline_apply_button, "clicked",
	                        G_CALLBACK(on_pipeline_apply_button_clicked), self, 0);

	setup_feature(_("Prevent the system from going to sleep while playing."),
	              priv->inhibitor_label,
	              priv->inhibitor_switch,
	              priv->inhibitor_feat);

	setup_feature(_("Enable the native D-Bus server "
	                "(needed for the command-line interface)."),
	              priv->dbus_native_label,
	              priv->dbus_native_switch,
	              priv->dbus_native_feat);

	setup_feature(_("Enable the MPRIS2 D-Bus server."),
	              priv->dbus_mpris2_label,
	              priv->dbus_mpris2_switch,
	              priv->dbus_mpris2_feat);

	/* Display */
	setup_setting(_("Prefer a different variant of the theme (if available)."),
	              NULL,
	              priv->window_theme_combo, "active-id",
	              main_window_obj, "theme-variant",
	              NULL, NULL);

	if (!status_icon_obj) {
		setup_setting(_("Automatically adjust the window height when a station "
		                "is added or removed."),
		              NULL,
		              priv->window_autosize_check, "active",
		              main_window_mgr_obj, "autoset-height",
		              NULL, NULL);
	} else {
		setdown_widget(_("Setting not available in status icon mode."),
		               priv->window_frame);
	}

	setup_feature(_("Show notification when the status changes."),
	              priv->notif_enable_label,
	              priv->notif_enable_switch,
	              priv->notifications_feat);

	setup_feature(_("Display information on the standard output."),
	              priv->console_output_label,
	              priv->console_output_switch,
	              priv->console_output_feat);

	/* Controls */
	setup_feature(_("Bind mutimedia keys (play/pause/stop/previous/next)."),
	              priv->hotkeys_label,
	              priv->hotkeys_switch,
	              priv->hotkeys_feat);

	if (status_icon_obj) {
		setup_setting(_("Action triggered by a middle click on the status icon."),
		              priv->middle_click_action_label,
		              priv->middle_click_action_combo, "active-id",
		              status_icon_obj, "middle-click-action",
		              NULL, NULL);

		setup_setting(_("Action triggered by mouse-scrolling on the status icon."),
		              priv->scroll_action_label,
		              priv->scroll_action_combo, "active-id",
		              status_icon_obj, "scroll-action",
		              NULL, NULL);
	} else {
		setdown_widget(_("Setting only available in status icon mode."),
		               priv->mouse_frame);
	}

	/*
	 * Some widgets have conditional sensitivity.
	 */

	g_object_bind_property(priv->pipeline_check, "active",
	                       priv->pipeline_entry, "sensitive",
	                       G_BINDING_SYNC_CREATE);
	g_object_bind_property(priv->pipeline_check, "active",
	                       priv->pipeline_apply_button, "sensitive",
	                       G_BINDING_SYNC_CREATE);
}

static void
gv_prefs_window_setup_appearance(GvPrefsWindow *self)
{
	GvPrefsWindowPrivate *priv = self->priv;

	/* Window */
	g_object_set(priv->window_vbox,
	             "margin", 0,
	             "spacing", 0,
	             NULL);

	/* Misc */
	setup_notebook_page_appearance(priv->misc_vbox);
	setup_section_appearance(priv->playback_frame, priv->playback_grid);
	setup_section_appearance(priv->system_frame, priv->system_grid);
	setup_section_appearance(priv->dbus_frame, priv->dbus_grid);

	/* Display */
	setup_notebook_page_appearance(priv->display_vbox);
	setup_section_appearance(priv->window_frame, priv->window_grid);
	setup_section_appearance(priv->notif_frame, priv->notif_grid);
	setup_section_appearance(priv->console_frame, priv->console_grid);

	/* Controls */
	setup_notebook_page_appearance(priv->controls_vbox);
	setup_section_appearance(priv->keyboard_frame, priv->keyboard_grid);
	setup_section_appearance(priv->mouse_frame, priv->mouse_grid);
}

/*
 * GObject methods
 */

static void
gv_prefs_window_finalize(GObject *object)
{
	TRACE("%p", object);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_prefs_window, object);
}

static void
gv_prefs_window_constructed(GObject *object)
{
	GvPrefsWindow *self = GV_PREFS_WINDOW(object);
	GvPrefsWindowPrivate *priv = self->priv;

	/* Build the window */
	gv_prefs_window_populate_features(self);
	gv_prefs_window_populate_widgets(self);
	gv_prefs_window_setup_widgets(self);
	gv_prefs_window_setup_appearance(self);

	/* Connect signal handlers */
	g_signal_connect_object(priv->close_button, "clicked",
	                        G_CALLBACK(on_close_button_clicked), self, 0);
	g_signal_connect_object(self, "key_press_event",
	                        G_CALLBACK(on_window_key_press_event), NULL, 0);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_prefs_window, object);
}

static void
gv_prefs_window_init(GvPrefsWindow *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_prefs_window_get_instance_private(self);
}

static void
gv_prefs_window_class_init(GvPrefsWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_prefs_window_finalize;
	object_class->constructed = gv_prefs_window_constructed;
}

/*
 * Convenience functions
 */

static GtkWidget *
make_prefs_window(GtkWindow *parent)
{
	GtkWidget *window;

	window = gv_prefs_window_new();
	gtk_window_set_transient_for(GTK_WINDOW(window), parent);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
	gtk_window_set_title(GTK_WINDOW(window), _("Preferences"));

	return window;
}

void
gv_show_prefs_window(GtkWindow *parent)
{
	static GtkWidget *prefs;

	/* Create if needed */
	if (prefs == NULL) {
		prefs = make_prefs_window(parent);
		g_object_add_weak_pointer(G_OBJECT(prefs), (gpointer *) &prefs);
	}

	/* Present */
	gtk_window_present(GTK_WINDOW(prefs));
}
