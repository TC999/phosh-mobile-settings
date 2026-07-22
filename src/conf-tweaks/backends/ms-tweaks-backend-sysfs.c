/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-backend-sysfs"

#include "ms-tweaks-backend-sysfs.h"
#include "ms-tweaks-gtk-utils.h"
#include "ms-tweaks-utils.h"

#define SYSFS_PREFIX "/sys/"
#define SYSFS_CONFIG_NAME "phosh-mobile-settings-tweaks.conf"
#define INSTALLED_SYSFS_CONFIG_PATH "/etc/sysfs.d/" SYSFS_CONFIG_NAME
#define STAGED_SYSFS_CONFIG_DIR_PATH "phosh-mobile-settings"
#define STAGED_SYSFS_CONFIG_PATH STAGED_SYSFS_CONFIG_DIR_PATH "/" SYSFS_CONFIG_NAME


enum {
  PROP_0,
  PROP_KEY_BASEDIR,
  PROP_SETTING_DATA,
  PROP_INSTALLED_SYSFS_CONFIG,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MsTweaksBackendSysfs {
  MsTweaksBackendInterface parent_instance;

  MsTweaksSetting         *setting_data;
  GFile                   *installed_sysfs_config;
  char                    *key;
  char                    *key_basedir;
};


static void ms_tweaks_backend_sysfs_interface_init (MsTweaksBackendInterface *iface);
static void ms_tweaks_backend_sysfs_initable_iface_init (GInitableIface *iface);


G_DEFINE_TYPE_WITH_CODE (MsTweaksBackendSysfs, ms_tweaks_backend_sysfs, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MS_TYPE_TWEAKS_BACKEND,
                                                ms_tweaks_backend_sysfs_interface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                ms_tweaks_backend_sysfs_initable_iface_init))
G_DEFINE_QUARK (ms-tweaks-backend-sysfs-error-quark, ms_tweaks_backend_sysfs_error)

/**
 * get_absolute_staged_sysfs_config_dir_path:
 *
 * Returns: Path to the directory in which the sysfs config should be written or read.
 */
static char *
get_absolute_staged_sysfs_config_dir_path (void)
{
  return g_build_path ("/", g_get_user_cache_dir (), STAGED_SYSFS_CONFIG_DIR_PATH, NULL);
}

/**
 * get_staged_sysfs_config_path:
 *
 * Returns: Path to the exact file path which the sysfs config should be written to or read from.
 */
static char *
get_staged_sysfs_config_path (void)
{
  return g_build_path ("/", g_get_user_cache_dir (), STAGED_SYSFS_CONFIG_PATH, NULL);
}

/**
 * create_staged_sysfs_config_dir:
 *
 * Creates the directory where the staged sysfs config is stored.
 *
 * Returns: TRUE if successful, FALSE with errno set if there was an error.
 */
static gboolean
create_staged_sysfs_config_dir (void)
{
  g_autofree char *staged_sysfs_config_dir = get_absolute_staged_sysfs_config_dir_path ();

  return g_mkdir_with_parents (staged_sysfs_config_dir, 0755) == 0;
}

/**
 * copy_installed_to_staged:
 * @self: Instance of MsTweaksBackendSysfs where installed_sysfs_config will be read from.
 * @error: Unset error.
 *
 * Copy the config file installed inside of /etc to the staging location inside of `XDG_CACHE_HOME`.
 *
 * Returns: TRUE in the case of success, FALSE in the case of errors along with @error being set.
 */
static gboolean
copy_installed_to_staged (const MsTweaksBackendSysfs *self, GError **error)
{
  g_autofree char *staged_config_path = NULL;
  g_autoptr (GFile) staged_config = NULL;

  staged_config_path = get_staged_sysfs_config_path ();

  staged_config = g_file_new_for_path (staged_config_path);

  /* Ensure that this path exists. */
  if (!create_staged_sysfs_config_dir ()) {
    ms_tweaks_warning (self->setting_data->name,
                       "Failed to create sysfs config dir: '%s'",
                       g_strerror (errno));
    return FALSE;
  }

  return g_file_copy (self->installed_sysfs_config,
                      staged_config,
                      G_FILE_COPY_NONE,
                      NULL,
                      NULL,
                      NULL,
                      error);
}

/**
 * get_relevant_sysfs_config_stream:
 * Figure out which sysfs configuration file to read from—either the system-wide one installed
 * inside /etc/sysfs.d or the locally staged one inside the XDG user cache directory. The latter
 * has precedence if it exists. This returns a stream rather than a path or GFile to avoid TOCTOU.
 *
 * Returns: A stream from which the relevant configuration file can be read.
 */
static GFileInputStream *
get_relevant_sysfs_config_stream (GFile *installed_sysfs_config, GError **error)
{
  g_autoptr (GFile) staged_sysfs_config_file = NULL;
  g_autofree char *staged_sysfs_config_path = NULL;
  GFileInputStream *file_input_stream = NULL;
  g_autoptr (GError) local_error = NULL;

  staged_sysfs_config_path = get_staged_sysfs_config_path ();
  staged_sysfs_config_file = g_file_new_for_path (staged_sysfs_config_path);
  file_input_stream = g_file_read (staged_sysfs_config_file, NULL, &local_error);

  if (file_input_stream)
    return file_input_stream;

  if (!g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
    *error = g_steal_pointer (&local_error);
    return NULL;
  }

  return g_file_read (installed_sysfs_config, NULL, error);
}


static GValue *
read_value_from_sysfs (const MsTweaksBackendSysfs *self)
{
  g_autofree char *sysfs_path = NULL;
  char *contents = NULL;
  GValue *value = NULL;
  GError *error = NULL;

  sysfs_path = g_build_filename (self->key_basedir, self->key, NULL);

  if (!g_file_get_contents (sysfs_path, &contents, NULL, &error)) {
    ms_tweaks_warning (self->setting_data->name,
                       "Failed to read value from sysfs: %s",
                       error->message);
    return NULL;
  }

  g_strstrip (contents);
  value = g_new0 (GValue, 1);
  g_value_init (value, G_TYPE_STRING);
  g_value_take_string (value, contents);

  return value;
}

/**
 * ms_tweaks_backend_sysfs_get_value:
 * @backend: Instance of MsTweaksBackendSysfs.
 *
 * Looks for the most current value for the relevant sysfs property in 3 different places. In order
 * of lowest to highest precedence:
 *
 *  1. Actual value in /sys
 *  2. Value from installed configuration file in /etc
 *  3. Value from staged configuration file in ~/.cache
 *
 * (assuming default paths)
 *
 * This order was chosen to be least surprising for end users as it is most likely to result in the
 * value they choose being displayed.
 *
 * Returns: The current value for the sysfs property if found, otherwise NULL.
 */
static GValue *
ms_tweaks_backend_sysfs_get_value (MsTweaksBackend *backend)
{
  const MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (backend);
  g_autoptr (GFileInputStream) file_input_stream = NULL;
  g_autoptr (GDataInputStream) input_stream = NULL;
  g_autoptr (GFile) config_file = NULL;
  g_autoptr (GError) error = NULL;
  char *contents = NULL;
  GValue *value = NULL;
  char *line = NULL;

  /* If the property is readonly there's no point in trying to see if it has been set to something
   * else. */
  if (self->setting_data->readonly)
    return read_value_from_sysfs (self);

  file_input_stream = get_relevant_sysfs_config_stream (self->installed_sysfs_config, &error);

  if (!file_input_stream) {
    /* Only warn once for "No such file or directory" as it otherwise may get printed many times. */
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
      g_warning_once ("Failed to read: %s", error->message);
    else
      ms_tweaks_warning (self->setting_data->name, "Failed to read: %s", error->message);

    return read_value_from_sysfs (self);
  }

  input_stream = g_data_input_stream_new (G_INPUT_STREAM (file_input_stream));

  while ((line = g_data_input_stream_read_line (input_stream, NULL, NULL, &error)) != NULL) {
    if (line) {
      g_auto (GStrv) parts = NULL;
      char *key;

      g_strstrip (line);

      /* Ignore blank lines and comments, which are indicated by # according to sysfs.conf.pod. */
      if (g_utf8_strlen (line, -1) == 0 || line[0] == '#') {
        g_free (line);
        continue;
      }

      parts = g_strsplit (line, "=", 2);
      g_free (line);

      /* Invalid or unsupported line. */
      if (g_strv_length (parts) != 2)
        continue;

      key = parts[0];

      g_strstrip (key);

      /* Check if we found the right key. */
      if (g_str_equal (self->key, key)) {
        contents = g_steal_pointer (&parts[1]);
        break;
      }
    } else {
      ms_tweaks_warning (self->setting_data->name, "Error while reading: %s", error->message);

      g_clear_error (&error);
    }
  }

  if (contents) {
    g_strstrip (contents);
    value = g_new0 (GValue, 1);
    g_value_init (value, G_TYPE_STRING);
    g_value_take_string (value, contents);
  } else {
    return read_value_from_sysfs (self);
  }

  return value;
}

/**
 * make_entry:
 * @key: Path relative to /sys/.
 * @value: The value to write to the path provided in @key.
 *
 * Generates an entry that can be read by the update-sysfs script from Debian's sysfsutils package.
 *
 * Returns: Entry that can be read by the update-sysfs script.
 */
static char *
make_entry (const char *key, const char *value)
{
  g_assert (key);
  g_assert (value);

  return g_strconcat (key, " = ", value, "\n", NULL);
}


static gboolean
rewrite_existing_sysfs_conf (MsTweaksBackendSysfs  *self,
                             const char            *sysfs_config_contents,
                             const char            *sysfs_config_path,
                             const GValue          *new_value_container,
                             GError               **error)
{
  g_autofree char *line_to_insert = NULL, *line_to_replace = NULL;
  g_autoptr (GString) contents_string = NULL;
  g_autofree GValue *old_value = NULL;
  const char *new_value = NULL;

  g_assert (MS_IS_TWEAKS_BACKEND_SYSFS (self));
  g_assert (!*error);

  old_value = ms_tweaks_backend_sysfs_get_value (MS_TWEAKS_BACKEND (self));
  new_value = new_value_container ? g_value_get_string (new_value_container) : NULL;

  /* If there is no old value to remove and no new value to add, we have nothing to do. */
  if (!old_value && !new_value)
    return TRUE;

  /* If we have an old value to overwrite, start by reconstructing the full line so we . */
  if (old_value) {
    line_to_replace = make_entry (self->key, g_value_get_string (old_value));

    g_value_unset (old_value);
  }

  /* If we have a new value to replace the old one (or just insert), construct a line that sets it.
   * Otherwise, just make a blank line to replace the old value if we have one. We should never run
   * into the case where nether are set due to the check above. */
  if (new_value)
    line_to_insert = make_entry (self->key, new_value);
  else if (old_value)
    line_to_insert = g_strdup ("");

  g_debug ("Rewriting existing sysfs config at '%s'", sysfs_config_path);

  contents_string = g_string_new (sysfs_config_contents);

  /* If there is more than one identical entry, something is weird anyway. */
  if (line_to_replace)
    g_string_replace (contents_string, line_to_replace, line_to_insert, 1);

  if (new_value && g_str_equal (sysfs_config_contents, contents_string->str)) {
    /* No change happened from g_string_replace, which probably means that the property doesn't
     * yet exist in the sysfs config. Append a new line with it, if we have something to set. */

    /* But first, append a newline to the end of the file if necessary. */
    if (contents_string->len != 0 && g_str_has_suffix (contents_string->str, "\n"))
      g_string_append_c (contents_string, '\n');

    g_string_append_printf (contents_string, "%s\n", line_to_insert);
  }

  return g_file_set_contents (sysfs_config_path, contents_string->str, -1, error);
}


static char *
generate_default_config (const char *key, const char *value)
{
  g_autofree char *entry = NULL;

  g_assert (key);
  g_assert (value);

  entry = make_entry (key, value);

  return g_strconcat ("# This file is autogenerated and owned by Phosh Mobile Settings.\n",
                      entry,
                      NULL);
}


static gboolean
write_new_sysfs_conf (const MsTweaksBackendSysfs *self, const GValue *value, GError **error)
{
  g_autofree char *staged_config_contents = generate_default_config (self->key,
                                                                     g_value_get_string (value));
  g_autofree char *staged_config_path = get_staged_sysfs_config_path ();

  /* Ensure that this path exists. */
  if (!create_staged_sysfs_config_dir ()) {
    ms_tweaks_warning (self->setting_data->name,
                       "Failed to create sysfs config dir: '%s'",
                       g_strerror (errno));
    return FALSE;
  }

  return g_file_set_contents (staged_config_path, staged_config_contents, -1, error);
}


static gboolean
ms_tweaks_backend_sysfs_set_value (MsTweaksBackend *backend, GValue *value, GError **return_error)
{
  MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (backend);
  g_autofree char *staged_sysfs_config_path = NULL;
  g_autofree char *contents = NULL;
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = copy_installed_to_staged (self, &error);

  if (!success && !(g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)
                    || g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))) {
    if (return_error)
      *return_error = g_steal_pointer (&error);

    return success;
  }

  staged_sysfs_config_path = get_staged_sysfs_config_path ();

  if (success || g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
    g_clear_error (&error);

    success = g_file_get_contents (staged_sysfs_config_path, &contents, NULL, &error);

    if (success)
      success = rewrite_existing_sysfs_conf (self, contents, staged_sysfs_config_path, value, &error);
  } else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
    g_clear_error (&error);

    /* Only write a new configuration file if we are actually setting a value. */
    success = value ? write_new_sysfs_conf (self, value, &error) : TRUE;
  }

  if (success) {
    g_signal_emit_by_name (self,
                           "save-as-administrator",
                           staged_sysfs_config_path,
                           g_file_peek_path (self->installed_sysfs_config),
                           NULL);
  } else if (return_error) {
    *return_error = g_steal_pointer (&error);
  }

  return success;
}


static const MsTweaksSetting *
ms_tweaks_backend_sysfs_get_setting_data (MsTweaksBackend *backend)
{
  MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (backend);

  return self->setting_data;
}


static void
ms_tweaks_backend_sysfs_interface_init (MsTweaksBackendInterface *iface)
{
  iface->get_value = ms_tweaks_backend_sysfs_get_value;
  iface->set_value = ms_tweaks_backend_sysfs_set_value;

  iface->get_setting_data = ms_tweaks_backend_sysfs_get_setting_data;
}


static void
ms_tweaks_backend_sysfs_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (object);

  switch (property_id) {
  case PROP_KEY_BASEDIR:
    g_clear_pointer (&self->key_basedir, g_free);
    self->key_basedir = g_value_dup_string (value);
    break;
  case PROP_SETTING_DATA:
    self->setting_data = g_value_dup_boxed (value);
    break;
  case PROP_INSTALLED_SYSFS_CONFIG:
    g_clear_pointer (&self->installed_sysfs_config, g_object_unref);
    self->installed_sysfs_config = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_backend_sysfs_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (object);

  switch (property_id) {
  case PROP_KEY_BASEDIR:
    g_value_set_string (value, self->key_basedir);
    break;
  case PROP_SETTING_DATA:
    g_value_set_boxed (value, self->setting_data);
    break;
  case PROP_INSTALLED_SYSFS_CONFIG:
    g_value_set_object (value, self->installed_sysfs_config);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_tweaks_backend_sysfs_dispose (GObject *object)
{
  MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (object);

  g_clear_pointer (&self->setting_data, ms_tweaks_setting_free);
  g_clear_pointer (&self->key, g_free);
  g_clear_pointer (&self->key_basedir, g_free);
  g_clear_pointer (&self->installed_sysfs_config, g_object_unref);
}


static void
ms_tweaks_backend_sysfs_class_init (MsTweaksBackendSysfsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_backend_sysfs_dispose;
  object_class->set_property = ms_tweaks_backend_sysfs_set_property;
  object_class->get_property = ms_tweaks_backend_sysfs_get_property;

  props[PROP_KEY_BASEDIR] = g_param_spec_string ("key-basedir", "", "",
                                                 NULL,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  props[PROP_SETTING_DATA] = g_param_spec_boxed ("setting-data", "", "",
                                                 MS_TYPE_TWEAKS_SETTING,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_INSTALLED_SYSFS_CONFIG] = g_param_spec_object ("installed-sysfs-config", "", "",
                                                            G_TYPE_FILE,
                                                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (props), props);
}


static void
ms_tweaks_backend_sysfs_init (MsTweaksBackendSysfs *self)
{
  self->key_basedir = g_strdup (SYSFS_PREFIX);
  self->installed_sysfs_config = g_file_new_for_path (INSTALLED_SYSFS_CONFIG_PATH);
}

/**
 * canonicalize_sysfs_path:
 * @prefix: Prefix to remove (typically "/sys").
 * @sysfs_path: Path to a given node in /sys, which may or may not contain the "/sys/"-prefix. May
 *              be mutated in-place.
 *
 * While the original postmarketOS Tweaks had a homegrown daemon for handling everything that
 * requires root, our conf-tweaks implementation relies on update-sysfs. A key difference between
 * these two is that while the postmarketOS Tweaks daemon used a full path*, update-sysfs expects a
 * path relative to /sys/. For the sake of compatibility, this function transform paths from the
 * postmarketOS Tweaks style to the update-sysfs style and also rejects any paths outside /sys.
 *
 * *: This also meant that it hypothetically could write to any path, not just ones in /sys.
 */
static gboolean
canonicalize_sysfs_path (const char *prefix, char **sysfs_path, GError **error)
{
  if (!g_path_is_absolute (*sysfs_path)) {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_SYSFS_ERROR,
                 MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_BE_ABSOLUTE,
                 "Must be an absolute path: %s", *sysfs_path);
    return FALSE;
  }

  if (g_str_has_prefix (*sysfs_path, prefix)) {
    GString *sysfs_path_string = g_string_new_take (*sysfs_path);
    g_string_erase (sysfs_path_string, 0, strlen (prefix));
    *sysfs_path = g_string_free_and_steal (sysfs_path_string);
  } else {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_SYSFS_ERROR,
                 MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_HAVE_SYSFS_PREFIX,
                 "Must start with '%s': %s", prefix, *sysfs_path);
    return FALSE;
  }

  return TRUE;
}


static gboolean
ms_tweaks_backend_sysfs_initialise (GInitable *initable, GCancellable *cancellable, GError **error)
{
  MsTweaksBackendSysfs *self = MS_TWEAKS_BACKEND_SYSFS (initable);

  if (self->setting_data->stype != MS_TWEAKS_STYPE_INT && !self->setting_data->readonly) {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_SYSFS_ERROR,
                 MS_TWEAKS_BACKEND_SYSFS_ERROR_ONLY_STYPE_INT_SUPPORTED,
                 "Only stype 'int' is supported unless readonly is set");
    return FALSE;
  }

  self->key = g_strdup (ms_tweaks_util_get_single_key (self->setting_data->key));

  if (!self->key) {
    g_set_error (error,
                 MS_TWEAKS_BACKEND_SYSFS_ERROR,
                 MS_TWEAKS_BACKEND_SYSFS_ERROR_NO_KEY,
                 "Only single-element key values are allowed");
    return FALSE;
  }

  return canonicalize_sysfs_path (self->key_basedir, &self->key, error);
}


static void
ms_tweaks_backend_sysfs_initable_iface_init (GInitableIface *iface)
{
  iface->init = ms_tweaks_backend_sysfs_initialise;
}


MsTweaksBackend *
ms_tweaks_backend_sysfs_new (MsTweaksSetting *setting_data)
{
  MsTweaksBackendSysfs *self = g_initable_new (MS_TYPE_TWEAKS_BACKEND_SYSFS,
                                               NULL,
                                               NULL,
                                               "setting-data",
                                               setting_data,
                                               NULL);

  return MS_TWEAKS_BACKEND (self);
}
