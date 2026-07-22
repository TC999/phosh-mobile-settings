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

G_BEGIN_DECLS

enum {
  MS_TWEAKS_BACKEND_SYMLINK_ERROR_FAILED_TO_CREATE,
  MS_TWEAKS_BACKEND_SYMLINK_ERROR_FAILED_TO_CREATE_LEADING_DIRS,
  MS_TWEAKS_BACKEND_SYMLINK_ERROR_FAILED_TO_REMOVE,
  MS_TWEAKS_BACKEND_SYMLINK_ERROR_SYMLINK_NONEXISTENT,
  MS_TWEAKS_BACKEND_SYMLINK_ERROR_SYMLINK_OUTSIDE_HOME,
  MS_TWEAKS_BACKEND_SYMLINK_ERROR_WRONG_FILE_TYPE,
};

GQuark ms_tweaks_backend_symlink_error_quark (void);
#define MS_TWEAKS_BACKEND_SYMLINK_ERROR ms_tweaks_backend_symlink_error_quark ()

#define MS_TYPE_TWEAKS_BACKEND_SYMLINK ms_tweaks_backend_symlink_get_type ()
G_DECLARE_DERIVABLE_TYPE (MsTweaksBackendSymlink, ms_tweaks_backend_symlink, MS, TWEAKS_BACKEND_SYMLINK, GObject)

struct _MsTweaksBackendSymlinkClass {
  GObjectClass parent_class;
};

MsTweaksBackend *ms_tweaks_backend_symlink_new (const MsTweaksSetting *setting_data);

G_END_DECLS
