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

G_BEGIN_DECLS

enum {
  MS_TWEAKS_BACKEND_SYSFS_ERROR_ONLY_STYPE_INT_SUPPORTED,
  MS_TWEAKS_BACKEND_SYSFS_ERROR_NO_KEY,
  MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_BE_ABSOLUTE,
  MS_TWEAKS_BACKEND_SYSFS_PATH_MUST_HAVE_SYSFS_PREFIX,
};

#define MS_TYPE_TWEAKS_BACKEND_SYSFS ms_tweaks_backend_sysfs_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksBackendSysfs, ms_tweaks_backend_sysfs, MS, TWEAKS_BACKEND_SYSFS, GObject)

GQuark ms_tweaks_backend_sysfs_error_quark (void);
#define MS_TWEAKS_BACKEND_SYSFS_ERROR ms_tweaks_backend_sysfs_error_quark ()

MsTweaksBackend *ms_tweaks_backend_sysfs_new (MsTweaksSetting *setting_data);

G_END_DECLS

