/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-preferences-page"

#include "mobile-settings-config.h"

#include "ms-tweaks-preferences-page.h"

#include "backends/ms-tweaks-backend-gsettings.h"
#include "backends/ms-tweaks-backend-gtk3settings.h"
#include "backends/ms-tweaks-backend-symlink.h"
#include "backends/ms-tweaks-backend-sysfs.h"
#include "backends/ms-tweaks-backend-xresources.h"
#include "ms-tweaks-backend-interface.h"
#include "ms-tweaks-callback-handlers.h"
#include "ms-tweaks-mappings.h"
#include "ms-tweaks-utils.h"

#include <glib/gi18n-lib.h>

#define PRIVILEGE_ESCALATION_PROGRAM "/usr/bin/pkexec"
#define PKEXEC_CANCELLED_EXIT_CODE 126


enum {
  PROP_0,
  PROP_DATA,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MsTweaksPreferencesPage {
  MsPanel parent_instance;

  GtkWidget *page;
  AdwBanner *banner;
  GtkWidget *toast_overlay;

  GPtrArray *commands_to_run_as_administrator; /* Value type: GStrv. */

  const MsTweaksPage *data;
};


static void
set_title_and_subtitle (GtkWidget *widget, const MsTweaksSetting *setting_data)
{
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (widget), setting_data->name_i18n);
  adw_action_row_set_subtitle (ADW_ACTION_ROW (widget), setting_data->help_i18n ?: "");
}


static GtkStringList *
get_keys_from_hashtable (GHashTable *hashtable)
{
  g_autoptr (GPtrArray) hash_table_keys_view = g_hash_table_get_keys_as_ptr_array (hashtable);

  /* We need to NULL-terminate the array to use the gtk_string_list_new () constructor. */
  g_ptr_array_add (hash_table_keys_view, NULL);

  return gtk_string_list_new ((const char **) hash_table_keys_view->pdata);
}


static void
destroy_callback_meta (gpointer data, GClosure *closure)
{
  ms_tweaks_callback_meta_free (data);
}


static GtkWidget *
setting_data_to_boolean_widget (MsTweaksBackend       *backend,
                                AdwToastOverlay       *toast_overlay,
                                const MsTweaksSetting *setting_data,
                                GValue                *widget_value)
{
  MsTweaksCallbackMeta *callback_meta = ms_tweaks_callback_meta_new (backend, toast_overlay);
  GtkWidget *switch_row = adw_switch_row_new ();

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend));

  set_title_and_subtitle (switch_row, setting_data);

  if (widget_value)
    adw_switch_row_set_active (ADW_SWITCH_ROW (switch_row), g_value_get_boolean (widget_value));

  g_signal_connect_data (switch_row,
                         "notify::active",
                         G_CALLBACK (ms_tweaks_callback_handlers_type_boolean),
                         callback_meta,
                         destroy_callback_meta,
                         G_CONNECT_DEFAULT);

  return switch_row;
}


static GtkWidget *
setting_data_to_choice_widget (MsTweaksBackend       *backend,
                               AdwToastOverlay       *toast_overlay,
                               const MsTweaksSetting *setting_data,
                               GValue                *widget_value)
{
  GtkWidget *combo_row = adw_combo_row_new ();
  MsTweaksCallbackMeta *callback_meta;
  GtkStringList *choice_model = NULL;

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend));

  if (setting_data->map) {
    choice_model = get_keys_from_hashtable (setting_data->map);
    adw_combo_row_set_model (ADW_COMBO_ROW (combo_row), G_LIST_MODEL (choice_model));

    if (widget_value) {
      gpointer key = NULL, value = NULL;
      gboolean found = FALSE;
      GHashTableIter iter;
      guint at_index = G_MAXUINT;

      /* Iterate over choices to find the index of the one we want to set as default. */
      g_hash_table_iter_init (&iter, setting_data->map);
      while (g_hash_table_iter_next (&iter, &key, &value)) {
        at_index = G_MAXUINT;

        if (g_str_equal (value, g_value_get_string (widget_value)))
          at_index = gtk_string_list_find (choice_model, key);

        found = at_index != G_MAXUINT;

        if (found) {
          adw_combo_row_set_selected (ADW_COMBO_ROW (combo_row), at_index);
          break;
        }
      }
    }
  } else {
    ms_tweaks_warning (setting_data->name,
                       "Choice widget with NULL map — either the datasource failed or the markup is wrong");
    return NULL;
  }

  set_title_and_subtitle (combo_row, setting_data);

  callback_meta = ms_tweaks_callback_meta_new (backend, toast_overlay);

  g_signal_connect_data (combo_row,
                         "notify::selected",
                         G_CALLBACK (ms_tweaks_callback_handlers_type_choice),
                         callback_meta,
                         destroy_callback_meta,
                         G_CONNECT_DEFAULT);

  return combo_row;
}


static GtkWidget *
setting_data_to_color_widget (MsTweaksBackend       *backend,
                              AdwToastOverlay       *toast_overlay,
                              const MsTweaksSetting *setting_data,
                              GValue                *widget_value)
{
  GdkRGBA widget_colour;
  GtkWidget *action_row = adw_action_row_new ();
  GtkColorDialog *color_dialog = gtk_color_dialog_new ();
  const char *colour_from_backend = g_value_get_string (widget_value);
  GtkWidget *color_dialog_button = gtk_color_dialog_button_new (color_dialog);
  MsTweaksCallbackMeta *callback_meta = ms_tweaks_callback_meta_new (backend, toast_overlay);

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend));

  set_title_and_subtitle (action_row, setting_data);
  adw_action_row_add_suffix (ADW_ACTION_ROW (action_row), color_dialog_button);

  gdk_rgba_parse (&widget_colour, colour_from_backend);
  gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON (color_dialog_button),
                                    &widget_colour);

  /* Set up listener for listening to colours picked by the widget and setting them in the
   * backend. */
  g_signal_connect_data (color_dialog_button,
                         "notify::rgba",
                         G_CALLBACK (ms_tweaks_callback_handlers_type_color),
                         callback_meta,
                         destroy_callback_meta,
                         G_CONNECT_DEFAULT);

  return action_row;
}


static void
file_widget_open_file_picker (GtkButton *widget, MsTweaksPreferencesPageFilePickerMeta *metadata)
{
  GtkFileDialog *file_picker_dialog = gtk_file_dialog_new ();

  gtk_file_dialog_open (file_picker_dialog,
                        NULL,
                        NULL,
                        ms_tweaks_callback_handlers_type_file,
                        metadata);
}


static const char *none_selected_label = "(None selected)";


static void
file_widget_unset (GtkButton *widget, MsTweaksPreferencesPageFilePickerMeta *metadata)
{
  GError *error = NULL;
  gboolean success;

  g_assert (metadata);
  g_assert (metadata->backend_state);
  g_assert (GTK_IS_LABEL (metadata->file_picker_label));
  g_assert (MS_IS_TWEAKS_BACKEND (metadata->backend_state));

  success = ms_tweaks_backend_set_value (metadata->backend_state, NULL, &error);

  if (success)
    gtk_label_set_label (GTK_LABEL (metadata->file_picker_label), none_selected_label);
  else
    ms_tweaks_callback_handlers_show_error_toast (metadata->toast_overlay, error->message);
}


static GtkWidget *
setting_data_to_file_widget (const MsTweaksSetting                 *setting_data,
                             MsTweaksBackend                       *backend_state,
                             const GValue                          *widget_value,
                             AdwToastOverlay                       *toast_overlay,
                             MsTweaksPreferencesPageFilePickerMeta *metadata)
{
  GtkWidget *reset_selection_button = gtk_button_new ();
  GtkWidget *file_picker_row = adw_action_row_new ();
  GtkWidget *file_picker_button = gtk_button_new ();
  GtkWidget *file_picker_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *file_picker_icon = gtk_image_new_from_icon_name ("folder-open-symbolic");

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend_state));

  metadata->backend_state = backend_state;
  metadata->file_picker_label = gtk_label_new (none_selected_label);
  metadata->toast_overlay = toast_overlay;

  set_title_and_subtitle (file_picker_row, setting_data);

  adw_action_row_add_suffix (ADW_ACTION_ROW (file_picker_row), reset_selection_button);
  adw_action_row_add_suffix (ADW_ACTION_ROW (file_picker_row), file_picker_button);

  gtk_box_append (GTK_BOX (file_picker_box), metadata->file_picker_label);
  gtk_box_append (GTK_BOX (file_picker_box), file_picker_icon);
  gtk_button_set_child (GTK_BUTTON (file_picker_button), file_picker_box);
  gtk_button_set_icon_name (GTK_BUTTON (reset_selection_button), "document-revert-symbolic");

  if (widget_value) {
    const char *file_path = g_value_get_string (widget_value);
    g_autofree char *filename = NULL;

    /* The GValue received from _get () should never contain NULL. */
    g_assert (file_path);

    filename = g_path_get_basename (file_path);

    gtk_label_set_text (GTK_LABEL (metadata->file_picker_label), filename);
  }

  g_signal_connect (file_picker_button,
                    "clicked",
                    G_CALLBACK (file_widget_open_file_picker),
                    metadata);
  g_signal_connect (reset_selection_button,
                    "clicked",
                    G_CALLBACK (file_widget_unset),
                    metadata);

  return file_picker_row;
}


static GtkWidget *
setting_data_to_font_widget (MsTweaksBackend       *backend,
                             AdwToastOverlay       *toast_overlay,
                             const MsTweaksSetting *setting_data,
                             const GValue          *widget_value)
{
  GtkWidget *action_row = adw_action_row_new ();
  GtkFontDialog *font_dialog = gtk_font_dialog_new ();
  GtkWidget *font_dialog_button = gtk_font_dialog_button_new (font_dialog);
  MsTweaksCallbackMeta *callback_meta = ms_tweaks_callback_meta_new (backend, toast_overlay);

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend));

  set_title_and_subtitle (action_row, setting_data);
  gtk_widget_set_valign (font_dialog_button, GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (ADW_ACTION_ROW (action_row), font_dialog_button);

  if (widget_value) {
    const char *font_name = g_value_get_string (widget_value);
    g_autoptr (PangoFontDescription) font_desc = pango_font_description_from_string (font_name);

    gtk_font_dialog_button_set_font_desc (GTK_FONT_DIALOG_BUTTON (font_dialog_button), font_desc);
  }

  g_signal_connect_data (font_dialog_button,
                         "notify::font-desc",
                         G_CALLBACK (ms_tweaks_callback_handlers_type_font),
                         callback_meta,
                         destroy_callback_meta,
                         G_CONNECT_DEFAULT);

  return action_row;
}


static GtkWidget *
setting_data_to_info_widget (const MsTweaksSetting *setting_data, const GValue *widget_value)
{
  g_assert (setting_data);

  if (widget_value) {
    GtkWidget *action_row = adw_action_row_new ();

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (action_row), setting_data->name_i18n);
    adw_action_row_set_subtitle (ADW_ACTION_ROW (action_row), g_value_get_string (widget_value));

    adw_action_row_set_subtitle_selectable (ADW_ACTION_ROW (action_row), true);
    gtk_widget_add_css_class (action_row, "property");

    return action_row;
  } else {
    ms_tweaks_warning (setting_data->name,
                       "widget_value was NULL in setting_data_to_boolean_widget ()");
    return NULL;
  }
}


static GtkWidget *
setting_data_to_number_widget (MsTweaksBackend       *backend,
                               AdwToastOverlay       *toast_overlay,
                               const MsTweaksSetting *setting_data,
                               const GValue          *widget_value)
{
  MsTweaksCallbackMeta *callback_meta;
  GtkWidget *spin_row;

  g_assert (setting_data);
  g_assert (MS_IS_TWEAKS_BACKEND (backend));

  if (G_APPROX_VALUE (setting_data->step, 0, DBL_EPSILON)) {
    ms_tweaks_warning (setting_data->name,
                       "step was %f in number widget, too close to 0",
                       setting_data->step);

    return NULL;
  }

  spin_row = adw_spin_row_new_with_range (setting_data->min, setting_data->max, setting_data->step);

  set_title_and_subtitle (spin_row, setting_data);

  if (widget_value)
    adw_spin_row_set_value (ADW_SPIN_ROW (spin_row), g_value_get_double (widget_value));

  callback_meta = ms_tweaks_callback_meta_new (backend, toast_overlay);

  g_signal_connect_data (spin_row,
                         "changed",
                         G_CALLBACK (ms_tweaks_callback_handlers_type_number),
                         callback_meta,
                         destroy_callback_meta,
                         G_CONNECT_DEFAULT);

  return spin_row;
}


typedef struct {
  MsTweaksPreferencesPage *preferences_page;
  GArray                  *cmds_to_remove;
  GPtrArray               *error_array;
  guint                    running_cmds;
} ChildExitCbShared;


static ChildExitCbShared *
child_exit_cb_shared_new (MsTweaksPreferencesPage *preferences_page)
{
  ChildExitCbShared *child_exit_cb_shared = g_new0 (ChildExitCbShared, 1);

  child_exit_cb_shared->cmds_to_remove = g_array_new (FALSE, FALSE, sizeof (int));
  child_exit_cb_shared->error_array = g_ptr_array_new_full (0, (GDestroyNotify) g_error_free);

  child_exit_cb_shared->preferences_page = g_object_ref (preferences_page);
  child_exit_cb_shared->running_cmds = preferences_page->commands_to_run_as_administrator->len;

  return child_exit_cb_shared;
}


static void
child_exit_cb_shared_free (ChildExitCbShared *self)
{
  g_object_unref (self->preferences_page);
  g_array_unref (self->cmds_to_remove);
  g_ptr_array_unref (self->error_array);

  g_free (self);
}

/**
 * ChildExitCbState:
 * Wrapper for `ChildExitCbShared` that associates a specific callback with a given command by its
 * index. This needs to be a separate structure as `running_cmds` in the shared structure only
 * makes sense if it is shared across all callbacks.
 */
typedef struct {
  guint              command_index;
  ChildExitCbShared *shared;
} ChildExitCbState;

/**
 * handle_process_fate:
 * @child_exit_cb_state: Should be uniquely allocated to each call of this function.
 * @success: If there was an error running the command, this should be set to TRUE.
 * @error: Information about the error running the command. Should only be set if `success` is TRUE.
 */
static void
handle_process_fate (ChildExitCbState *child_exit_cb_state,
                     gboolean success,
                     GError *error)
{
  ChildExitCbShared *child_exit_cb_shared = child_exit_cb_state->shared;

  /* Only attempt to retry the pkexec invocation if it was cancelled by the user ... */
  if (success || !g_error_matches (error, G_SPAWN_EXIT_ERROR, PKEXEC_CANCELLED_EXIT_CODE)) {
    /* Add the command to the list of entries to remove from the command list, but don't actually
     * remove them yet since the spawn loop may still be running which would cause problems. */
    g_array_prepend_val (child_exit_cb_shared->cmds_to_remove, child_exit_cb_state->command_index);
  }

  /* ... and only count it as an error if it wasn't due to user cancellation. */
  if (!success && !g_error_matches (error, G_SPAWN_EXIT_ERROR, PKEXEC_CANCELLED_EXIT_CODE))
    g_ptr_array_add (child_exit_cb_shared->error_array, error);
  else
    g_clear_error (&error);

  child_exit_cb_shared->running_cmds--;

  if (child_exit_cb_shared->running_cmds == 0) {
    /* If running_cmds has reached zero, we have run all the commands. Now we need to figure out
     * how it went and present this to the user. */
    guint cmd_count = child_exit_cb_shared->preferences_page->commands_to_run_as_administrator->len;

    gtk_widget_set_sensitive (GTK_WIDGET (child_exit_cb_shared->preferences_page->banner), TRUE);


    if (child_exit_cb_shared->cmds_to_remove->len == cmd_count) {
      /* Hide the banner if we are removing all the commands since there will be nothing to run. */
      adw_banner_set_revealed (child_exit_cb_shared->preferences_page->banner, FALSE);
    } else {
      /* If there still are commands left to run, the user must've cancelled the authentication. */
      adw_toast_overlay_add_toast (ADW_TOAST_OVERLAY (child_exit_cb_shared->preferences_page->toast_overlay),
                                   adw_toast_new_format (_("Authentication cancelled")));
    }

    if (child_exit_cb_shared->error_array->len != 0) {
      /* We had errors while running the commands, so inform the user of this. */
      AdwToast *error_toast = adw_toast_new_format (ngettext ("%i error occurred while saving",
                                                              "%i errors occurred while saving",
                                                              child_exit_cb_shared->error_array->len),
                                                    child_exit_cb_shared->error_array->len);

      for (guint i = 0; i < child_exit_cb_shared->error_array->len; i++) {
        GError *error_entry = g_ptr_array_index (child_exit_cb_shared->error_array, i);

        g_warning ("Error %i: %s", i + 1, error_entry->message);
      }

      adw_toast_overlay_add_toast (ADW_TOAST_OVERLAY (child_exit_cb_shared->preferences_page->toast_overlay),
                                   error_toast);
    }

    /* Remove any commands that weren't cancelled so we don't needlessly run them twice if the user
     * presses the button again to retry. */
    for (guint i = 0; i < child_exit_cb_shared->cmds_to_remove->len; i++) {
      g_ptr_array_remove_index_fast (child_exit_cb_shared->preferences_page->commands_to_run_as_administrator,
                                     g_array_index (child_exit_cb_shared->cmds_to_remove, int, i));
    }

    child_exit_cb_shared_free (child_exit_cb_shared);
  }

  g_free (child_exit_cb_state);
}


static void
on_child_exit (GPid pid, int wait_status, gpointer user_data)
{
  ChildExitCbState *child_exit_cb_state = user_data;
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = g_spawn_check_wait_status (wait_status, &error);

  handle_process_fate (child_exit_cb_state, success, g_steal_pointer (&error));

  g_spawn_close_pid (pid);
}


static void
on_save_as_administrator_pressed (AdwBanner *banner, gpointer preferences_page)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (preferences_page);
  ChildExitCbShared *child_exit_cb_shared;

  child_exit_cb_shared = child_exit_cb_shared_new (self);

  /* Disable banner button until we have figured out whether the commands succeeded. */
  gtk_widget_set_sensitive (GTK_WIDGET (banner), FALSE);

  for (guint i = 0; i < self->commands_to_run_as_administrator->len; i++) {
    GStrv cmd = g_ptr_array_index (self->commands_to_run_as_administrator, i);
    ChildExitCbState *child_exit_cb_state = NULL;
    gboolean spawn_success;
    GError *error = NULL;
    GPid child_pid;

    child_exit_cb_state = g_new0 (ChildExitCbState, 1);

    child_exit_cb_state->command_index = i;
    child_exit_cb_state->shared = child_exit_cb_shared;

    spawn_success = g_spawn_async (NULL,
                                   cmd,
                                   NULL,
                                   G_SPAWN_DO_NOT_REAP_CHILD,
                                   NULL, NULL,
                                   &child_pid,
                                   &error);

    if (spawn_success)
      g_child_watch_add (child_pid, on_child_exit, child_exit_cb_state);
    else
      handle_process_fate (child_exit_cb_state, spawn_success, error);
  }
}


static GString *
pretty_format_cmd (const GStrv cmd)
{
  g_autofree char *joint_cmds = NULL;
  GString *buf;

  buf = g_string_new ("# ");
  joint_cmds = g_strjoinv (" ", cmd);
  g_string_append (buf, joint_cmds);

  return buf;
}


static gboolean
is_command_queued (GPtrArray *commands_to_run_as_administrator, const char * const *cmd)
{
  gboolean match_found = FALSE;

  for (guint i = 0; i < commands_to_run_as_administrator->len; i++) {
    const char * const *cmd_entry = g_ptr_array_index (commands_to_run_as_administrator, i);

    if (g_strv_equal (cmd, cmd_entry)) {
      match_found = TRUE;
      break;
    }
  }

  return match_found;
}

/**
 * queue_command:
 * @self: Instance of MsTweaksPreferencesPage.
 * @cmd: The command to queue.
 */
static void
ms_tweaks_preferences_page_queue_command (MsTweaksPreferencesPage *self, GStrv cmd)
{
  g_autoptr (GString) cmd_fmt = NULL;

  /* Ensure we don't queue duplicate commands. */
  if (is_command_queued (self->commands_to_run_as_administrator, (const char * const *) cmd)) {
    g_strfreev (cmd);
    return;
  }

  g_ptr_array_add (self->commands_to_run_as_administrator, cmd);

  cmd_fmt = pretty_format_cmd (cmd);
  g_debug ("Queued command: %s\n", cmd_fmt->str);
}

/**
 * on_save_as_administrator_requested:
 * @backend: The backend that sent the request.
 * @from: Path to move file from.
 * @to: Path to move file to.
 * @user_data: Instance of MsTweaksPreferencesPage.
 *
 * Queues up the given command to later be run as administrator.
 */
static void
on_save_as_administrator_requested (MsTweaksBackend *backend,
                                    const char      *from,
                                    const char      *to,
                                    gpointer         user_data)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (user_data);
  g_autoptr (GStrvBuilder) mv_cmd_builder = NULL;
  g_autofree char *to_dirname = NULL;
  gboolean to_dir_exists;

  if (!from || g_utf8_strlen (from, -1) == 0) {
    g_critical ("Empty 'from' argument to save as administrator, ignoring request");
    return;
  }

  if (!to || g_utf8_strlen (to, -1) == 0) {
    g_critical ("Empty 'to' argument to save as administrator, ignoring request");
    return;
  }

  to_dirname = g_path_get_dirname (to);
  to_dir_exists = g_file_test (to_dirname, G_FILE_TEST_IS_DIR);

  if (!to_dir_exists) {
    g_autoptr (GStrvBuilder) make_dir_cmd_builder = g_strv_builder_new ();

    g_strv_builder_add_many (make_dir_cmd_builder,
                             PRIVILEGE_ESCALATION_PROGRAM,
                             "/usr/bin/mkdir",
                             "-p",
                             NULL);
    g_strv_builder_take (make_dir_cmd_builder, g_steal_pointer (&to_dirname));

    ms_tweaks_preferences_page_queue_command (self, g_strv_builder_end (make_dir_cmd_builder));
  }

  mv_cmd_builder = g_strv_builder_new ();

  g_strv_builder_add_many (mv_cmd_builder,
                           PRIVILEGE_ESCALATION_PROGRAM,
                           "/usr/bin/mv",
                           from,
                           to,
                           NULL);

  ms_tweaks_preferences_page_queue_command (self, g_strv_builder_end (mv_cmd_builder));

  adw_banner_set_revealed (ADW_BANNER (self->banner), TRUE);
}


static gboolean
ms_tweaks_preferences_page_initable_init (GInitable     *initable,
                                          GCancellable  *cancellable,
                                          GError       **error)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (initable);
  g_autoptr (GList) section_list = ms_tweaks_parser_sort_by_weight (self->data->section_table);
  g_autoptr (GtkStringList) search_keywords = gtk_string_list_new (NULL);
  gboolean page_widget_is_valid = FALSE;

  for (const GList *section_iter = section_list; section_iter; section_iter = section_iter->next) {
    const MsTweaksSection *section_data = section_iter->data;
    g_autoptr (GList) setting_list = ms_tweaks_parser_sort_by_weight (section_data->setting_table);
    GtkWidget *section_preference_group = adw_preferences_group_new ();
    gboolean section_widget_is_valid = FALSE;

    adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (section_preference_group),
                                     section_data->name_i18n);

    for (const GList *setting_iter = setting_list; setting_iter; setting_iter = setting_iter->next) {
      MsTweaksSetting *setting_data = setting_iter->data;
      GtkWidget *widget_to_add = NULL;
      gboolean setting_widget_is_valid = TRUE;
      MsTweaksBackend *backend_state = NULL;
      GValue *widget_value = NULL;

      switch (setting_data->backend) {
      case MS_TWEAKS_BACKEND_IDENTIFIER_HARDWAREINFO:
        ms_tweaks_warning (setting_data->name,
                           "The hardwareinfo backend is not supported. Please see the documentation.");
        setting_widget_is_valid = FALSE;
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_OSKSDL:
        ms_tweaks_warning (setting_data->name,
                           "The OSKSDL backend is not supported. Please see the documentation.");
        setting_widget_is_valid = FALSE;
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_UNKNOWN:
        ms_tweaks_debug (setting_data->name,
                         "Unknown backend type, cannot get value. Is your system up-to-date?");
        setting_widget_is_valid = FALSE;
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS:
        backend_state = ms_tweaks_backend_gsettings_new (setting_data);
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS:
        backend_state = ms_tweaks_backend_gtk3settings_new (setting_data);
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_SYMLINK:
        backend_state = ms_tweaks_backend_symlink_new (setting_data);
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_SYSFS:
        backend_state = ms_tweaks_backend_sysfs_new (setting_data);
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_XRESOURCES:
        backend_state = ms_tweaks_backend_xresources_new (setting_data);
        break;
      case MS_TWEAKS_BACKEND_IDENTIFIER_CSS:
      case MS_TWEAKS_BACKEND_IDENTIFIER_SOUNDTHEME:
      default:
        ms_tweaks_debug (setting_data->name,
                         "Unimplemented backend type '%s'",
                         pretty_format_backend_identifier (setting_data->backend));
        setting_widget_is_valid = FALSE;
        break;
      }

      /* Ensure that we actually constructed a tweaks backend. */
      if (!MS_IS_TWEAKS_BACKEND (backend_state)) {
        ms_tweaks_debug (setting_data->name, "Failed to construct backend, ignoring");
        continue;
      }

      /* Get widget value. */
      widget_value = ms_tweaks_backend_get_value (backend_state);

      /* Handle mappings. */
      if (widget_value) {
        g_autoptr (GError) mapping_error = NULL;
        gboolean success;

        success = ms_tweaks_mappings_handle_get (widget_value, setting_data, &mapping_error);

        if (!success) {
          ms_tweaks_warning (setting_data->name,
                             "Failed to handle mappings, ignoring: %s",
                             mapping_error->message);
          g_value_unset (widget_value);
          g_clear_pointer (&widget_value, g_free);
          continue;
        }
      }

      if (setting_widget_is_valid) {
        switch (setting_data->type) {
        case MS_TWEAKS_TYPE_BOOLEAN:
          widget_to_add = setting_data_to_boolean_widget (backend_state,
                                                          ADW_TOAST_OVERLAY (self->toast_overlay),
                                                          setting_data,
                                                          widget_value);
          break;
        case MS_TWEAKS_TYPE_CHOICE:
          widget_to_add = setting_data_to_choice_widget (backend_state,
                                                         ADW_TOAST_OVERLAY (self->toast_overlay),
                                                         setting_data,
                                                         widget_value);
          break;
        case MS_TWEAKS_TYPE_COLOR:
          widget_to_add = setting_data_to_color_widget (backend_state,
                                                        ADW_TOAST_OVERLAY (self->toast_overlay),
                                                        setting_data,
                                                        widget_value);
          break;
        case MS_TWEAKS_TYPE_FILE:
          MsTweaksPreferencesPageFilePickerMeta *metadata = g_new (MsTweaksPreferencesPageFilePickerMeta, 1);
          widget_to_add = setting_data_to_file_widget (setting_data,
                                                       backend_state,
                                                       widget_value,
                                                       ADW_TOAST_OVERLAY (self->toast_overlay),
                                                       metadata);
          break;
        case MS_TWEAKS_TYPE_FONT:
          widget_to_add = setting_data_to_font_widget (backend_state,
                                                       ADW_TOAST_OVERLAY (self->toast_overlay),
                                                       setting_data,
                                                       widget_value);
          break;
        case MS_TWEAKS_TYPE_INFO:
          widget_to_add = setting_data_to_info_widget (setting_data, widget_value);
          break;
        case MS_TWEAKS_TYPE_NUMBER:
          widget_to_add = setting_data_to_number_widget (backend_state,
                                                         ADW_TOAST_OVERLAY (self->toast_overlay),
                                                         setting_data,
                                                         widget_value);
          break;
        case MS_TWEAKS_TYPE_UNKNOWN:
          ms_tweaks_warning (setting_data->name,
                             "Unknown type, cannot create widget. Is your system up-to-date?");
          if (widget_value) {
            g_value_unset (widget_value);
            g_clear_pointer (&widget_value, g_free);
          }
          continue;
        default:
          ms_tweaks_critical (setting_data->name,
                              "Unimplemented setting type '%i'",
                              setting_data->type);
        }
      }

      if (widget_value) {
        g_value_unset (widget_value);
        g_clear_pointer (&widget_value, g_free);
      }

      if (widget_to_add) {
        adw_preferences_group_add (ADW_PREFERENCES_GROUP (section_preference_group), widget_to_add);
        gtk_string_list_append (search_keywords, setting_data->name_i18n);
        g_signal_connect (backend_state,
                          "save-as-administrator",
                          G_CALLBACK (on_save_as_administrator_requested),
                          self);
        section_widget_is_valid = TRUE;
      } else
        ms_tweaks_warning (setting_data->name, "Failed to construct widget");
    }

    if (section_widget_is_valid) {
      adw_preferences_page_add (ADW_PREFERENCES_PAGE (self->page),
                                ADW_PREFERENCES_GROUP (section_preference_group));
      gtk_string_list_append (search_keywords, section_data->name_i18n);
      page_widget_is_valid = TRUE;
    } else {
      g_debug ("No valid settings in section '%s' inside page '%s', hiding it",
               section_data->name,
               self->data->name);
    }
  }

  if (page_widget_is_valid) {
    gtk_string_list_append (search_keywords, self->data->name_i18n);
    ms_panel_set_keywords (MS_PANEL (self), search_keywords);
  }

  /* In case of failure g_initable_new will drop a ref so make this non-floating */
  if (!page_widget_is_valid)
    g_object_ref (self);

  return page_widget_is_valid;
}


MsTweaksPreferencesPage *
ms_tweaks_preferences_page_new (const MsTweaksPage *data)
{
  return g_initable_new (MS_TYPE_TWEAKS_PREFERENCES_PAGE, NULL, NULL, "data", data, NULL);
}


static void
ms_tweaks_preferences_page_initable_iface_init (GInitableIface *iface)
{
  iface->init = ms_tweaks_preferences_page_initable_init;
}


G_DEFINE_TYPE_WITH_CODE (MsTweaksPreferencesPage,
                         ms_tweaks_preferences_page,
                         MS_TYPE_PANEL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                ms_tweaks_preferences_page_initable_iface_init))


static void
ms_tweaks_preferences_page_init (MsTweaksPreferencesPage *self)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  self->page = adw_preferences_page_new ();
  self->banner = ADW_BANNER (adw_banner_new (_("You need to authenticate as administrator to save some settings")));
  self->toast_overlay = adw_toast_overlay_new ();

  adw_banner_set_button_label (ADW_BANNER (self->banner), "Save");
  adw_banner_set_button_style (ADW_BANNER (self->banner), ADW_BANNER_BUTTON_SUGGESTED);

  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->banner));
  gtk_box_append (GTK_BOX (box), self->page);
  adw_toast_overlay_set_child (ADW_TOAST_OVERLAY (self->toast_overlay), box);
  adw_bin_set_child (ADW_BIN (self), self->toast_overlay);

  g_signal_connect (self->banner,
                    "button-clicked",
                    G_CALLBACK (on_save_as_administrator_pressed),
                    self);

  self->commands_to_run_as_administrator = g_ptr_array_new_with_free_func ((GDestroyNotify) g_strfreev);
}


static void
ms_tweaks_preferences_page_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (object);

  switch (property_id) {
  case PROP_DATA:
    self->data = g_value_get_boxed (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_preferences_page_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (object);

  switch (property_id) {
  case PROP_DATA:
    g_value_set_boxed (value, self->data);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_preferences_page_finalize (GObject *object)
{
  MsTweaksPreferencesPage *self = MS_TWEAKS_PREFERENCES_PAGE (object);

  g_clear_pointer (&self->commands_to_run_as_administrator, g_ptr_array_unref);

  G_OBJECT_CLASS (ms_tweaks_preferences_page_parent_class)->finalize (object);
}


static void
ms_tweaks_preferences_page_class_init (MsTweaksPreferencesPageClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = ms_tweaks_preferences_page_finalize;
  gobject_class->set_property = ms_tweaks_preferences_page_set_property;
  gobject_class->get_property = ms_tweaks_preferences_page_get_property;

  props[PROP_DATA] = g_param_spec_boxed ("data", NULL, NULL, MS_TYPE_TWEAKS_PAGE, G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, G_N_ELEMENTS (props), props);
}
