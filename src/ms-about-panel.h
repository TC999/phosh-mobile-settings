/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-panel.h"

G_BEGIN_DECLS

#define MS_TYPE_ABOUT_PANEL (ms_about_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsAboutPanel, ms_about_panel, MS, ABOUT_PANEL, MsPanel)

G_END_DECLS
