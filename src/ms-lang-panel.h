/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-panel.h"

G_BEGIN_DECLS

#define MS_TYPE_LANG_PANEL (ms_lang_panel_get_type ())

G_DECLARE_FINAL_TYPE (MsLangPanel, ms_lang_panel, MS, LANG_PANEL, MsPanel)

MsLangPanel *ms_lang_panel_new (void);

G_END_DECLS
