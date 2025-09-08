/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-backend-gsettings"

#include "ms-tweaks-backend-gsettings.h"
#include "../ms-tweaks-gtk-utils.h"
#include "../ms-tweaks-utils.h"

#include <adwaita.h>
#include <limits.h>
#include <stdlib.h>


struct _MsTweaksBackendGsettings {
  MsTweaksBackendInterface parent_interface;

  const MsTweaksSetting *setting_data;

  char *key;

  GSettings *settings;
};


static const MsTweaksSetting *
ms_tweaks_backend_gsettings_get_setting_data (MsTweaksBackend *self_)
{
  MsTweaksBackendGsettings *self = MS_TWEAKS_BACKEND_GSETTINGS (self_);

  return self->setting_data;
}


static GValue *
backend_gsettings_get_value (MsTweaksBackend *self_)
{
  MsTweaksBackendGsettings *self = MS_TWEAKS_BACKEND_GSETTINGS (self_);
  GValue *return_value = g_new0 (GValue, 1);

  if (self->setting_data->type == MS_TWEAKS_TYPE_BOOLEAN
      && self->setting_data->gtype == MS_TWEAKS_GTYPE_FLAGS) {
    const guint flags = g_settings_get_flags (self->settings, self->key);

    g_value_init (return_value, G_TYPE_UINT);
    g_value_set_uint (return_value, flags);
  } else {
    g_autoptr (GVariant) value_variant = g_settings_get_value (self->settings, self->key);

    g_dbus_gvariant_to_gvalue (value_variant, return_value);
  }

  return return_value;
}


static void
backend_gsettings_set_value (MsTweaksBackend *self_, GValue *value)
{
  MsTweaksBackendGsettings *self = MS_TWEAKS_BACKEND_GSETTINGS (self_);
  GType value_gtype;

  if (value) {
    value_gtype = G_VALUE_TYPE (value);

    switch (value_gtype) {
    case G_TYPE_BOOLEAN:
      g_settings_set_boolean (self->settings, self->key, g_value_get_boolean (value));
      break;
    case G_TYPE_DOUBLE:
      g_settings_set_double (self->settings, self->key, g_value_get_double (value));
      break;
    case G_TYPE_FLAGS:
      g_settings_set_flags (self->settings, self->key, g_value_get_flags (value));
      break;
    case G_TYPE_FLOAT:
      g_settings_set_flags (self->settings, self->key, g_value_get_flags (value));
      break;
    case G_TYPE_INT:
      g_settings_set_int (self->settings, self->key, g_value_get_int (value));
      break;
    case G_TYPE_STRING:
      g_settings_set_string (self->settings, self->key, g_value_get_string (value));
      break;
    case G_TYPE_UINT:
      g_settings_set_uint (self->settings, self->key, g_value_get_uint (value));
      break;;
    default:
      ms_tweaks_error (self->setting_data->name,
                       "Unsupported GType type: %s",
                       g_type_name (value_gtype));
      break;
    }
  } else
    g_settings_reset (self->settings, self->key);
}


static char *
ms_tweaks_backend_gsettings_get_key (MsTweaksBackend *self_)
{
  MsTweaksBackendGsettings *self = MS_TWEAKS_BACKEND_GSETTINGS (self_);

  return self->key;
}


static void
ms_tweaks_backend_gsettings_interface_init (MsTweaksBackendInterface *iface)
{
  iface->get_value = backend_gsettings_get_value;
  iface->set_value = backend_gsettings_set_value;

  iface->get_key = ms_tweaks_backend_gsettings_get_key;

  iface->get_setting_data = ms_tweaks_backend_gsettings_get_setting_data;
}


G_DEFINE_TYPE_WITH_CODE (MsTweaksBackendGsettings, ms_tweaks_backend_gsettings, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MS_TYPE_TWEAKS_BACKEND,
                                                ms_tweaks_backend_gsettings_interface_init))


static void
ms_tweaks_backend_gsettings_init (MsTweaksBackendGsettings *self)
{
}


static void
ms_tweaks_backend_gsettings_dispose (GObject *object)
{
  MsTweaksBackendGsettings *self = MS_TWEAKS_BACKEND_GSETTINGS (object);

  g_clear_pointer (&self->key, g_free);
  g_clear_pointer (&self->settings, g_object_unref);
}


static void
ms_tweaks_backend_gsettings_class_init (MsTweaksBackendGsettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_backend_gsettings_dispose;
}


MsTweaksBackend *
ms_tweaks_backend_gsettings_new (const MsTweaksSetting *setting_data)
{
  GSettingsSchemaSource *default_schema_source = g_settings_schema_source_get_default ();
  MsTweaksBackendGsettings *self = g_object_new (MS_TYPE_TWEAKS_BACKEND_GSETTINGS, NULL);

  if (setting_data->gtype == MS_TWEAKS_GTYPE_UNKNOWN) {
    ms_tweaks_warning (setting_data->name,
                       "Cannot create GSettings backend with gtype == GTYPE_UNKNOWN");
    g_object_unref (self);
    return NULL;
  }

  self->setting_data = setting_data;

  for (guint i = 0; i < setting_data->key->len; i++) {
    const char *key_entry = g_ptr_array_index (setting_data->key, i);
    char **parts = g_strsplit (key_entry, ".", 0);
    guint parts_length = g_strv_length (parts);
    g_autoptr (GSettingsSchema) schema = NULL;
    g_autofree char *base_key = NULL;
    gboolean schema_found = FALSE;
    char **schema_keys = NULL;

    if (parts_length < 3) {
      ms_tweaks_warning (self->setting_data->name,
                         "Invalid GSettings key '%s' (too few periods, must have at least 3)",
                         key_entry);
      g_strfreev (parts);
      continue;
    }

    self->key = g_strdup (parts[parts_length - 1]);
    /* We don't want the last segment in the "base key", so replace it with NULL. */
    g_free (parts[parts_length - 1]);
    parts[parts_length - 1] = NULL;

    base_key = g_strjoinv (".", parts);
    g_strfreev (parts);

    schema = g_settings_schema_source_lookup (default_schema_source, base_key, true);
    if (!schema) {
      ms_tweaks_debug (self->setting_data->name,
                       "Schema '%s' not found (this may be okay if there are multiple ones specified)",
                       base_key);
      continue;
    }

    schema_keys = g_settings_schema_list_keys (schema);

    for (guint j = 0; j < g_strv_length (schema_keys); j++)
      if (strcmp (schema_keys[j], self->key) == 0)
        schema_found = true;

    g_strfreev (schema_keys);

    if (!schema_found) {
      ms_tweaks_debug (self->setting_data->name,
                       "Schema key '%s' was not found in schema '%s' (this may be okay if there are multiple ones specified)",
                       self->key,
                       base_key);
      continue;
    }

    self->settings = g_settings_new (base_key);
    break;
  }

  if (!self->settings) {
    ms_tweaks_warning (self->setting_data->name, "Failed to create backend!");
    return NULL;
  }

  return MS_TWEAKS_BACKEND (self);
}
