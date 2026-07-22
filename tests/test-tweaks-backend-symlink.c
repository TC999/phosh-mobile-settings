/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "backends/ms-tweaks-backend-symlink.h"
#include "test-tweaks-backend-common.h"

#include <gio/gio.h>


static void
test_symlink_fixture_setup (BackendTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->name = g_strdup ("Templink");
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup ("/tmp/pmos-tweaks-test"));
  fixture->setting->source_ext = TRUE;

  fixture->backend = g_initable_new (MS_TYPE_TWEAKS_BACKEND_SYMLINK,
                                     NULL,
                                     NULL,
                                     "block-target-outside-home",
                                     FALSE,
                                     "setting-data",
                                     fixture->setting,
                                     "source-ext",
                                     fixture->setting->source_ext,
                                     NULL);
}


#define BACKEND_TEST_ADD(name, string_value, test_func) g_test_add ((name), \
                                                                    BackendTestFixture, \
                                                                    (string_value), \
                                                                    test_symlink_fixture_setup, \
                                                                    (test_func), \
                                                                    test_backend_fixture_teardown)


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);


  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-construct",
                    NULL,
                    test_construct);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-set",
                    "/doesnotexist/pmos-tweaks-test-other",
                    test_set);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-get",
                    NULL,
                    test_get);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-gsettings-remove",
                    NULL,
                    test_remove);

  return g_test_run ();
}
