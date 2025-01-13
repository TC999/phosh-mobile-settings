/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Gotam Gorabh <gautamy672@gmail.com>
 */

/*
 * Inspired by gnome-control-center search provider:
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/control-center-search-provider.h
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/control-center-search-provider.c
 *
 */

#include "mobile-settings-config.h"

#include <glib/gi18n.h>
#include <stdlib.h>

#include <gio/gio.h>

#include "ms-search-provider.h"
#include "ms-search-provider-app.h"

G_DEFINE_TYPE (MsSearchProviderApp, ms_search_provider_app, GTK_TYPE_APPLICATION);

static gboolean
ms_search_provider_app_dbus_register (GApplication    *application,
                                      GDBusConnection *connection,
                                      const gchar     *object_path,
                                      GError         **error)
{
  MsSearchProviderApp *self;

  if (!G_APPLICATION_CLASS (ms_search_provider_app_parent_class)->dbus_register (application,
                                                                                 connection,
                                                                                 object_path,
                                                                                 error))
    return FALSE;

  self = MS_SEARCH_PROVIDER_APP (application);

  return ms_search_provider_dbus_register (self->search_provider, connection,
                                           object_path, error);
}

static void
ms_search_provider_app_dbus_unregister (GApplication    *application,
                                        GDBusConnection *connection,
                                        const gchar     *object_path)
{
  MsSearchProviderApp *self;

  self = MS_SEARCH_PROVIDER_APP (application);
  if (self->search_provider)
    ms_search_provider_dbus_unregister (self->search_provider, connection, object_path);

  G_APPLICATION_CLASS (ms_search_provider_app_parent_class)->dbus_unregister (application,
                                                                              connection,
                                                                              object_path);
}

static void
ms_search_provider_app_dispose (GObject *object)
{
  MsSearchProviderApp *self;

  self = MS_SEARCH_PROVIDER_APP (object);

  g_clear_object (&self->search_provider);

  G_OBJECT_CLASS (ms_search_provider_app_parent_class)->dispose (object);
}

static void
ms_search_provider_app_init (MsSearchProviderApp *self)
{
  self->search_provider = ms_search_provider_new ();
}

static void
ms_search_provider_app_startup (GApplication *application)
{
}

static void
ms_search_provider_app_class_init (MsSearchProviderAppClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->dispose = ms_search_provider_app_dispose;

  app_class->dbus_register = ms_search_provider_app_dbus_register;
  app_class->dbus_unregister = ms_search_provider_app_dbus_unregister;
  app_class->startup = ms_search_provider_app_startup;
}

MsSearchProviderApp *
ms_search_provider_app_get (void)
{
  static MsSearchProviderApp *singleton;

  if (singleton)
    return singleton;

  singleton = g_object_new (MS_TYPE_SEARCH_PROVIDER_APP,
                            "application-id", "mobi.phosh.MobileSettings.SearchProvider",
                            "flags", G_APPLICATION_IS_SERVICE,
                            NULL);

  return singleton;
}

int main (int argc, char **argv)
{
  GApplication *app;

  app = G_APPLICATION (ms_search_provider_app_get ());
  return g_application_run (app, argc, argv);
}
