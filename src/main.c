/*
 * Copyright (C) 2022 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <glib/gi18n.h>

#include "mobile-settings-config.h"
#include "mobile-settings-application.h"

#include <libfeedback.h>

int
main (int argc, char *argv[])
{
  g_autoptr (MobileSettingsApplication) app = NULL;
  g_autoptr (GError) err = NULL;
  int ret;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  if (!lfb_init (MOBILE_SETTINGS_APP_ID, &err))
    g_warning ("Failed to init libfeedback: %s", err->message);

  app = mobile_settings_application_new (MOBILE_SETTINGS_APP_ID);
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  lfb_uninit ();

  return ret;
}
