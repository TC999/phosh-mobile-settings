/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_PLUGIN_LIST_BOX (ms_plugin_list_box_get_type ())

G_DECLARE_FINAL_TYPE (MsPluginListBox, ms_plugin_list_box, MS, PLUGIN_LIST_BOX, AdwBin)

MsPluginListBox *ms_plugin_list_box_new (void);
void             ms_plugin_list_box_open_plugin_prefs (MsPluginListBox *self, const char *plugin_name);

G_END_DECLS
