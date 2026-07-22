/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/backends/ms-tweaks-backend-gsettings.h"
#include "test-tweaks-backend-common.h"


static void
test_gsettings_fixture_setup (BackendTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->gtype = MS_TWEAKS_GTYPE_STRING;
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup ("org.gnome.desktop.interface.color-scheme"));
  fixture->setting->map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  fixture->setting->default_ = g_strdup ("default");
  g_hash_table_insert (fixture->setting->map, g_strdup ("Default"), g_strdup ("default"));
  g_hash_table_insert (fixture->setting->map, g_strdup ("Light"), g_strdup ("prefer-light"));
  g_hash_table_insert (fixture->setting->map, g_strdup ("Dark:"), g_strdup ("prefer-dark"));

  fixture->backend = ms_tweaks_backend_gsettings_new (fixture->setting);
}


static void
test_gsettings_fixture_setup_alternative (BackendTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->type = MS_TWEAKS_TYPE_BOOLEAN;
  fixture->setting->gtype = MS_TWEAKS_GTYPE_FLAGS;
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup ("sm.puri.phosh.app-filter-mode"));
  fixture->setting->map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_hash_table_insert (fixture->setting->map, g_strdup ("true"), g_strdup ("1"));
  g_hash_table_insert (fixture->setting->map, g_strdup ("false"), g_strdup ("0"));

  fixture->backend = ms_tweaks_backend_gsettings_new (fixture->setting);
}


static void
test_set_alternative (BackendTestFixture *fixture, gconstpointer unused)
{
  g_autofree GValue *value = g_new0 (GValue, 1);
  g_autoptr (GError) error = NULL;

  g_value_init (value, G_TYPE_FLAGS);
  g_value_set_flags (value, 0);

  g_assert_true (fixture->backend);

  ms_tweaks_backend_set_value (fixture->backend, value, &error);

  g_assert_false (error);

  g_value_unset (value);
}


#define BACKEND_TEST_ADD(name, string_value, test_func) g_test_add ((name), \
                                                                    BackendTestFixture, \
                                                                    (string_value), \
                                                                    test_gsettings_fixture_setup, \
                                                                    (test_func), \
                                                                    test_backend_fixture_teardown)


#define BACKEND_TEST_ADD_ALT(name, test_func) g_test_add ((name), \
                                                          BackendTestFixture, \
                                                          NULL, \
                                                          test_gsettings_fixture_setup_alternative, \
                                                          (test_func), \
                                                          test_backend_fixture_teardown)


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-construct",
                    NULL,
                    test_construct);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-get",
                    NULL,
                    test_get);
  BACKEND_TEST_ADD_ALT ("/phosh-mobile-settings/test-tweaks-backend-gsettings-alternative",
                        test_get);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-set",
                    "prefer-light",
                    test_set);
  BACKEND_TEST_ADD_ALT ("/phosh-mobile-settings/test-tweaks-backend-gsettings-set-alternative",
                        test_set_alternative);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-remove",
                    NULL,
                    test_remove);

  return g_test_run ();
}
