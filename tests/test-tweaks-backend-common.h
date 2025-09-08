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
