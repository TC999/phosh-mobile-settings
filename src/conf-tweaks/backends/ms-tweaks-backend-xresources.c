/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-backend-xresources"

#include "ms-tweaks-backend-xresources.h"

#include "ms-tweaks-gtk-utils.h"
#include "ms-tweaks-utils.h"

#include <gio/gio.h>


struct _MsTweaksBackendXresources {
  MsTweaksBackendInterface parent_interface;

  const MsTweaksSetting *setting_data;

  const char            *key;

  char                  *xresources_path;
};


static char *
ms_tweaks_backend_xresources_get_xresources_path (MsTweaksBackendXresources *self)
{
  GPathBuf *xresources_path;

  if (self->xresources_path)
    xresources_path = g_path_buf_new_from_path (self->xresources_path);
  else {
    xresources_path = g_path_buf_new_from_path (g_get_home_dir ());
    g_path_buf_push (xresources_path, ".Xresources");
  }

  return g_path_buf_free_to_path (xresources_path);
}


void
ms_tweaks_backend_xresources_set_xresources_path (MsTweaksBackend *backend, char *xresources_path)
{
  MsTweaksBackendXresources *self = MS_TWEAKS_BACKEND_XRESOURCES (backend);
  self->xresources_path = xresources_path;
}


static GValue *
ms_tweaks_backend_xresources_get_value (MsTweaksBackend *backend)
{
  MsTweaksBackendXresources *self = MS_TWEAKS_BACKEND_XRESOURCES (backend);
  g_autoptr (GDataInputStream) input_stream = NULL;
  g_autoptr (GFileInputStream) file_input_stream;
  g_autofree char *xresources_path;
  g_autoptr (GError) error = NULL;
  g_autoptr (GFile) input_file;
  g_autofree char *line = NULL;
  GValue *value;

  xresources_path = ms_tweaks_backend_xresources_get_xresources_path (self);
  input_file = g_file_new_for_path (xresources_path);
  file_input_stream = g_file_read (input_file, NULL, &error);

  value = g_new0 (GValue, 1);
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, self->setting_data->default_);

  if (!file_input_stream) {
    /* Only warn once for "No such file or directory" as it otherwise may get printed many times. */
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
      g_warning_once ("Failed to read: %s", error->message);
    else
      ms_tweaks_warning (self->setting_data->name, "Failed to read: %s", error->message);

    return value;
  }

  input_stream = g_data_input_stream_new (G_INPUT_STREAM (file_input_stream));

  while ((line = g_data_input_stream_read_line (input_stream, NULL, NULL, &error)) != NULL) {
    if (!error) {
      char *result = g_strstr_len (line, -1, self->key);

      if (line == result) {
        /* We found a line where the start matches our key. */
        g_auto (GStrv) key_value_pair = g_strsplit (line, ": ", 2);

        if (g_strv_length (key_value_pair) == 2) {
          g_strstrip (key_value_pair[1]);
          g_value_set_string (value, key_value_pair[1]);
          break;
        } else {
          ms_tweaks_warning (self->setting_data->name,
                             "Malformed matching line skipped: %s",
                             line);
        }
      }

      g_free (line);
    } else {
      ms_tweaks_warning (self->setting_data->name, "Error while reading: %s", error->message);

      g_clear_error (&error);
    }
  }

  return value;
}


static gboolean
rewrite_existing_xresources (MsTweaksBackendXresources  *self,
                             const char                 *xresources_contents,
                             const char                 *xresources_path,
                             const char                 *new_value,
                             GError                    **error)
{
  /* Reading the file another time just to find what value currently is there is pretty ugly,
   * should be possible to do this in a better way. However, given that this only is done in the
   * setter method (which isn't called in startup), I'm not sure it's worth optimising. */
  g_autofree GValue *old_value = ms_tweaks_backend_xresources_get_value (MS_TWEAKS_BACKEND (self));
  g_autoptr (GString) contents_string = g_string_new (xresources_contents);
  g_autofree char *line_to_replace = g_strconcat (self->key,
                                                  ": ",
                                                  g_value_get_string (old_value),
                                                  NULL);
  g_autofree char *line_to_insert = NULL;

  g_value_unset (old_value);

  if (new_value)
    line_to_insert = g_strconcat (self->key, ": ", new_value, NULL);
  else
    line_to_insert = g_strdup (""); /* Remove the entry by replacing it with an empty string. */

  g_debug ("Rewriting existing Xresources at \"%s\"", xresources_path);
  /* If there is more than one identical entry, something is weird anyway. */
  g_string_replace (contents_string, line_to_replace, line_to_insert, 1);

  if (new_value && g_str_equal (xresources_contents, contents_string->str)) {
    /* No change happened from g_string_replace, which probably means that the property doesn't
     * yet exist in Xresources. Append a new line with it, if we have a new colour to set. */

    /* But first, append a newline to the end of the file if necessary. */
    if (contents_string->len != 0 && contents_string->str[contents_string->len - 1] != '\n')
      g_string_append_c (contents_string, '\n');

    g_string_append_printf (contents_string, "%s\n", line_to_insert);
  }

  if (!g_file_set_contents (xresources_path, contents_string->str, -1, error)) {
    g_warning ("Error while writing to Xresources at \"%s\": %s",
               xresources_path,
               (*error)->message);
    return FALSE;
  }

  return TRUE;
}


static gboolean
write_new_xresources (MsTweaksBackendXresources  *self,
                      const char                 *xresources_path,
                      const char                 *new_value,
                      GError                    **error)
{
  g_autofree char *new_xresources = g_strconcat (self->key, ": ", new_value, NULL);
  g_autofree char *xresources_dir_path = g_path_get_dirname (xresources_path);

  g_debug ("xresources doesn't exist at \"%s\", creating new one (error: %s)",
           xresources_path,
           (*error)->message);
  g_clear_error (error);

  if (g_mkdir_with_parents (xresources_dir_path, 0700) == -1) {
    ms_tweaks_warning (self->setting_data->name,
                       "failed to create leading directories \"%s\": %s",
                       xresources_dir_path,
                       strerror (errno));
    g_set_error (error,
                 MS_TWEAKS_BACKEND_XRESOURCES_ERROR,
                 MS_TWEAKS_BACKEND_XRESOURCES_ERROR_FAILED_TO_CREATE_PARENTS,
                 "Failed to write new Xresources");
    return FALSE;
  }

  if (!g_file_set_contents (xresources_path, new_xresources, -1, error)) {
    ms_tweaks_warning (self->setting_data->name,
                       "error while writing to xresources at \"%s\": %s",
                       xresources_path,
                       (*error)->message);
    return FALSE;
  }

  return TRUE;
}


static gboolean
ms_tweaks_backend_xresources_set_value (MsTweaksBackend *backend,
                                        GValue *new_value_,
                                        GError **error)
{
  MsTweaksBackendXresources *self = MS_TWEAKS_BACKEND_XRESOURCES (backend);
  const char *new_value = new_value_ ? g_value_get_string (new_value_) : NULL;
  g_autofree char *xresources_path = NULL;
  g_autofree char *contents = NULL;
  gboolean success;

  xresources_path = ms_tweaks_backend_xresources_get_xresources_path (self);

  if (!self->key) {
    ms_tweaks_warning (self->setting_data->name, "key was NULL. Can't set property.");
    return FALSE;
  }

  if (g_file_get_contents (xresources_path, &contents, NULL, error))
    success = rewrite_existing_xresources (self, contents, xresources_path, new_value, error);
  else
    success = write_new_xresources (self, xresources_path, new_value, error);

  return success;
}


static const MsTweaksSetting *
ms_tweaks_backend_xresources_get_setting_data (MsTweaksBackend *backend)
{
  MsTweaksBackendXresources *self = MS_TWEAKS_BACKEND_XRESOURCES (backend);

  return self->setting_data;
}


static void
ms_tweaks_backend_xresources_interface_init (MsTweaksBackendInterface *iface)
{
  iface->get_value = ms_tweaks_backend_xresources_get_value;
  iface->set_value = ms_tweaks_backend_xresources_set_value;

  iface->get_setting_data = ms_tweaks_backend_xresources_get_setting_data;
}


G_DEFINE_TYPE_WITH_CODE (MsTweaksBackendXresources, ms_tweaks_backend_xresources, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MS_TYPE_TWEAKS_BACKEND,
                                                ms_tweaks_backend_xresources_interface_init))
G_DEFINE_QUARK (ms-tweaks-backend-xresources-error-quark, ms_tweaks_backend_xresources_error)


static void
ms_tweaks_backend_xresources_init (MsTweaksBackendXresources *self)
{
}


static void
ms_tweaks_backend_xresources_dispose (GObject *object)
{
  MsTweaksBackendXresources *self = MS_TWEAKS_BACKEND_XRESOURCES (object);

  g_clear_pointer (&self->xresources_path, g_free);
}


static void
ms_tweaks_backend_xresources_class_init (MsTweaksBackendXresourcesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_backend_xresources_dispose;
}


MsTweaksBackend *
ms_tweaks_backend_xresources_new (const MsTweaksSetting *setting_data)
{
  MsTweaksBackendXresources *self = g_object_new (MS_TYPE_TWEAKS_BACKEND_XRESOURCES, NULL);

  self->setting_data = setting_data;
  self->key = ms_tweaks_util_get_single_key (setting_data->key);

  return MS_TWEAKS_BACKEND (self);
}
