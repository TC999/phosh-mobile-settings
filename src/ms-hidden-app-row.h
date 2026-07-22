/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_HIDDEN_APP_ROW (ms_hidden_app_row_get_type ())

G_DECLARE_FINAL_TYPE (MsHiddenAppRow, ms_hidden_app_row, MS, HIDDEN_APP_ROW, AdwComboRow)

const char     *ms_hidden_app_row_get_filename (MsHiddenAppRow *self);
gboolean        ms_hidden_app_row_get_selected (MsHiddenAppRow *self);

G_END_DECLS
