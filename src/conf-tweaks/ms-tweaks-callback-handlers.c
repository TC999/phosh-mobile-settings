/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "mobile-settings-config.h"

#include "ms-tweaks-callback-handlers.h"
#include "ms-tweaks-gtk-utils.h"
#include "ms-tweaks-mappings.h"
#include "ms-tweaks-utils.h"

#include <glib/gi18n-lib.h>


static void
do_set_value (MsTweaksBackend *backend_state, GValue *value_to_set, AdwToastOverlay *toast_overlay)
{
  const MsTweaksSetting *setting_data = NULL;
  GError *error = NULL;
  gboolean success;

  setting_data = ms_tweaks_backend_get_setting_data (backend_state);
  ms_tweaks_mappings_handle_set (value_to_set, setting_data);
  success = ms_tweaks_backend_set_value (backend_state, value_to_set, &error);

  if (!success)
    ms_tweaks_callback_handlers_show_error_toast (toast_overlay, error->message);
}


MsTweaksCallbackMeta *
ms_tweaks_callback_meta_new (MsTweaksBackend *backend_state, AdwToastOverlay *toast_overlay)
{
  MsTweaksCallbackMeta *self = g_new (MsTweaksCallbackMeta, 1);

  self->backend_state = g_object_ref (backend_state);
  self->toast_overlay = g_object_ref (toast_overlay);

  return self;
}


void
ms_tweaks_callback_meta_free (MsTweaksCallbackMeta *self)
{
  g_object_unref (self->backend_state);
  g_object_unref (self->toast_overlay);

  g_free (self);
}


void
ms_tweaks_callback_handlers_show_error_toast (AdwToastOverlay *toast_overlay,
                                              const char      *error_message)
{
  AdwToast *toast = adw_toast_new_format (_("Something went wrong: %s"), error_message);

  adw_toast_overlay_add_toast (toast_overlay, toast);
}

/**
 * ms_tweaks_callback_handlers_type_boolean:
 * @switch_row: Instance of AdwSwitchRow to read the state of. Should be provided by
 *              g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::active" signal.
 * @callback_meta: Instance of MsTweaksCallbackMeta.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_BOOLEAN.
 */
void
ms_tweaks_callback_handlers_type_boolean (AdwSwitchRow         *switch_row,
                                          GParamSpec           *unused,
                                          MsTweaksCallbackMeta *callback_meta)
{
  const gboolean is_active = adw_switch_row_get_active (switch_row);
  GValue is_active_container = G_VALUE_INIT;

  g_value_init (&is_active_container, G_TYPE_BOOLEAN);
  g_value_set_boolean (&is_active_container, is_active);

  do_set_value (callback_meta->backend_state, &is_active_container, callback_meta->toast_overlay);
}

/**
 * ms_tweaks_callback_handlers_type_choice:
 * @combo_row: Instance of AdwComboRow to read the state of. Should be provided by
 *             g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::selected" signal.
 * @callback_meta: Instance of MsTweaksCallbackMeta.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_CHOICE.
 */
void
ms_tweaks_callback_handlers_type_choice (AdwComboRow          *combo_row,
                                         GParamSpec           *unused,
                                         MsTweaksCallbackMeta *callback_meta)
{
  GtkStringList *list = GTK_STRING_LIST (adw_combo_row_get_model (combo_row));
  MsTweaksBackend *backend_state = callback_meta->backend_state;
  const guint selected = adw_combo_row_get_selected (combo_row);
  const char *string_repr = gtk_string_list_get_string (list, selected);
  const MsTweaksSetting *setting_data = NULL;
  GValue value_container = G_VALUE_INIT;
  char *value = NULL;

  setting_data = ms_tweaks_backend_get_setting_data (backend_state);
  value = g_hash_table_lookup (setting_data->map, string_repr);

  if (!value) {
    ms_tweaks_critical (setting_data->name,
                        "Couldn't find data in map even when it definitely should have been there");
    return;
  }

  g_value_init (&value_container, G_TYPE_STRING);
  g_value_set_string (&value_container, value);

  do_set_value (backend_state, &value_container, callback_meta->toast_overlay);
}


/**
 * ms_tweaks_callback_handlers_type_color:
 * @widget: Instance of GtkColorDialogButton to read the state of. Should be provided by
 *          g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::rgba" signal.
 * @callback_meta: Instance of MsTweaksCallbackMeta.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_COLOR.
 */
void
ms_tweaks_callback_handlers_type_color (GtkColorDialogButton *widget,
                                        GParamSpec           *unused,
                                        MsTweaksCallbackMeta *callback_meta)
{
  const GdkRGBA *picked_colour = gtk_color_dialog_button_get_rgba (widget);
  GValue picked_colour_container = G_VALUE_INIT;

  g_value_init (&picked_colour_container, G_TYPE_STRING);
  g_value_set_string (&picked_colour_container,
                      ms_tweaks_util_gdkrgba_to_rgb_hex_string (picked_colour));

  do_set_value (callback_meta->backend_state,
                &picked_colour_container,
                callback_meta->toast_overlay);
}


void
ms_tweaks_callback_handlers_type_file (GObject *source_object, GAsyncResult *result, gpointer data)
{
  MsTweaksPreferencesPageFilePickerMeta *metadata = (MsTweaksPreferencesPageFilePickerMeta *) data;
  g_autoptr (GtkFileDialog) file_picker_dialog = NULL;
  GValue path_to_picked_file = G_VALUE_INIT;
  g_autofree char *picked_file_name = NULL;
  g_autoptr (GFile) picked_file = NULL;
  g_autoptr (GError) error = NULL;

  g_assert (metadata->backend_state);

  file_picker_dialog = GTK_FILE_DIALOG (source_object);
  picked_file = gtk_file_dialog_open_finish (file_picker_dialog, result, &error);

  if (!picked_file) {
    if (!g_error_matches (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
      ms_tweaks_callback_handlers_show_error_toast (metadata->toast_overlay, error->message);
    return;
  }

  g_value_init (&path_to_picked_file, G_TYPE_STRING);
  g_value_set_string (&path_to_picked_file, g_file_get_path (picked_file));

  do_set_value (metadata->backend_state, &path_to_picked_file, metadata->toast_overlay);

  picked_file_name = g_path_get_basename (g_value_get_string (&path_to_picked_file));
  gtk_label_set_text (GTK_LABEL (metadata->file_picker_label), picked_file_name);
}

/**
 * ms_tweaks_callback_handlers_type_font:
 * @widget: Instance of GtkFontDialogButton to read the state of. Should be provided by
 *          g_signal_connect (), see explanation for `unused`.
 * @unused: Property spec as provided by the callback from g_signal_connect () when connected to the
 *          "notify::font-desc" signal.
 * @callback_meta: Instance of MsTweaksCallbackMeta.
 *
 * Generic handler for MsTweaksBackend types that support MS_TWEAKS_TYPE_FONT.
 */
void
ms_tweaks_callback_handlers_type_font (GtkFontDialogButton  *widget,
                                       GParamSpec           *unused,
                                       MsTweaksCallbackMeta *callback_meta)
{
  PangoFontDescription *font_desc = gtk_font_dialog_button_get_font_desc (widget);
  GValue font_container = G_VALUE_INIT;

  g_value_init (&font_container, G_TYPE_STRING);
  g_value_set_string (&font_container, pango_font_description_to_string (font_desc));

  do_set_value (callback_meta->backend_state,
                &font_container,
                callback_meta->toast_overlay);
}


void
ms_tweaks_callback_handlers_type_number (AdwSpinRow *spin_row, MsTweaksCallbackMeta *callback_meta)
{
  const double spin_row_value = adw_spin_row_get_value (spin_row);
  GValue spin_row_value_container = G_VALUE_INIT;
  g_autofree char *spin_row_value_string = NULL;

  g_assert (MS_IS_TWEAKS_BACKEND (callback_meta->backend_state));

  spin_row_value_string = g_strdup_printf ("%f", spin_row_value);

  g_value_init (&spin_row_value_container, G_TYPE_STRING);
  g_value_set_string (&spin_row_value_container, spin_row_value_string);

  do_set_value (callback_meta->backend_state,
                &spin_row_value_container,
                callback_meta->toast_overlay);
}
