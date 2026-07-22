/*
 * Copyright (C) 2024-2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#if !defined(_LIBPMS_INSIDE) && !defined(LIBPMS_COMPILATION)
# error "Only <libpms.h> can be included directly."
#endif

#define MS_TYPE_OSK_ADD_LAYOUT_DIALOG (ms_osk_add_layout_dialog_get_type ())

G_DECLARE_FINAL_TYPE (MsOskAddLayoutDialog, ms_osk_add_layout_dialog, MS, OSK_ADD_LAYOUT_DIALOG,
                      AdwDialog)

GtkWidget *ms_osk_add_layout_dialog_new (GListModel *layouts);

G_END_DECLS
