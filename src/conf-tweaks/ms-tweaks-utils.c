/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-utils"

#include "ms-tweaks-utils.h"


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
