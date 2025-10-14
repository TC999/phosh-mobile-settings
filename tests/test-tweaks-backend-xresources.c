/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "test-tweaks-backend-constants.h"

#include "conf-tweaks/backends/ms-tweaks-backend-xresources.h"
#include "test-tweaks-backend-common.h"


static void
test_xresources_fixture_setup (BackendTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->setting->name = g_strdup ("Dwm background or something idk");
  fixture->setting->key = g_ptr_array_new_full (1, g_free);
  g_ptr_array_add (fixture->setting->key, g_strdup ("dwm.background"));

  fixture->backend = ms_tweaks_backend_xresources_new (fixture->setting);
  ms_tweaks_backend_xresources_set_xresources_path (fixture->backend,
                                                    g_strdup (MS_TWEAKS_BACKEND_TEST_DIRECTORY"/.Xresources"));
}


#define BACKEND_TEST_ADD(name, string_value, test_func) g_test_add ((name), \
                                                                    BackendTestFixture, \
                                                                    string_value, \
                                                                    test_xresources_fixture_setup, \
                                                                    (test_func), \
                                                                    test_backend_fixture_teardown)


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-xresources-construct",
                    NULL,
                    test_construct);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-xresources-set",
                    "#005577",
                    test_set);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-xresources-get",
                    NULL,
                    test_get);
  BACKEND_TEST_ADD ("/phosh-mobile-settings/test-tweaks-backend-xresources-remove",
                    NULL,
                    test_remove);

  return g_test_run ();
}
