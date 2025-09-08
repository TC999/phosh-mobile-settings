/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-datasources"

#include "ms-tweaks-datasources.h"

#include <glob.h>

/**
 * glob_all:
 * @patterns: Patterns to glob. Tildes will be expanded.
 * @pattern_count: The number of elements in `patterns`.
 * @result: (out): Will be populated by the results of the glob invocations.
 *
 * Runs one glob invocation for each pattern in `patterns`, and stores all the results in `result`.
 */
static void
glob_all (const char **patterns, const int pattern_count, glob_t *results)
{
  int glob_flags = GLOB_NOSORT | GLOB_TILDE;
  gboolean is_first_iteration = TRUE;

  for (int i = 0; i < pattern_count; i++) {
    const char *pattern = patterns[i];
    int glob_ret = glob (pattern, glob_flags, NULL, results);

    switch (glob_ret) {
    case 0:
      /* Only add the GLOB_APPEND flag if the glob () call actually succeeds. */
      break;
    case GLOB_NOSPACE:
      g_warning ("Ran out of memory when globbing for '%s'", pattern);
      continue;
    case GLOB_ABORTED:
      g_warning ("Encountered a read error when globbing for '%s'", pattern);
      continue;
    case GLOB_NOMATCH:
      g_debug ("No matches when globbing for pattern '%s'", pattern);
      continue;
    default:
      g_critical ("Unknown glob %d encountered", glob_ret);
      continue;
    }

    if (is_first_iteration) {
      glob_flags |= GLOB_APPEND;
      is_first_iteration = FALSE;
    }
  }
}

/**
 * build_gtk3theme_hash_table_from_glob:
 * @glob_results: Instance of glob_t with matches that should be used when building the hash table.
 * @hash_table_to_build: Hash table that will contain the mappings. Does not have to be empty.
 *
 * Reads the matches from `glob_results` and uses them to build a hash table mapping human-readable
 * theme names to their machine-readable identifiers. Only works with GTK 3 themes.
 */
static void
build_gtk3theme_hash_table_from_glob (glob_t *glob_results, GHashTable *hash_table_to_build)
{
  for (size_t i = 0; i < glob_results->gl_pathc; i++) {
    const char *result = glob_results->gl_pathv[i];
    g_autoptr (GPathBuf) index_path_buf = g_path_buf_new_from_path (result);
    g_autofree char *gtk_css_path = NULL;
    g_autofree char *index_path = NULL;

    g_path_buf_push (index_path_buf, "gtk-3.0/gtk.css");

    gtk_css_path = g_path_buf_to_path (index_path_buf);

    if (g_file_test (gtk_css_path, G_FILE_TEST_IS_REGULAR)) {
      g_autoptr (GKeyFile) theme_index_key_file = g_key_file_new ();
      char *theme = g_path_get_basename (result);
      char *name = NULL;
      gboolean success;

      /* We added two elements, so we need to pop twice. */
      g_path_buf_pop (index_path_buf);
      g_path_buf_pop (index_path_buf);

      g_path_buf_push (index_path_buf, "index.theme");
      index_path = g_path_buf_free_to_path (g_steal_pointer (&index_path_buf));
      success = g_key_file_load_from_file (theme_index_key_file, index_path,
                                                     G_KEY_FILE_NONE, NULL);

      if (success) {
        name = g_key_file_get_string (theme_index_key_file, "X-GNOME-Metatheme", "name", NULL);
        name = g_key_file_get_string (theme_index_key_file, "Desktop Entry", "Name", NULL);
      }

      if (!name)
        name = g_strdup (theme);

      g_hash_table_insert (hash_table_to_build, name, theme);
    }
  }
}

/**
 * build_theme_hash_table_from_glob:
 * @glob_results: Instance of glob_t with matches that should be used when building the hash table.
 * @hash_table_to_build: Hash table that will contain the mappings. Does not have to be empty.
 * @theme_type: Name of group in key file where the "Name" section is. Used to get theme name.
 *
 * Reads the matches from `glob_results` and uses them to build a hash table mapping human-readable
 * theme names to their machine-readable identifiers. Works with at least icon themes and sound
 * themes.
 */
static void
build_theme_hash_table_from_glob (glob_t *glob_results,
                                  GHashTable *hash_table_to_build,
                                  const char *restrict theme_type)
{
  for (size_t i = 0; i < glob_results->gl_pathc; i++) {
    const char *result = glob_results->gl_pathv[i];
    g_autoptr (GKeyFile) theme_index_key_file = g_key_file_new ();
    GPathBuf *index_path_buf = g_path_buf_new_from_path (result);
    g_autofree char *index_path = NULL;

    g_path_buf_push (index_path_buf, "index.theme");
    index_path = g_path_buf_free_to_path (index_path_buf);

    if (g_key_file_load_from_file (theme_index_key_file, index_path, G_KEY_FILE_NONE, NULL)) {
      char *theme = g_path_get_basename (result);
      char *name = NULL;

      if ((name = g_key_file_get_string (theme_index_key_file, theme_type, "Name", NULL)) == NULL)
        name = g_strdup (theme);

      g_hash_table_insert (hash_table_to_build, name, theme);
    }
  }
}


static GHashTable *
default_datasource_hash_table_new (void)
{
  return g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}


static GHashTable *
ms_tweaks_datasource_gtk3themes (void)
{
  GHashTable *ret = default_datasource_hash_table_new ();
  const char *theme_paths[] = {"/usr/share/themes/*", "~/.local/share/themes/*", "~/.themes/*"};
  glob_t results;

  /* We can't use static strings here as then the hash table contains both statically and
   * dynamically allocated strings, which is unpleasant to deal with when freeing it. */
  g_hash_table_insert (ret, g_strdup ("Adwaita"), g_strdup ("Adwaita"));
  g_hash_table_insert (ret, g_strdup ("High Contrast"), g_strdup ("HighContrast"));

  glob_all (theme_paths, G_N_ELEMENTS (theme_paths), &results);

  build_gtk3theme_hash_table_from_glob (&results, ret);

  globfree (&results);

  return ret;
}


static GHashTable *
ms_tweaks_datasource_iconthemes (void)
{
  GHashTable *ret = default_datasource_hash_table_new ();
  const char *theme_paths[] = {"/usr/share/icons/*", "~/.local/share/icons/*", "~/.icons/*"};
  glob_t results;

  glob_all (theme_paths, G_N_ELEMENTS (theme_paths), &results);

  build_theme_hash_table_from_glob (&results, ret, "Icon Theme");

  globfree (&results);

  return ret;
}


static GHashTable *
ms_tweaks_datasource_soundthemes (void)
{
  GHashTable *ret = default_datasource_hash_table_new ();
  const char *theme_paths[] = {"/usr/share/sounds/*", "~/.local/share/sounds/*"};
  glob_t results;

  /* We can't use static strings here as then the hash table contains both statically and
   * dynamically allocated strings, which is unpleasant to deal with when freeing it. */
  g_hash_table_insert (ret, g_strdup ("Custom"), g_strdup ("__custom"));

  glob_all (theme_paths, G_N_ELEMENTS (theme_paths), &results);

  build_theme_hash_table_from_glob (&results, ret, "Sound Theme");

  globfree (&results);

  return ret;
}


GHashTable *
ms_tweaks_datasources_get_map (const char *datasource_identifier_str)
{
  GHashTable *ret = NULL;

  if (g_str_equal (datasource_identifier_str, "gtk3themes"))
    ret = ms_tweaks_datasource_gtk3themes ();
  else if (g_str_equal (datasource_identifier_str, "iconthemes"))
    ret = ms_tweaks_datasource_iconthemes ();
  else if (g_str_equal (datasource_identifier_str, "soundthemes"))
    ret = ms_tweaks_datasource_soundthemes ();
  else
    g_warning ("Unknown data source type '%s'", datasource_identifier_str);

  return ret;
}
