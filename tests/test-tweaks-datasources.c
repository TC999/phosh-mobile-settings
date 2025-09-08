/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/ms-tweaks-datasources.c"


/*
 * test_public_api_valid:
 * Only verifies that the following datasources actually return a non-null pointer as the data
 * contained in the hash tables is dependent on the filesystem state, so we can't rely on it being
 * consistent across different environments.
 */
static void
test_public_api_valid (void)
{
  GHashTable *gtk3themes_table = ms_tweaks_datasources_get_map ("gtk3themes");
  GHashTable *iconthemes_table = ms_tweaks_datasources_get_map ("iconthemes");
  GHashTable *soundthemes_table = ms_tweaks_datasources_get_map ("soundthemes");

  g_assert_true (gtk3themes_table);
  g_assert_true (iconthemes_table);
  g_assert_true (soundthemes_table);

  g_hash_table_unref (soundthemes_table);
  g_hash_table_unref (iconthemes_table);
  g_hash_table_unref (gtk3themes_table);
}


#define INVALID_DATASOURCE_NAME "turbobåt"


static void
test_public_api_invalid (void)
{
  GHashTable *invalid_table = NULL;

  g_test_expect_message ("ms-tweaks-datasources",
                         G_LOG_LEVEL_WARNING,
                         "Unknown data source type '"INVALID_DATASOURCE_NAME"'");

  invalid_table = ms_tweaks_datasources_get_map (INVALID_DATASOURCE_NAME);

  g_assert_false (invalid_table);
  g_test_assert_expected_messages ();
}

/*
 * test_build_hash_table_from_glob:
 * Ensures that the function does not crash even if the paths are nonexistent.
 */
static void
test_build_hash_table_from_glob (void)
{
  GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
  char *paths[] = {"/invalid/path", "/invalid/path/again"};
  glob_t results;

  results.gl_pathc = 2;
  results.gl_pathv = paths;

  build_theme_hash_table_from_glob (&results, hash_table, "Icon Theme");

  g_hash_table_unref (hash_table);
}

/*
 * test_build_gtk3theme_hash_table_from_glob:
 * Ensures that the function does not crash even if the paths are nonexistent.
 */
static void
test_build_gtk3theme_hash_table_from_glob (void)
{
  GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
  char *paths[] = {"/invalid/path", "/invalid/path/again"};
  glob_t results;

  results.gl_pathc = 2;
  results.gl_pathv = paths;

  build_gtk3theme_hash_table_from_glob (&results, hash_table);

  g_hash_table_unref (hash_table);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-tweaks-datasource-public-api-valid",
                   test_public_api_valid);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-datasource-public-api-invalid",
                   test_public_api_invalid);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-datasource-build-hash-table-from-glob",
                   test_build_hash_table_from_glob);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-datasource-build-gtk3theme-hash-table-from-glob",
                   test_build_gtk3theme_hash_table_from_glob);

  return g_test_run ();
}
