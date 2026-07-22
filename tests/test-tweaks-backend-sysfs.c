/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/backends/ms-tweaks-backend-sysfs.c"
#include "test-tweaks-backend-common.h"

#define TMP_TEMPLATE "sysfs-XXXXXX"

/**
 * write_random_config_file:
 * @installed_config_path: Path to write to.
 *
 * Writes a random config file.
 *
 * Returns: The string that was written to the file.
 */
static char *
write_random_config_file (const char *config_path)
{
  char *config_contents;
  GError *error = NULL;

  /* Write something random to it. */
  config_contents = g_uuid_string_random (); /* Only random string generator I could find. */
  g_file_set_contents (config_path, config_contents, -1, &error);
  g_assert_no_error (error);

  return config_contents;
}


static char *
create_random_staged_config_file (void)
{
  g_autofree char *staged_config_path = NULL, *staged_config_dir_path = NULL;

  /* Create leading directories for sysfs config file. */
  staged_config_dir_path = get_absolute_staged_sysfs_config_dir_path ();
  g_assert_no_errno (g_mkdir_with_parents (staged_config_dir_path, 0755));

  /* Create staged sysfs config file. */
  staged_config_path = get_staged_sysfs_config_path ();

  return write_random_config_file (staged_config_path);
}


static GFile *
create_random_installed_config_file (void)
{
  g_autofree char *installed_config_path_dir = NULL;
  GError *error = NULL;

  /* Create temporary directory to write files to. */
  installed_config_path_dir = g_dir_make_tmp (TMP_TEMPLATE, &error);
  g_assert_no_error (error);

  /* Create file to read from. */
  return g_file_new_build_filename (installed_config_path_dir, SYSFS_CONFIG_NAME, NULL);
}

/**
 * test_get_relevant_sysfs_config_stream_neither_exist:
 * Ensures that if neither the staged configuration file in the user's home directory nor the
 * system-wide installed one exist, we get a NULL stream back along with a not found error.
 */
static void
test_get_relevant_sysfs_config_stream_neither_exist (void)
{
  g_autoptr (GFile) nonexistent = NULL;
  GFileInputStream *file_input_stream;
  g_autoptr (GError) error = NULL;

  nonexistent = g_file_new_for_path ("/nonexistent-path");
  file_input_stream = get_relevant_sysfs_config_stream (nonexistent, &error);

  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
  g_assert_null (file_input_stream);
}


static void
test_get_relevant_sysfs_config_stream_staged_exists (void)
{
  g_autoptr (GFileInputStream) file_input_stream = NULL;
  g_autoptr (GDataInputStream) data_input_stream = NULL;
  g_autofree char *config_contents = NULL;
  g_autoptr (GFile) nonexistent = NULL;
  g_autofree char *line = NULL;
  GError *error = NULL;

  config_contents = create_random_staged_config_file ();
  nonexistent = g_file_new_for_path ("/nonexistent-path");

  /* Get stream. */
  file_input_stream = get_relevant_sysfs_config_stream (nonexistent, &error);
  g_assert_no_error (error);
  g_assert_true (G_IS_FILE_INPUT_STREAM (file_input_stream));

  /* Ensure that we read back the expected value. */
  data_input_stream = g_data_input_stream_new (G_INPUT_STREAM (file_input_stream));
  line = g_data_input_stream_read_line (data_input_stream, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (config_contents, ==, line);
}



static void
test_get_relevant_sysfs_config_stream_installed_exists (void)
{
  g_autoptr (GFileInputStream) file_input_stream = NULL;
  g_autoptr (GDataInputStream) data_input_stream = NULL;
  g_autoptr (GFile) installed_config = NULL;
  g_autofree char *config_contents = NULL;
  g_autofree char *line = NULL;
  GError *error = NULL;

  installed_config = create_random_installed_config_file ();
  config_contents = write_random_config_file (g_file_peek_path (installed_config));

  /* Get stream. */
  file_input_stream = get_relevant_sysfs_config_stream (installed_config, &error);
  g_assert_no_error (error);
  g_assert_true (G_IS_FILE_INPUT_STREAM (file_input_stream));

  /* Ensure that we read back the expected value. */
  data_input_stream = g_data_input_stream_new (G_INPUT_STREAM (file_input_stream));
  line = g_data_input_stream_read_line (data_input_stream, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (config_contents, ==, line);
}


static void
test_get_relevant_sysfs_config_stream_both_exist (void)
{
  g_autoptr (GFileInputStream) file_input_stream = NULL;
  g_autoptr (GDataInputStream) data_input_stream = NULL;
  g_autoptr (GFile) installed_config = NULL;
  g_autofree char *config_contents = NULL;
  g_autofree char *unused = NULL;
  g_autofree char *line = NULL;
  GError *error = NULL;

  installed_config = create_random_installed_config_file ();
  unused = write_random_config_file (g_file_peek_path (installed_config));
  config_contents = create_random_staged_config_file ();

  /* Get stream. */
  file_input_stream = get_relevant_sysfs_config_stream (installed_config, &error);
  g_assert_no_error (error);
  g_assert_true (G_IS_FILE_INPUT_STREAM (file_input_stream));

  /* Ensure that we read back the expected value, particularly the one from the staged config
   * rather than the installed config. */
  data_input_stream = g_data_input_stream_new (G_INPUT_STREAM (file_input_stream));
  line = g_data_input_stream_read_line (data_input_stream, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (config_contents, ==, line);
}


#define REDUNDANT_PREFIX "/sys/"


static void
test_canonicalize_sysfs_path_absolute (void)
{
  g_autofree char *test_path = g_strdup ("/sys/class/dmi/id/board_name");
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = canonicalize_sysfs_path (REDUNDANT_PREFIX, &test_path, &error);

  g_assert_true (success);
  g_assert_no_error (error);
  g_assert_cmpstr (test_path, ==, "class/dmi/id/board_name");
}


static void
test_canonicalize_sysfs_path_relative (void)
{
  g_autofree char *test_path = g_strdup ("class/dmi/id/board_name");
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = canonicalize_sysfs_path (REDUNDANT_PREFIX, &test_path, &error);

  g_assert_false (success);
  g_assert_error (error,
                  MS_TWEAKS_BACKEND_SYSFS_ERROR,
                  MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_BE_ABSOLUTE);
}


static void
test_canonicalize_sysfs_path_sneaky (void)
{
  g_autofree char *test_path = g_strdup ("../tmp/something.txt");
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = canonicalize_sysfs_path (REDUNDANT_PREFIX, &test_path, &error);

  g_assert_false (success);
  g_assert_error (error,
                  MS_TWEAKS_BACKEND_SYSFS_ERROR,
                  MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_BE_ABSOLUTE);
}


static void
test_canonicalize_sysfs_path_crude (void)
{
  g_autofree char *test_path = g_strdup ("/sbin/sudo");
  g_autoptr (GError) error = NULL;
  gboolean success;

  success = canonicalize_sysfs_path (REDUNDANT_PREFIX, &test_path, &error);

  g_assert_false (success);
  g_assert_error (error,
                  MS_TWEAKS_BACKEND_SYSFS_ERROR,
                  MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_HAVE_SYSFS_PREFIX);
}


static void
on_save_as_administrator (MsTweaksBackendSysfs *self, char *from, char *to, gpointer unused)
{
}


static void
test_sysfs_fixture_setup (BackendTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GFile) installed_config_path = NULL;
  GError *error = NULL;

  installed_config_path = create_random_installed_config_file ();
  g_file_set_contents (g_file_peek_path (installed_config_path),
                       "class/power_supply/axp20x-battery/voltage_max = 4200000",
                       -1,
                       &error);
  g_assert_no_error (error);

  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->stype = MS_TWEAKS_STYPE_INT;
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup ("/sys/class/power_supply/axp20x-battery/voltage_max"));

  fixture->backend = ms_tweaks_backend_sysfs_new (fixture->setting);
  g_object_set (G_OBJECT (fixture->backend),
                "installed-sysfs-config",
                installed_config_path,
                NULL);
  g_signal_connect (fixture->backend,
                    "save-as-administrator",
                    G_CALLBACK (on_save_as_administrator),
                    NULL);
}


#define INCORRECT_DATE "9999-99-99"
#define CORRECT_DATE "1950-05-09"
#define SYSFS_PROPERTY "class/rtc/rtc0/date"
#define SYSFS_PROPERTY_ABSOLUTE "/sys/" SYSFS_PROPERTY


static void
test_sysfs_fixture_setup_readonly (BackendTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GFile) installed_config = NULL;
  g_autofree char *fake_sysfs_dir = NULL;
  g_autofree char *file_path = NULL;
  g_autofree char *dir_path = NULL;
  GError *error = NULL;

  /* Write a "trap" config that the code should ignore. */
  installed_config = create_random_installed_config_file ();
  g_file_set_contents (g_file_peek_path (installed_config),
                       SYSFS_PROPERTY " = " INCORRECT_DATE,
                       -1,
                       &error);
  g_assert_no_error (error);

  /* Write a fake sysfs entry that the code should read from. */
  fake_sysfs_dir = g_dir_make_tmp (TMP_TEMPLATE, &error);
  g_assert_no_error (error);
  dir_path = g_build_filename (fake_sysfs_dir, "/class/rtc/rtc0", NULL);
  file_path = g_build_filename (dir_path, "/date", NULL);
  g_mkdir_with_parents (dir_path, 0777);
  g_file_set_contents (file_path, CORRECT_DATE, -1, &error);
  g_assert_no_error (error);

  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->stype = MS_TWEAKS_STYPE_STRING;
  fixture->setting->readonly = TRUE;
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup (SYSFS_PROPERTY_ABSOLUTE));

  fixture->backend = ms_tweaks_backend_sysfs_new (fixture->setting);
  g_object_set (G_OBJECT (fixture->backend), "key-basedir", fake_sysfs_dir, NULL);
}


static void
test_get_readonly (BackendTestFixture *fixture, gconstpointer unused)
{
  g_autofree GValue *value = NULL;

  g_assert_nonnull (fixture->backend);

  value = ms_tweaks_backend_get_value (fixture->backend);

  g_assert_cmpstr (g_value_get_string (value), ==, CORRECT_DATE);

  g_value_unset (value);
}


#define BACKEND_TEST_ADD(name, test_func) g_test_add ((name), \
                                                      BackendTestFixture, \
                                                      NULL, \
                                                      test_sysfs_fixture_setup, \
                                                      (test_func), \
                                                      test_backend_fixture_teardown)



int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-get-relevant-sysfs-config-stream-neither-exist",
                   test_get_relevant_sysfs_config_stream_neither_exist);
  g_test_add_func ("/phosh-mobile-settings/test-get-relevant-sysfs-config-stream-staged-exists",
                   test_get_relevant_sysfs_config_stream_staged_exists);
  g_test_add_func ("/phosh-mobile-settings/test-get-relevant-sysfs-config-stream-installed-exists",
                   test_get_relevant_sysfs_config_stream_installed_exists);
  g_test_add_func ("/phosh-mobile-settings/test-get-relevant-sysfs-config-stream-both-exist",
                   test_get_relevant_sysfs_config_stream_both_exist);
  g_test_add_func ("/phosh-mobile-settings/test-get-canonicalize-sysfs-path-absolute",
                   test_canonicalize_sysfs_path_absolute);
  g_test_add_func ("/phosh-mobile-settings/test-get-canonicalize-sysfs-path-relative",
                   test_canonicalize_sysfs_path_relative);
  g_test_add_func ("/phosh-mobile-settings/test-get-canonicalize-sysfs-path-sneaky",
                   test_canonicalize_sysfs_path_sneaky);
  g_test_add_func ("/phosh-mobile-settings/test-get-canonicalize-sysfs-path-crude",
                   test_canonicalize_sysfs_path_crude);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-sysfs-construct", test_construct);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-sysfs-get", test_get);
  g_test_add ("/phosh-mobile-settings/test-tweaks-backend-sysfs-get-readonly",
              BackendTestFixture,
              NULL,
              test_sysfs_fixture_setup_readonly,
              test_get_readonly,
              test_backend_fixture_teardown);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-sysfs-set", test_set);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-sysfs-remove", test_remove);

  return g_test_run ();
}
