/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-lockscreen-panel"

#include "mobile-settings-config.h"
#include "ms-lockscreen-panel.h"
#include "ms-plugin-row.h"
#include "ms-util.h"

#include <libportal/portal.h>
#include <libportal-gtk4/portal-gtk4.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include <phosh-plugin.h>

/* Verbatim from phosh */
#define LOCKSCREEN_SCHEMA_ID "sm.puri.phosh.lockscreen"
#define LOCKSCREEN_KEY_SHUFFLE_KEYPAD "shuffle-keypad"
#define LOCKSCREEN_KEY_REQUIRE_UNLOCK "require-unlock"

#define SCREENSAVER_SCHEMA_ID "org.gnome.desktop.screensaver"
#define SCREENSAVER_KEY_LOCK_DELAY "lock-delay"
#define SCREENSAVER_KEY_PICTURE_OPTIONS "picture-options"

struct _MsLockscreenPanel {
  AdwBin           parent;

  GSettings       *settings;
  GSettings       *screensaver_settings;
  AdwSwitchRow    *shuffle_switch;
  AdwSwitchRow    *require_unlock_switch;
  AdwSwitchRow    *wallpaper_switch;
  GtkWidget       *lock_delay_adjustment;

  AdwToastOverlay *toast_overlay;
  AdwToast        *toast;
};

G_DEFINE_TYPE (MsLockscreenPanel, ms_lockscreen_panel, ADW_TYPE_BIN)


static gboolean
picture_mode_to_bool (GValue *out_value, GVariant *in_variant, gpointer user_data)
{
  const char *mode = g_variant_get_string (in_variant, NULL);
  gboolean active;

  active = g_strcmp0 (mode, "none");
  g_value_set_boolean (out_value, active);
  return TRUE;
}


static GVariant *
bool_to_picture_mode (const GValue *in_value, const GVariantType *out_type, gpointer data)
{
  gboolean active = g_value_get_boolean (in_value);
  const char *mode;

  mode = active ? "zoom" : "none";

  return g_variant_new_string (mode);
}


static void
on_set_wallpaper (GObject *source, GAsyncResult *result, gpointer user_data)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (user_data);
  XdpPortal *portal = XDP_PORTAL (source);
  g_autoptr(GError) err = NULL;

  if (!xdp_portal_set_wallpaper_finish (portal, result, &err)
      && !g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    AdwToast *toast;

    toast = adw_toast_new (_("Failed to set lockscreen wallpaper"));
    adw_toast_overlay_add_toast (self->toast_overlay, toast);
    g_warning ("Failed to set lockscreen wallpaper: %s", err->message);
    return;
  }

  g_debug ("Updated wallpaper via portal");
}


static void
on_file_chooser_done (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (user_data);
  GtkWindow *parent = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW));
  GtkFileDialog *filechooser = GTK_FILE_DIALOG (source_object);
  g_autofree char *uri = NULL;
  g_autoptr (XdpParent) xdp_parent = NULL;
  g_autoptr (XdpPortal) portal = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GError) err = NULL;

  file = gtk_file_dialog_open_finish (filechooser, res, &err);
  if (!file) {
    g_warning ("Failed to load background: %s", err->message);
     return;
  }

  uri = g_file_get_uri (file);
  portal = xdp_portal_new ();
  xdp_parent = xdp_parent_new_gtk (parent);
  xdp_portal_set_wallpaper (portal,
                            xdp_parent,
                            uri,
                            XDP_WALLPAPER_FLAG_LOCKSCREEN | XDP_WALLPAPER_FLAG_PREVIEW,
                            NULL,
                            on_set_wallpaper,
                            self);
}


static void
on_select_wallpaper_clicked (MsLockscreenPanel *self)
{
  GtkFileDialog *filechooser;
  GtkFileFilter *filter;
  GListStore *filters;
  g_autoptr (GFile) pictures_folder = NULL;
  GtkWindow *parent = GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW));

  filechooser = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (filechooser, _("Choose Wallpaper"));

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_pixbuf_formats (filter);

  filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (filters, filter);
  gtk_file_dialog_set_filters (filechooser, G_LIST_MODEL (filters));

  pictures_folder = g_file_new_for_path (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));
  gtk_file_dialog_set_initial_folder (filechooser, pictures_folder);

  gtk_file_dialog_open (filechooser, parent, NULL, on_file_chooser_done, self);
}


static gboolean
uint32_to_double_get_mapping (GValue *out_value, GVariant *in_variant, gpointer user_data)
{
  guint32 uint32_value = g_variant_get_uint32 (in_variant);

  g_value_set_double (out_value, (double) uint32_value);
  return TRUE;
}


static GVariant *
double_to_uint32_set_mapping (const GValue *in_value, const GVariantType *out_type, gpointer data)
{
  double dbl_value = g_value_get_double (in_value);
  guint32 int32_value = (guint32) CLAMP (dbl_value, 0.0, (double) G_MAXUINT32);

  return g_variant_new_uint32 (int32_value);
}


static void
ms_lockscreen_panel_finalize (GObject *object)
{
  MsLockscreenPanel *self = MS_LOCKSCREEN_PANEL (object);

  g_clear_object (&self->settings);
  g_clear_object (&self->screensaver_settings);

  G_OBJECT_CLASS (ms_lockscreen_panel_parent_class)->finalize (object);
}


static void
ms_lockscreen_panel_class_init (MsLockscreenPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_lockscreen_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-lockscreen-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, lock_delay_adjustment);
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, require_unlock_switch);
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, shuffle_switch);
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, toast_overlay);
  gtk_widget_class_bind_template_child (widget_class, MsLockscreenPanel, wallpaper_switch);

  gtk_widget_class_bind_template_callback (widget_class, on_select_wallpaper_clicked);
}


static void
ms_lockscreen_panel_init (MsLockscreenPanel *self)
{
  gboolean found;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (LOCKSCREEN_SCHEMA_ID);
  g_settings_bind (self->settings, LOCKSCREEN_KEY_SHUFFLE_KEYPAD,
                   self->shuffle_switch, "active", G_SETTINGS_BIND_DEFAULT);
  found = ms_schema_bind_property (LOCKSCREEN_SCHEMA_ID, LOCKSCREEN_KEY_REQUIRE_UNLOCK,
                                   G_OBJECT (self->require_unlock_switch), "active",
                                   G_SETTINGS_BIND_DEFAULT);
  gtk_widget_set_visible (GTK_WIDGET (self->require_unlock_switch), found);

  self->screensaver_settings = g_settings_new (SCREENSAVER_SCHEMA_ID);
  g_settings_bind_with_mapping (self->screensaver_settings,
                                SCREENSAVER_KEY_LOCK_DELAY,
                                self->lock_delay_adjustment,
                                "value",
                                G_SETTINGS_BIND_DEFAULT,
                                uint32_to_double_get_mapping,
                                double_to_uint32_set_mapping,
                                NULL, NULL);

  g_settings_bind_with_mapping (self->screensaver_settings,
                                SCREENSAVER_KEY_PICTURE_OPTIONS,
                                self->wallpaper_switch,
                                "active",
                                G_SETTINGS_BIND_DEFAULT,
                                picture_mode_to_bool,
                                bool_to_picture_mode,
                                NULL, NULL);
}


MsLockscreenPanel *
ms_lockscreen_panel_new (void)
{
  return MS_LOCKSCREEN_PANEL (g_object_new (MS_TYPE_LOCKSCREEN_PANEL, NULL));
}
