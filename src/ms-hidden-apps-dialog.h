/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_HIDDEN_APPS_DIALOG (ms_hidden_apps_dialog_get_type ())

G_DECLARE_FINAL_TYPE (MsHiddenAppsDialog, ms_hidden_apps_dialog, MS, HIDDEN_APPS_DIALOG, AdwDialog)

MsHiddenAppsDialog *ms_hidden_apps_dialog_new (void);

G_END_DECLS
