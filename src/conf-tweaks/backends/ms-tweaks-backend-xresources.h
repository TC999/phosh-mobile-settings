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

#include <gtk/gtk.h>

enum {
  MS_TWEAKS_BACKEND_XRESOURCES_ERROR_FAILED_TO_CREATE_PARENTS,
  MS_TWEAKS_BACKEND_XRESOURCES_ERROR_NULL_KEY,
};

G_BEGIN_DECLS

#define MS_TYPE_TWEAKS_BACKEND_XRESOURCES ms_tweaks_backend_xresources_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksBackendXresources, ms_tweaks_backend_xresources, MS, TWEAKS_BACKEND_XRESOURCES, GObject)

GQuark ms_tweaks_backend_xresources_error_quark (void);
#define MS_TWEAKS_BACKEND_XRESOURCES_ERROR ms_tweaks_backend_xresources_error_quark ()

MsTweaksBackend *ms_tweaks_backend_xresources_new (const MsTweaksSetting *setting_data);

G_END_DECLS
