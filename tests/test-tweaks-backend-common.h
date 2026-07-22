/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/ms-tweaks-backend-interface.h"


typedef struct {
  MsTweaksBackend *backend;
  MsTweaksSetting *setting;
} BackendTestFixture;


static void
test_backend_fixture_teardown (BackendTestFixture *fixture, gconstpointer unused)
{
  g_object_unref (fixture->backend);
  ms_tweaks_setting_free (fixture->setting);
}


static void
test_construct (BackendTestFixture *fixture, gconstpointer unused)
{
  g_assert_nonnull (fixture->backend);
}


static void
test_get (BackendTestFixture *fixture, gconstpointer unused)
{
  g_autofree GValue *value = NULL;

  g_assert_nonnull (fixture->backend);

  value = ms_tweaks_backend_get_value (fixture->backend);

  g_assert_nonnull (value);

  g_value_unset (value);
}


static void
test_set (BackendTestFixture *fixture, gconstpointer string_value)
{
  g_autofree GValue *value = g_new0 (GValue, 1);
  g_autoptr (GError) error = NULL;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, string_value);

  g_assert_nonnull (fixture->backend);

  ms_tweaks_backend_set_value (fixture->backend, value, &error);

  g_assert_false (error);

  g_value_unset (value);
}


static void
test_remove (BackendTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) error = NULL;

  g_assert_nonnull (fixture->backend);

  ms_tweaks_backend_set_value (fixture->backend, NULL, &error);

  g_assert_false (error);
}
