/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Inspired by gnome-control-center search provider:
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/cc-search-provider.h
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/cc-search-provider.c
 *
 */

#pragma once

#include <glib-object.h>
#include <gio/gio.h>
#include "ms-shell-search-provider-generated.h"

G_BEGIN_DECLS

#define MS_TYPE_SEARCH_PROVIDER (ms_search_provider_get_type())

G_DECLARE_FINAL_TYPE (MsSearchProvider, ms_search_provider, MS, SEARCH_PROVIDER, GObject)

MsSearchProvider *ms_search_provider_new (void);

gboolean ms_search_provider_dbus_register   (MsSearchProvider  *provider,
                                             GDBusConnection   *connection,
                                             const char        *object_path,
                                             GError           **error);
void     ms_search_provider_dbus_unregister (MsSearchProvider  *provider,
                                             GDBusConnection   *connection,
                                             const char        *object_path);

G_END_DECLS
