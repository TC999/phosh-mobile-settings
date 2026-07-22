/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-backend-symlink"

#include "ms-tweaks-backend-symlink.h"

#include "ms-tweaks-utils.h"

#include <gio/gio.h>

#include <glob.h>
#include <unistd.h>


enum {
  PROP_0,
  PROP_BLOCK_TARGET_OUTSIDE_HOME,
  PROP_SETTING_DATA,
  PROP_SOURCE_EXT,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


typedef struct _MsTweaksBackendSymlinkPrivate {
  MsTweaksSetting *setting_data;

  char *key;
  gboolean source_ext;
  gboolean block_target_outside_home;

  char *file_extension;
} MsTweaksBackendSymlinkPrivate;


static void ms_tweaks_backend_symlink_interface_init (MsTweaksBackendInterface *iface);
static void ms_tweaks_backend_symlink_initable_iface_init (GInitableIface *iface);


G_DEFINE_TYPE_WITH_CODE (MsTweaksBackendSymlink, ms_tweaks_backend_symlink, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MS_TYPE_TWEAKS_BACKEND,
                                                ms_tweaks_backend_symlink_interface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                ms_tweaks_backend_symlink_initable_iface_init)
                         G_ADD_PRIVATE (MsTweaksBackendSymlink))
G_DEFINE_QUARK (ms-tweaks-backend-symlink-error-quark, ms_tweaks_backend_symlink_error)


static GValue *
ms_tweaks_backend_symlink_get_value (MsTweaksBackend *backend)
{
  g_autofree char *link = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree GValue *value = g_new0 (GValue, 1);
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (backend);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);

  g_assert (private->setting_data->key);
  g_value_init (value, G_TYPE_STRING);

  /* If source_ext is set, we need to guess which file extension the symlink has. */
  if (private->source_ext) {
    g_autofree char *key_with_asterisk = g_strconcat (private->key, ".*", NULL);
    glob_t results;

    glob (key_with_asterisk, GLOB_NOSORT | GLOB_PERIOD, NULL, &results);

    for (size_t i = 0; i < results.gl_pathc; i++) {
      const char *match = results.gl_pathv[i];

      if (g_file_test (match, G_FILE_TEST_IS_SYMLINK)) {
        private->file_extension = g_strdup (ms_tweaks_get_filename_extension (match));
        g_value_set_string (value, match);
        break;
      }
    }

    globfree (&results);
  } else {
    g_value_set_string (value, private->key);
  }

  link = g_value_steal_string (value);
  if (link) {
    g_autofree char *link_target = NULL;

    if ((link_target = g_file_read_link (link, &error)) == NULL) {
      if (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
        ms_tweaks_info (private->setting_data->name,
                        "Symlink at '%s' doesn't exist yet: %s",
                        link,
                        error->message);
      } else {
        ms_tweaks_warning (private->setting_data->name,
                           "Failed to read symlink at '%s': %s",
                           link,
                           error->message);
      }

      return NULL;
    }

    g_value_take_string (value, g_steal_pointer (&link_target));

    return g_steal_pointer (&value);
  } else {
    return NULL;
  }
}


static gboolean
remove_symlink (const char *link, GError **error)
{
  /* Yeah it's vulnerable to TOCTOU but you tell me how to write this better. */
  if (g_file_test (link, G_FILE_TEST_EXISTS)) {
    if (g_file_test (link, G_FILE_TEST_IS_SYMLINK)) {
      if (unlink (link) == 0)
        return TRUE;
      else {
        g_set_error (error,
                     MS_TWEAKS_BACKEND_SYMLINK_ERROR,
                     MS_TWEAKS_BACKEND_SYMLINK_ERROR_FAILED_TO_REMOVE,
                     "Failed to remove symlink at '%s': %s",
                     link,
                     strerror (errno));
        return FALSE;
      }
    } else {
      g_set_error (error,
                   MS_TWEAKS_BACKEND_SYMLINK_ERROR,
                   MS_TWEAKS_BACKEND_SYMLINK_ERROR_WRONG_FILE_TYPE,
                   "'%s' is not a symlink, refusing to remove it!",
                   link);
      return FALSE;
    }
  } else {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_SYMLINK_ERROR,
                 MS_TWEAKS_BACKEND_SYMLINK_ERROR_SYMLINK_NONEXISTENT,
                 "'%s' doesn't exist, cannot remove it",
                 link);
    return FALSE;
  }
}


static gboolean
ms_tweaks_backend_symlink_set_value (MsTweaksBackend *backend,
                                     GValue *value_container,
                                     GError **error)
{
  g_autofree char *link = NULL;
  g_autofree char *target = NULL;
  const char *value = value_container ? g_value_get_string (value_container) : NULL;
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (backend);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);

  /* Update private->file_extension if we are making a new symlink. */
  if (value) {
    /* Remove old symlink before creating a new one. */
    if (!ms_tweaks_backend_symlink_set_value (backend, NULL, error))
      return FALSE;

    target = ms_tweaks_expand_single (value, error);

    private->file_extension = g_strdup (ms_tweaks_get_filename_extension (target));

    if (!private->file_extension && *error) {
      ms_tweaks_info (private->setting_data->name,
                      "Couldn't get filename extension for '%s': %s",
                      target,
                      (*error)->message);
    }

    g_clear_error (error);
  }

  /* Construct full symlink path. */
  if (private->source_ext) {
    if (private->file_extension == NULL && value == NULL) {
      /* If file_extension is NULL when we are trying to remove a file, there is probably no file
       * to remove. */
      return TRUE;
    }

    link = g_strconcat (private->key, ".", private->file_extension, NULL);
  } else {
    /* Copy rather than borrow the string for consistency with the source_ext == TRUE condition, as
     * to not have to later figure out whether we have to free `link` or not. */
    link = g_strdup (private->key);
  }

  if (!value) {
    /* Remove symlink. */
    g_autoptr (GError) remove_symlink_error = NULL;
    gboolean success = remove_symlink (link, &remove_symlink_error);

    if (success) {
      ms_tweaks_info (private->setting_data->name, "Removed symlink at '%s'", link);
    } else if (remove_symlink_error->code == MS_TWEAKS_BACKEND_SYMLINK_ERROR_SYMLINK_NONEXISTENT) {
      ms_tweaks_info (private->setting_data->name,
                      "Didn't remove symlink as it doesn't exist: %s",
                      link);
      /* The symlink not yet existing isn't an issue in this context since we just were trying to
       * remove it anyway. */
      success = TRUE;
    } else {
      ms_tweaks_warning (private->setting_data->name, "%s", remove_symlink_error->message);
      *error = g_steal_pointer (&remove_symlink_error);
    }

    g_free (g_steal_pointer (&private->file_extension));

    return success;
  } else {
    /* Create symlink. */
    g_autofree char *link_directory = g_path_get_dirname (link);

    if (g_mkdir_with_parents (link_directory, 0755) == -1) {
      g_set_error (error,
                   MS_TWEAKS_BACKEND_SYMLINK_ERROR,
                   MS_TWEAKS_BACKEND_SYMLINK_ERROR_FAILED_TO_CREATE_LEADING_DIRS,
                   "Failed to create leading directories for '%s': %s",
                   link,
                   strerror (errno));
      return FALSE;
    }

    if (symlink (target, link) == 0) {
      ms_tweaks_info (private->setting_data->name,
                      "Created symlink to '%s' at '%s'",
                      target,
                      link);
      return TRUE;
    } else if (errno == EEXIST) {
      ms_tweaks_info (private->setting_data->name,
                      "Didn't create symlink '%s' at '%s' because it already exists",
                      target,
                      link);
      return TRUE;
    } else {
      g_set_error (error,
                   MS_TWEAKS_BACKEND_SYMLINK_ERROR,
                   MS_TWEAKS_BACKEND_SYMLINK_ERROR_FAILED_TO_CREATE,
                   "Failed to create symlink to '%s' at '%s': %s",
                   target,
                   link,
                   strerror (errno));
      return FALSE;
    }
  }
}


static const char *
ms_tweaks_backend_symlink_get_key (MsTweaksBackend *backend)
{
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (backend);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);

  return private->key;
}


static const MsTweaksSetting *ms_tweaks_backend_symlink_get_setting_data (MsTweaksBackend *backend)
{
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (backend);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);

  return private->setting_data;
}


static void
ms_tweaks_backend_symlink_init (MsTweaksBackendSymlink *self)
{
}


static void
ms_tweaks_backend_symlink_dispose (GObject *gobject)
{
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (MS_TWEAKS_BACKEND_SYMLINK (gobject));

  g_clear_pointer (&private->key, g_free);
  g_clear_pointer (&private->file_extension, g_free);
  g_clear_pointer (&private->setting_data, ms_tweaks_setting_free);

  G_OBJECT_CLASS (ms_tweaks_backend_symlink_parent_class)->dispose (gobject);
}


static gboolean
ms_tweaks_backend_symlink_initialise (GInitable *initable,
                                      GCancellable *cancellable,
                                      GError **error)
{
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (initable);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);
  const char *raw_key;

  private->file_extension = NULL; /* Filled by get_value (). */

  if (((raw_key = ms_tweaks_util_get_single_key (private->setting_data->key)) == NULL)
      || ((private->key = ms_tweaks_expand_single (raw_key, error)) == NULL))
    return FALSE;

  if (private->block_target_outside_home && !ms_tweaks_is_path_inside_user_home_directory (private->key)) {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_SYMLINK_ERROR,
                 MS_TWEAKS_BACKEND_SYMLINK_ERROR_SYMLINK_OUTSIDE_HOME,
                 "Only symlink targets inside of your home directory are allowed (\"%s\" is outside)",
                 private->key);
    return FALSE;
  }

  return TRUE;
}


static void
ms_tweaks_backend_symlink_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (object);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);

  switch (property_id) {
  case PROP_BLOCK_TARGET_OUTSIDE_HOME:
    private->block_target_outside_home = g_value_get_boolean (value);
    break;
  case PROP_SETTING_DATA:
    private->setting_data = g_value_dup_boxed (value);
    break;
  case PROP_SOURCE_EXT:
    private->source_ext = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_backend_symlink_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  MsTweaksBackendSymlink *self = MS_TWEAKS_BACKEND_SYMLINK (object);
  MsTweaksBackendSymlinkPrivate *private = ms_tweaks_backend_symlink_get_instance_private (self);

  switch (property_id) {
  case PROP_BLOCK_TARGET_OUTSIDE_HOME:
    g_value_set_boolean (value, private->block_target_outside_home);
    break;
  case PROP_SETTING_DATA:
    g_value_set_boxed (value, private->setting_data);
    break;
  case PROP_SOURCE_EXT:
    g_value_set_boolean (value, private->source_ext);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_backend_symlink_class_init (MsTweaksBackendSymlinkClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_backend_symlink_dispose;
  object_class->set_property = ms_tweaks_backend_symlink_set_property;
  object_class->get_property = ms_tweaks_backend_symlink_get_property;

  props[PROP_BLOCK_TARGET_OUTSIDE_HOME] = g_param_spec_boolean ("block-target-outside-home",
                                                                NULL,
                                                                NULL,
                                                                TRUE,
                                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_SETTING_DATA] = g_param_spec_boxed ("setting-data",
                                                 NULL,
                                                 NULL,
                                                 MS_TYPE_TWEAKS_SETTING,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_SOURCE_EXT] = g_param_spec_boolean ("source-ext",
                                                 NULL,
                                                 NULL,
                                                 FALSE,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (props), props);
}


static void
ms_tweaks_backend_symlink_interface_init (MsTweaksBackendInterface *iface)
{
  iface->get_value = ms_tweaks_backend_symlink_get_value;
  iface->set_value = ms_tweaks_backend_symlink_set_value;

  iface->get_setting_data = ms_tweaks_backend_symlink_get_setting_data;

  iface->get_key = ms_tweaks_backend_symlink_get_key;
}


static void
ms_tweaks_backend_symlink_initable_iface_init (GInitableIface *iface)
{
  iface->init = ms_tweaks_backend_symlink_initialise;
}


MsTweaksBackend *
ms_tweaks_backend_symlink_new (const MsTweaksSetting *setting_data)
{
  MsTweaksBackendSymlink *self = g_initable_new (MS_TYPE_TWEAKS_BACKEND_SYMLINK,
                                                 NULL,
                                                 NULL,
                                                 "setting-data",
                                                 setting_data,
                                                 "source-ext",
                                                 setting_data->source_ext,
                                                 NULL);

  return MS_TWEAKS_BACKEND (self);
}
