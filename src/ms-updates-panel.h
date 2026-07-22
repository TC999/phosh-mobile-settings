/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-panel.h"

G_BEGIN_DECLS

#define MS_TYPE_UPDATES_PANEL (ms_updates_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsUpdatesPanel, ms_updates_panel, MS, UPDATES_PANEL, MsPanel)

MsUpdatesPanel *ms_updates_panel_new (void);

G_END_DECLS
