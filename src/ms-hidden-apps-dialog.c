/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-hidden-apps-dialog"

#include "mobile-settings-config.h"

#include "ms-hidden-app-row.h"
#include "ms-hidden-apps-dialog.h"

#include <glib/gstdio.h>

/**
 * MsHiddenAppsDialog:
 *
 * Dialog to unhide currently hidden and disabled applications
 */

enum {
  PROP_0,
  PROP_N_SELECTED,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


enum {
  DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


struct _MsHiddenAppsDialog {
  AdwDialog   parent;

  GtkStack   *stack;
  GtkListBox *hidden_apps_listbox;

  GListStore *hidden_apps;
  guint       n_selected;
};

G_DEFINE_TYPE (MsHiddenAppsDialog, ms_hidden_apps_dialog, ADW_TYPE_DIALOG)


static gboolean
hidden_app (const char *filename)
{
  g_autoptr (GKeyFile) keyfile = g_key_file_new ();
  g_autoptr (GError) err = NULL;
  g_autofree char *appsdir = NULL, *basename = NULL, *outfilename = NULL;
  gboolean success, inplace = FALSE;

  if (!g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, &err)) {
    g_warning ("Failed to load keyfile %s: %s", filename, err->message);
    return FALSE;
  }

  if (!g_key_file_has_key (keyfile, "Desktop Entry", "X-Phosh-Hidden", &err)) {
    g_warning ("Not a override file: %s: %s", filename, err->message);
    return FALSE;
  }

  inplace = g_key_file_get_integer (keyfile, "Desktop Entry", "X-Phosh-Hidden", &err);
  if (err) {
    g_warning ("Not a override file: %s: %s", filename, err->message);
    return FALSE;
  }

  /* Simple case: just remove the desktop file */
  if (!inplace) {
    if (g_unlink (filename) < 0) {
      g_warning ("Failed to unlink %s: %s", filename, g_strerror (errno));
      return FALSE;
    }
    return TRUE;
  }

  /* Best effort, we can't do much if the keys won't remove */
  g_key_file_remove_key (keyfile, "Desktop Entry", "X-Phosh-Hidden", NULL);
  g_key_file_remove_key (keyfile, "Desktop Entry", "Hidden", NULL);

  success = g_key_file_save_to_file (keyfile, filename, &err);
  if (!success) {
    g_warning ("Failed to hide %s: %s", outfilename, err->message);
    return FALSE;
  }

  return TRUE;
}


static void
on_select_button_clicked (MsHiddenAppsDialog *self)
{
  guint n_items = g_list_model_get_n_items (G_LIST_MODEL (self->hidden_apps));
  guint n_unhidden = 0;


  for (guint i = 0; i < n_items; i++) {
    MsHiddenAppRow *row = g_list_model_get_item (G_LIST_MODEL (self->hidden_apps), i);
    const char *filename = ms_hidden_app_row_get_filename (row);

    if (!ms_hidden_app_row_get_selected (row))
      continue;

    g_debug ("Unhiding '%s'", filename);
    hidden_app (filename);
    n_unhidden++;
  }

  g_debug ("%u apps unhidden", n_unhidden);
  g_signal_emit (self, signals[DONE], 0, n_unhidden);
}


static gboolean
n_selected_to_sensitive (gpointer unused, guint n_selected)
{
  return !!n_selected;
}


static gboolean
transform_n_items_to_stack_page_name (GBinding     *binding,
                                      const GValue *from_value,
                                      GValue       *to_value,
                                      gpointer      unused)
{
  guint n_items = g_value_get_uint (from_value);
  const char *name = n_items ? "has-hidden" : "no-hidden";

  g_value_set_string (to_value, name);
  return TRUE;
}


static int
compare_app_row (gconstpointer a, gconstpointer b, gpointer user_data)
{
  AdwPreferencesRow *rowA = (gpointer)a;
  AdwPreferencesRow *rowB = (gpointer)b;

  return g_strcmp0 (adw_preferences_row_get_title (rowA),
                    adw_preferences_row_get_title (rowB));
}



static void
ms_hidden_apps_dialog_load_hidden_apps (MsHiddenAppsDialog *self)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GDir) dir = NULL;
  g_auto (GStrv) enabled_plugins = NULL;
  g_autofree char *appdir = NULL;
  const char *datadir, *filename;

  datadir = g_get_user_data_dir ();
  g_assert (datadir);

  appdir = g_build_filename (datadir, "applications", NULL);
  dir = g_dir_open (appdir, 0, &err);
  if (dir == NULL) {
    g_warning ("Failed to read hidden apps %s: %s", appdir, err->message);
    return;
  }

  while ((filename = g_dir_read_name (dir))) {
    GtkWidget *row;
    g_autoptr (GError) error = NULL;
    g_autoptr (GKeyFile) keyfile = g_key_file_new ();
    g_autofree char *icon_name = NULL, *desktopfile = NULL, *name = NULL;

    if (!g_str_has_suffix (filename, ".desktop"))
      continue;

    desktopfile = g_build_filename (appdir, filename, NULL);
    if (g_key_file_load_from_file (keyfile, desktopfile, G_KEY_FILE_NONE, &error) == FALSE) {
      g_warning ("Failed to load desktop file '%s': %s", filename, error->message);
      continue;
    }

    if (!g_key_file_has_key (keyfile, "Desktop Entry", "X-Phosh-Hidden", &err)) {
      g_debug ("Not hidden by phosh: %s: %s", filename, err ? err->message : "");
      continue;
    }

    if (!g_key_file_has_key (keyfile, "Desktop Entry", "Hidden", &err)) {
      g_warning ("No Hidden key found: %s: %s", filename, err ? err->message : "");
      continue;
    }

    name = g_key_file_get_string (keyfile, "Desktop Entry", "Name", &err);
    if (!name) {
      g_warning ("No Name, desktop file unusable: %s: %s", filename, err ? err->message : "");
      continue;
    }

    icon_name = g_key_file_get_string (keyfile, "Desktop Entry", "Icon", NULL);
    if (!icon_name) {
      g_warning ("No icon found: %s: %s", filename, err ? err->message : "");
      /* We show it anyway */
    }

    row = g_object_new (MS_TYPE_HIDDEN_APP_ROW,
                        "filename", desktopfile,
                        "title", name,
                        "icon-name", icon_name,
                        NULL);

    g_list_store_insert_sorted (self->hidden_apps, row, compare_app_row, NULL);
  }
}


static void
ms_hidden_apps_dialog_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  MsHiddenAppsDialog *self = MS_HIDDEN_APPS_DIALOG (object);

  switch (property_id) {
  case PROP_N_SELECTED:
    g_value_set_uint (value, self->n_selected);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}



static void
ms_hidden_apps_dialog_dispose (GObject *object)
{
  MsHiddenAppsDialog *self = MS_HIDDEN_APPS_DIALOG (object);

  g_clear_object (&self->hidden_apps);

  G_OBJECT_CLASS (ms_hidden_apps_dialog_parent_class)->dispose (object);
}


static void
ms_hidden_apps_dialog_class_init (MsHiddenAppsDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_hidden_apps_dialog_get_property;
  object_class->dispose = ms_hidden_apps_dialog_dispose;

  props[PROP_N_SELECTED] =
    g_param_spec_uint ("n-selected", "", "",
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * MsHiddenAppsDialog::done:
   * @self: The dialog
   * @n_items: The number of apps that were made visible again
   *
   * Sent when the dialog is done and should be closed
   */
  signals[DONE] =
    g_signal_new ("done",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_UINT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/"
                                               "ms-hidden-apps-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, MsHiddenAppsDialog, stack);
  gtk_widget_class_bind_template_child (widget_class, MsHiddenAppsDialog, hidden_apps_listbox);

  gtk_widget_class_bind_template_callback (widget_class, n_selected_to_sensitive);
  gtk_widget_class_bind_template_callback (widget_class, on_select_button_clicked);

  gtk_widget_class_set_css_name (widget_class, "ms-hidden-apps-dialog");
}


static void
on_row_selected (MsHiddenAppsDialog *self, GParamSpec *pspec, MsHiddenAppRow *row)
{
  if (ms_hidden_app_row_get_selected (row))
    self->n_selected++;
  else
    self->n_selected--;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_SELECTED]);
}


static GtkWidget *
create_hidden_app_row (gpointer object, gpointer user_data)
{
  MsHiddenAppRow *row = MS_HIDDEN_APP_ROW (object);
  MsHiddenAppsDialog *self = MS_HIDDEN_APPS_DIALOG (user_data);

  g_signal_connect_object (row,
                           "notify::selected",
                           G_CALLBACK (on_row_selected),
                           self,
                           G_CONNECT_SWAPPED);

  return GTK_WIDGET (object);
}


static void
ms_hidden_apps_dialog_init (MsHiddenAppsDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->hidden_apps = g_list_store_new (G_TYPE_OBJECT);

  gtk_list_box_bind_model (self->hidden_apps_listbox,
                           G_LIST_MODEL (self->hidden_apps),
                           create_hidden_app_row,
                           self, NULL);

  g_object_bind_property_full (self->hidden_apps, "n-items",
                               self->stack, "visible-child-name",
                               G_BINDING_SYNC_CREATE,
                               transform_n_items_to_stack_page_name,
                               NULL, NULL, NULL);

  ms_hidden_apps_dialog_load_hidden_apps (self);
}


MsHiddenAppsDialog *
ms_hidden_apps_dialog_new (void)
{
  return g_object_new (MS_TYPE_HIDDEN_APPS_DIALOG, NULL);
}
