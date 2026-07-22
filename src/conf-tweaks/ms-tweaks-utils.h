/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include <glib.h>
#include <glib-object.h>

enum {
  MS_TWEAKS_UTILS_ERROR_WORDEXP_FAILED,
};

GQuark ms_tweaks_utils_error_quark (void);
#define MS_TWEAKS_UTILS_ERROR ms_tweaks_utils_error_quark ()

/* Data conversion functions. */
const char *ms_tweaks_util_boolean_to_string (const gboolean value);
const char *ms_tweaks_get_filename_extension (const char *filename);
char *ms_tweaks_expand_single (const char *to_expand, GError **error);

/* Value retrieval functions. */
const char *ms_tweaks_util_get_single_key (const GPtrArray *key_array);
char *ms_tweaks_util_get_key_by_value_string (GHashTable *hash_table, const char *value_to_find);
gboolean ms_tweaks_util_string_to_boolean (const char *string);
GValue *ms_tweaks_string_value_new_from_default (const char *default_);

/* Miscellaneous utilities. */
#define ms_tweaks_error(name, ...)    ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_ERROR, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_message(name, ...)  ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_MESSAGE, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_critical(name, ...) ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_CRITICAL, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_warning(name, ...)  ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_WARNING, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_info(name, ...)     ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_INFO, \
                                                     (name), \
                                                     __VA_ARGS__)
#define ms_tweaks_debug(name, ...)    ms_tweaks_log (G_LOG_DOMAIN, \
                                                     G_LOG_LEVEL_DEBUG, \
                                                     (name), \
                                                     __VA_ARGS__)

void ms_tweaks_log (const char     *log_domain,
                    GLogLevelFlags  log_level,
                    const char     *name,
                    const char     *message,
                    ...) G_GNUC_PRINTF(4, 5);

gboolean ms_tweaks_is_path_inside_user_home_directory (const char *path);
