/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/backends/ms-tweaks-backend-gtk3settings.h"
#include "test-tweaks-backend-common.h"


static void
test_gtk3settings_fixture_setup (BackendTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->name = g_strdup ("Legacy prefer dark");
  fixture->setting->type = MS_TWEAKS_TYPE_BOOLEAN;
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup ("gtk-application-prefer-dark-theme"));
  fixture->setting->map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  fixture->setting->default_ = g_strdup ("0");
  g_hash_table_insert (fixture->setting->map, g_strdup ("true"), g_strdup ("1"));
  g_hash_table_insert (fixture->setting->map, g_strdup ("false"), g_strdup ("0"));

  fixture->backend = ms_tweaks_backend_gtk3settings_new (fixture->setting);
}


#define BACKEND_TEST_ADD(name, string_value, test_func) g_test_add ((name), \
                                                                    BackendTestFixture, \
                                                                    (string_value), \
                                                                    test_gtk3settings_fixture_setup, \
                                                                    (test_func), \
                                                                    test_backend_fixture_teardown)


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gtk3settings-construct",
                    NULL,
                    test_construct);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gtk3settings-set",
                    "false",
                    test_set);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gtk3settings-get",
                    NULL,
                    test_get);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gtk3settings-remove",
                    NULL,
                    test_remove);

  return g_test_run ();
}
