/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-panel.h"

G_BEGIN_DECLS

#define MS_TYPE_WELCOME_PANEL (ms_welcome_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsWelcomePanel, ms_welcome_panel, MS, WELCOME_PANEL, MsPanel)

G_END_DECLS
