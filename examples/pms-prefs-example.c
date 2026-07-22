/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Rudra Pratap Singh <rudrasingh900@gmail.com>
 */

#include "libpms.h"

#include <locale.h>

static void
on_app_activated (GApplication *app)
{
  MsOskLayoutPrefs *prefs = ms_osk_layout_prefs_new ();
  const char *locale;
  GtkWidget *win, *page;
  gboolean found;

  win = adw_application_window_new (GTK_APPLICATION (app));
  gtk_window_set_default_size (GTK_WINDOW (win), 360, 720);
  page = adw_preferences_page_new ();
  adw_application_window_set_content (ADW_APPLICATION_WINDOW (win), page);

  adw_preferences_page_add (ADW_PREFERENCES_PAGE (page),
                            ADW_PREFERENCES_GROUP (prefs));
  ms_osk_layout_prefs_load_osk_layouts (prefs);

  gtk_window_present (GTK_WINDOW (win));

  locale = setlocale (LC_CTYPE, NULL);
  found = ms_osk_layout_prefs_add_for_locale (prefs, locale, NULL);

  if (found)
    g_message ("Added a layout for locale %s", locale);
  else
    g_warning ("No layout found for locale %s - please submit one", locale);
}


int
main (int argc, char **argv)
{
  int status;
  AdwApplication *app = adw_application_new ("mobi.phosh.MobileSettings.LibmsExampleC",
                                             G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect_object (G_APPLICATION (app),
                           "activate",
                           G_CALLBACK (on_app_activated),
                           NULL,
                           G_CONNECT_DEFAULT);

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
