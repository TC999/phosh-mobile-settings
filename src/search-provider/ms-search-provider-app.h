/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Inspired by gnome-control-center search provider:
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/control-center-search-provider.h
 * https://gitlab.gnome.org/GNOME/gnome-control-center/-/blob/main/search-provider/control-center-search-provider.c
 *
 */

#pragma once

#include <gtk/gtk.h>

#include "ms-search-provider.h"

G_BEGIN_DECLS

typedef struct {
  GtkApplication parent;

  MsSearchProvider *search_provider;
} MsSearchProviderApp;

typedef struct {
  GtkApplicationClass parent_class;
} MsSearchProviderAppClass;

#define MS_TYPE_SEARCH_PROVIDER_APP ms_search_provider_app_get_type ()

#define MS_SEARCH_PROVIDER_APP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MS_TYPE_SEARCH_PROVIDER_APP, MsSearchProviderApp))

GType ms_search_provider_app_get_type (void) G_GNUC_CONST;

MsSearchProviderApp *ms_search_provider_app_get (void);

G_END_DECLS
