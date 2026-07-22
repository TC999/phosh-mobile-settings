/*
 * Copyright (C) 2022-2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <glib/gi18n.h>

#include "mobile-settings-config.h"
#include "ms-application.h"
#include "ms-main.h"

#include "libpms.h"

#include <glib-unix.h>


static gboolean
on_sigkill (gpointer user_data)
{
  g_message ("Received SIGTERM, quitting");
  g_application_quit (g_application_get_default ());

  return FALSE;
}


int
main (int argc, char *argv[])
{
  g_autoptr (MsApplication) app = NULL;
  g_autoptr (GError) err = NULL;
  int ret;

  /* Init libpms */
  ms_init ();
  /* Init the private lib */
  ms_internal_init ();

  app = ms_application_new (MOBILE_SETTINGS_APP_ID);
  g_unix_signal_add (SIGTERM, on_sigkill, NULL);
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  ms_internal_uninit ();

  return ret;
}
