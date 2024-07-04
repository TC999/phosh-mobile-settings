/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Gotam Gorabh <gautamy672@gmail.com>
 */

/*
 * Inspired by gnome-control-center search provider:
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/cc-search-provider.h
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/cc-search-provider.c
 *
 */

#include "mobile-settings-config.h"

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>
#include <string.h>

#include "ms-search-provider-app.h"
#include "ms-search-provider.h"

struct _MsSearchProvider
{
  GObject parent;

  MsOrgGnomeShellSearchProvider2 *skeleton;
};

G_DEFINE_TYPE (MsSearchProvider, ms_search_provider, G_TYPE_OBJECT)

static gboolean
handle_get_initial_result_set (MsSearchProvider        *self,
                               GDBusMethodInvocation   *invocation,
                               char                   **terms)
{
  return TRUE;
}

static gboolean
handle_get_subsearch_result_set (MsSearchProvider        *self,
                                 GDBusMethodInvocation   *invocation,
                                 char                   **previous_results,
                                 char                   **terms)
{
  return TRUE;
}

static gboolean
handle_get_result_metas (MsSearchProvider        *self,
                         GDBusMethodInvocation   *invocation,
                         char                   **results)
{
  return TRUE;
}

static gboolean
handle_activate_result (MsSearchProvider        *self,
                        GDBusMethodInvocation   *invocation,
                        char                    *identifier,
                        char                   **results,
                        guint                    timestamp)
{
  return TRUE;
}

static gboolean
handle_launch_search (MsSearchProvider        *self,
                      GDBusMethodInvocation   *invocation,
                      char                   **terms,
                      guint                    timestamp)
{
  return TRUE;
}

static void
ms_search_provider_init (MsSearchProvider *self)
{
  self->skeleton = ms_org_gnome_shell_search_provider2_skeleton_new ();

  g_signal_connect_swapped (self->skeleton, "handle-get-initial-result-set",
                            G_CALLBACK (handle_get_initial_result_set), self);
  g_signal_connect_swapped (self->skeleton, "handle-get-subsearch-result-set",
                            G_CALLBACK (handle_get_subsearch_result_set), self);
  g_signal_connect_swapped (self->skeleton, "handle-get-result-metas",
                            G_CALLBACK (handle_get_result_metas), self);
  g_signal_connect_swapped (self->skeleton, "handle-activate-result",
                            G_CALLBACK (handle_activate_result), self);
  g_signal_connect_swapped (self->skeleton, "handle-launch-search",
                            G_CALLBACK (handle_launch_search), self);
}

gboolean
ms_search_provider_dbus_register (MsSearchProvider  *self,
                                  GDBusConnection   *connection,
                                  const gchar       *object_path,
                                  GError           **error)
{
  GDBusInterfaceSkeleton *skeleton;

  skeleton = G_DBUS_INTERFACE_SKELETON (self->skeleton);

  return g_dbus_interface_skeleton_export (skeleton, connection, object_path, error);
}

void
ms_search_provider_dbus_unregister (MsSearchProvider *self,
                                    GDBusConnection  *connection,
                                    const gchar      *object_path)
{
  GDBusInterfaceSkeleton *skeleton;

  skeleton = G_DBUS_INTERFACE_SKELETON (self->skeleton);

  if (g_dbus_interface_skeleton_has_connection (skeleton, connection))
      g_dbus_interface_skeleton_unexport_from_connection (skeleton, connection);
}

static void
ms_search_provider_dispose (GObject *object)
{
  MsSearchProvider *self;

  self = MS_SEARCH_PROVIDER (object);

  g_clear_object (&self->skeleton);

  G_OBJECT_CLASS (ms_search_provider_parent_class)->dispose (object);
}

static void
ms_search_provider_class_init (MsSearchProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_search_provider_dispose;
}

MsSearchProvider *
ms_search_provider_new (void)
{
  return g_object_new (MS_TYPE_SEARCH_PROVIDER, NULL);
}

