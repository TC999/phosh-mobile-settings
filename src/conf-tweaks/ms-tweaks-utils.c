/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-utils"

#include "ms-tweaks-utils.h"

#include <wordexp.h>


G_DEFINE_QUARK (ms-tweaks-utils-error-quark, ms_tweaks_utils_error)


const char *
ms_tweaks_util_boolean_to_string (const gboolean value)
{
  if (value)
    return "true";
  else
    return "false";
}

/**
 * ms_tweaks_util_string_to_boolean:
 * @string: A string of the form "true" or "false"
 *
 * This function assumes that string is valid input. Any invalid input will result in FALSE being
 * returned.
 *
 * Returns: The boolean representation of the string
 */
gboolean
ms_tweaks_util_string_to_boolean (const char *string)
{
  if (g_str_equal (string, "true"))
    return TRUE;
  else
    return FALSE;
}


GValue *
ms_tweaks_string_value_new_from_default (const char *default_)
{
  GValue *value = g_new0 (GValue, 1);
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, default_);

  return value;
}

/**
 * ms_tweaks_expand_single:
 * @to_expand: String to expand.
 * @error: Filled in the case of an error.
 *
 * Expands a tilde (~).
 *
 * Returns: Copy string provided in @to_expand with the tilde expanded, or NULL on error.
 */
char *
ms_tweaks_expand_single (const char *to_expand, GError **error)
{
  char *expanded_string = NULL;
  wordexp_t expansion_result;
  int ret = INT_MAX;

  if ((ret = wordexp (to_expand, &expansion_result, WRDE_NOCMD | WRDE_SHOWERR | WRDE_UNDEF)) != 0) {
    const char *reason = NULL;

    switch (ret) {
    case WRDE_BADCHAR:
      reason = "WRDE_BADCHAR: Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, }";
      break;
    case WRDE_CMDSUB:
      reason = "WRDE_CMDSUB: Command substitution is not allowed";
      break;
    case WRDE_NOSPACE:
      reason = "WRDE_NOSPACE: Out of memory";
      break;
    case WRDE_SYNTAX:
      reason = "Shell syntax error, such as unbalanced parentheses or unmatched quotes";
      break;
    case WRDE_BADVAL:
      reason = "An undefined shell variable was expanded";
      break;
    default:
      reason = "Unknown failure";
      break;
    }

    g_set_error (error,
                 MS_TWEAKS_UTILS_ERROR,
                 MS_TWEAKS_UTILS_ERROR_WORDEXP_FAILED,
                 "Failed to expand key '%s': %s",
                 to_expand,
                 reason);

    return NULL;
  }

  expanded_string = g_strdup (expansion_result.we_wordv[0]);
  wordfree (&expansion_result);

  return expanded_string;
}

/**
 * ms_tweaks_get_filename_extension:
 * @filename: File to get the file extension of.
 *
 * Modified from the original by ThiefMaster on Stack Overflow: https://stackoverflow.com/a/5309508
 *
 * Returns: The file extension of a file. This does not consider dotfiles (e.g. ".Xresources").
 */
const char *
ms_tweaks_get_filename_extension (const char *filename)
{
  char *dot;

  g_assert (filename);

  dot = strrchr (filename, '.');

  if (!dot || dot == filename)
    return "";

  return dot + 1;
}

/**
 * ms_tweaks_util_get_single_key:
 * @key_array: Array to get the first element of.
 *
 * Returns: The key if the array is 1 element long, otherwise NULL.
 */
const char *
ms_tweaks_util_get_single_key (const GPtrArray *key_array)
{
  const char *key = NULL;

  if (key_array->len == 1)
    key = g_ptr_array_index (key_array, 0);
  else
    g_warning ("Only single-element key values are allowed");

  return key;
}

/**
 * ms_tweaks_util_get_key_by_value_string:
 * @hash_table: The GHashTable to find the key in.
 * @value_to_find: The value to find the key of.
 *
 * Finds the first instance of the given value in a GHashTable. This should only be used on hash
 * tables where you can assume that every value is unique as hash tables don't guarantee any
 * particular ordering. Additionally, this only works for hash tables where both the keys and values
 * are strings.
 *
 * Returns: The key string if one was found, otherwise NULL.
 */
char *
ms_tweaks_util_get_key_by_value_string (GHashTable *hash_table,
                                        const char *value_to_find)
{
  gpointer key = NULL, value = NULL;
  char *matching_key = NULL;
  GHashTableIter iter;

  g_assert (hash_table);
  g_assert (value_to_find);

  g_hash_table_iter_init (&iter, hash_table);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    if (g_str_equal (value, value_to_find)) {
      matching_key = key;
      break;
    }
  }

  return matching_key;
}


void
ms_tweaks_log (const char     *log_domain,
               GLogLevelFlags  log_level,
               const char     *name,
               const char     *format,
               ...)
{
  va_list args;
  char *format_with_prefix = g_strconcat ("[Setting '", name, "'] ", format, NULL);

  va_start (args, format);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  g_logv (log_domain, log_level, format_with_prefix, args);
#pragma GCC diagnostic pop
  va_end (args);
  g_free (format_with_prefix);
}

/**
 * ms_tweaks_is_path_inside_user_home_directory:
 * @path: Absolute path without any variables or other things that need to be expanded.
 *
 * Determines whether a given path is inside of the current user's home directory.
 *
 * Returns: TRUE if it is within the user's home directory, FALSE otherwise.
 */
gboolean
ms_tweaks_is_path_inside_user_home_directory (const char *path)
{
  return g_str_has_prefix (path, g_get_home_dir ());
}
