/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "ms-panel-switcher.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_WINDOW (ms_window_get_type ())

G_DECLARE_FINAL_TYPE (MsWindow, ms_window, MS, WINDOW, AdwApplicationWindow)

GListModel *     ms_window_get_stack_pages    (MsWindow *self);
MsPanelSwitcher *ms_window_get_panel_switcher (MsWindow *self);

G_END_DECLS
