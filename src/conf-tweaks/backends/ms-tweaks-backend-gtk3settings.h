/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include "ms-tweaks-backend-interface.h"
#include "ms-tweaks-parser.h"

#include <adwaita.h>

enum {
  MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR_FAILED_TO_CREATE_PARENTS,
  MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR_FAILED_TO_REMOVE_FROM_CONFIGURATION,
  MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR_FAILED_TO_WRITE_CONFIGURATION,
};

G_BEGIN_DECLS

#define MS_TYPE_TWEAKS_BACKEND_GTK3SETTINGS ms_tweaks_backend_gtk3settings_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksBackendGtk3settings, ms_tweaks_backend_gtk3settings, MS, TWEAKS_BACKEND_GTK3SETTINGS, GObject)

GQuark ms_tweaks_backend_gtk3settings_error_quark (void);
#define MS_TWEAKS_BACKEND_GTK3SETTINGS_ERROR ms_tweaks_backend_gtk3settings_error_quark ()

MsTweaksBackend *ms_tweaks_backend_gtk3settings_new (MsTweaksSetting *setting_data);

G_END_DECLS
