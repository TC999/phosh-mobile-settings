/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-backend-gtk3settings"

#include "ms-tweaks-backend-gtk3settings.h"

#include "ms-tweaks-utils.h"

#define MS_TWEAKS_BACKEND_GTK3SETTINGS_FILENAME "gtk-3.0/settings.ini"
#define MS_TWEAKS_BACKEND_GTK3SETTINGS_SECTION "Settings"


enum {
  PROP_0,
  PROP_SETTING_DATA,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MsTweaksBackendGtk3settings {
  MsTweaksBackendInterface parent_interface;

  MsTweaksSetting *setting_data;
  const char *key;
};

/**
 * get_gtk3_configuration_path:
 *
 * Assembles the path to the GTK 3.0 configuration file and returns it. This cannot be replaced by a
 * constant as the exact path only can be determined at runtime.
 *
 * Returns: Full path to the GTK 3.0 configuration file.
 */
static char *
get_gtk3_configuration_path (void)
{
  return g_strconcat (g_get_user_config_dir (), "/", MS_TWEAKS_BACKEND_GTK3SETTINGS_FILENAME, NULL);
}


static GValue *
ms_tweaks_backend_gtk3settings_get_value (MsTweaksBackend *backend)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (backend);
  g_autofree char *gtk3_configuration_path = get_gtk3_configuration_path ();
  g_autoptr (GKeyFile) gtk3_configuration_file = g_key_file_new ();
  g_autofree char *configuration_value = NULL;
  g_autoptr (GError) error = NULL;
  GValue *value = NULL;

  if (self->setting_data->default_)
    value = ms_tweaks_string_value_new_from_default (self->setting_data->default_);

  if (!g_key_file_load_from_file (gtk3_configuration_file,
                                  gtk3_configuration_path,
                                  G_KEY_FILE_NONE, &error)) {
    ms_tweaks_info (self->setting_data->name,
                    "Failed to read configuration, falling back to default if it is set: %s",
                    error->message);
    return value;
  }

  configuration_value = g_key_file_get_value (gtk3_configuration_file,
                                              MS_TWEAKS_BACKEND_GTK3SETTINGS_SECTION,
                                              self->key,
                                              &error);

  if (!configuration_value) {
    ms_tweaks_warning (self->setting_data->name,
                       "Couldn't get key '%s', falling back to default if it is set: %s",
                       self->key,
                       error->message);
    return value;
  }

  if (!value)
    value = ms_tweaks_string_value_new_from_default (self->setting_data->default_);

  g_value_take_string (value, g_steal_pointer (&configuration_value));

  return value;
}


static gboolean
ms_tweaks_backend_gtk3settings_set_value (MsTweaksBackend *backend,
                                          GValue *new_value,
                                          GError **error)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (backend);
  g_autofree char *gtk3_configuration_path = get_gtk3_configuration_path();
  g_autofree char *gtk3_configuration_path_dir = NULL;
  g_autoptr (GKeyFile) gtk3_configuration_file = g_key_file_new ();

  if (!g_key_file_load_from_file (gtk3_configuration_file,
                                  gtk3_configuration_path,
                                  G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                  error)) {
    ms_tweaks_info (self->setting_data->name,
                    "Failed to read GTK 3.0 configuration at '%s', creating new file there",
                    gtk3_configuration_path);
    g_clear_error (error);
  }

  if (new_value) {
    const char *value_to_set = g_value_get_string (new_value);

    g_key_file_set_value (gtk3_configuration_file,
                          MS_TWEAKS_BACKEND_GTK3SETTINGS_SECTION,
                          self->key,
                          value_to_set);
    ms_tweaks_info (self->setting_data->name, "Setting %s = %s", self->key, value_to_set);
  } else {
    const gboolean was_removed = g_key_file_remove_key (gtk3_configuration_file,
                                                        MS_TWEAKS_BACKEND_GTK3SETTINGS_SECTION,
                                                        self->key,
                                                        error);

    if (!was_removed && (*error)->code == 3) {
      ms_tweaks_info (self->setting_data->name,
                      "Couldn't remove key '%s' as it doesn't exist: %s",
                      self->key,
                      (*error)->message);
      g_clear_error (error);
    } else if (!was_removed && (*error)->code == 4) {
      ms_tweaks_info (self->setting_data->name,
                      "Couldn't remove key '%s' as its group doesn't exist: %s",
                      self->key,
                      (*error)->message);
      g_clear_error (error);
    } else if (!was_removed) {
      g_set_error (error,
                   MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR,
                   MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR_FAILED_TO_REMOVE_FROM_CONFIGURATION,
                   "Failed to remove key '%s' from GTK 3-0 configuration file: %s",
                   self->key,
                   (*error)->message);
      return FALSE;
    }
  }

  gtk3_configuration_path_dir = g_path_get_dirname (gtk3_configuration_path);
  if (g_mkdir_with_parents (gtk3_configuration_path_dir, 0700) == -1) {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR,
                 MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR_FAILED_TO_CREATE_PARENTS,
                 "Failed to create leading directories '%s': %s",
                 gtk3_configuration_path_dir,
                 strerror (errno));
    return FALSE;
  }

  if (g_key_file_save_to_file (gtk3_configuration_file, gtk3_configuration_path, error)) {
    ms_tweaks_info (self->setting_data->name,
                    "Wrote GTK 3.0 configuration file to '%s'",
                    gtk3_configuration_path);
  } else {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR,
                 MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR_FAILED_TO_WRITE_CONFIGURATION,
                 "Failed to write GTK 3.0 configuration file to '%s': %s",
                 gtk3_configuration_path,
                 (*error)->message);
    return FALSE;
  }

  return TRUE;
}


static const MsTweaksSetting *
ms_tweaks_backend_gtk3settings_get_setting_data (MsTweaksBackend *backend)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (backend);

  return self->setting_data;
}


static void
ms_tweaks_backend_gtk3settings_interface_init (MsTweaksBackendInterface *iface)
{
  iface->get_value = ms_tweaks_backend_gtk3settings_get_value;
  iface->set_value = ms_tweaks_backend_gtk3settings_set_value;

  iface->get_setting_data = ms_tweaks_backend_gtk3settings_get_setting_data;
}


G_DEFINE_TYPE_WITH_CODE (MsTweaksBackendGtk3settings, ms_tweaks_backend_gtk3settings, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MS_TYPE_TWEAKS_BACKEND,
                                                ms_tweaks_backend_gtk3settings_interface_init))
G_DEFINE_QUARK (ms-tweaks-backend-gtk3settings-error-quark, ms_tweaks_backend_gtk3settings_error)


static void
ms_tweaks_backend_gtk3settings_init (MsTweaksBackendGtk3settings *self)
{
}


static void
ms_tweaks_backend_gtk3settings_dispose (GObject *object)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (object);

  g_clear_pointer (&self->setting_data, ms_tweaks_setting_free);
}


static void
ms_tweaks_backend_gtk3settings_constructed (GObject *object)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (object);

  self->key = ms_tweaks_util_get_single_key (self->setting_data->key);
}


static void
ms_tweaks_backend_gtk3settings_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (object);

  switch (property_id) {
  case PROP_SETTING_DATA:
    self->setting_data = g_value_dup_boxed (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_backend_gtk3settings_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  MsTweaksBackendGtk3settings *self = MS_TWEAKS_BACKEND_GTK3SETTINGS (object);

  switch (property_id) {
  case PROP_SETTING_DATA:
    g_value_set_boxed (value, self->setting_data);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_backend_gtk3settings_class_init (MsTweaksBackendGtk3settingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_backend_gtk3settings_dispose;
  object_class->constructed = ms_tweaks_backend_gtk3settings_constructed;
  object_class->set_property = ms_tweaks_backend_gtk3settings_set_property;
  object_class->get_property = ms_tweaks_backend_gtk3settings_get_property;

  props[PROP_SETTING_DATA] = g_param_spec_boxed ("setting-data",
                                                 NULL,
                                                 NULL,
                                                 MS_TYPE_TWEAKS_SETTING,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (props), props);
}


MsTweaksBackend *
ms_tweaks_backend_gtk3settings_new (MsTweaksSetting *setting_data)
{
  MsTweaksBackendGtk3settings *self = g_object_new (MS_TYPE_TWEAKS_BACKEND_GTK3SETTINGS,
                                                    "setting-data",
                                                    setting_data,
                                                    NULL);

  return MS_TWEAKS_BACKEND (self);
}
