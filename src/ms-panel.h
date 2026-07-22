/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_PANEL (ms_panel_get_type ())

G_DECLARE_DERIVABLE_TYPE (MsPanel, ms_panel, MS, PANEL, AdwBin)

/**
 * MsPanelClass:
 * @parent_class: The parent class
 * @handle_options: Virtual function to handle options. Implementations
 *                  should override this to add their own logic for parsing
 *                  options. Returns TRUE if options were handled successfully,
 *                  FALSE otherwise.
 */
struct _MsPanelClass {
  AdwBinClass parent_class;

  /* Virtual function to handle options specific to implementing panel */
  gboolean (* handle_options) (MsPanel *self, GVariant *params);
};

GtkStringList *ms_panel_get_keywords (MsPanel *self);
void           ms_panel_set_keywords (MsPanel *self, GtkStringList *keywords);
gboolean       ms_panel_get_enabled (MsPanel *self);
void           ms_panel_set_enabled (MsPanel *self, gboolean enabled);
gboolean       ms_panel_handle_options (MsPanel *self, GVariant *params);

G_END_DECLS
