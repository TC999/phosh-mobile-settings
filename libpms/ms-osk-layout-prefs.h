/*
 * Copyright (C) 2024-2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <adwaita.h>

#if !defined(_LIBPMS_INSIDE) && !defined(LIBPMS_COMPILATION)
# error "Only <libpms.h> can be included directly."
#endif

G_BEGIN_DECLS

#define MS_TYPE_OSK_LAYOUT_PREFS (ms_osk_layout_prefs_get_type ())

G_DECLARE_FINAL_TYPE (MsOskLayoutPrefs, ms_osk_layout_prefs, MS, OSK_LAYOUT_PREFS,
                      AdwPreferencesGroup)

MsOskLayoutPrefs *ms_osk_layout_prefs_new (void);
void              ms_osk_layout_prefs_load_osk_layouts (MsOskLayoutPrefs *self);
void              ms_osk_layout_prefs_load_osk_layouts_async (MsOskLayoutPrefs   *self,
                                                              GCancellable       *cancellable,
                                                              GAsyncReadyCallback callback,
                                                              gpointer            user_data);
gboolean          ms_osk_layout_prefs_load_osk_layouts_finish (MsOskLayoutPrefs *self,
                                                               GAsyncResult     *res,
                                                               GError          **error);
gboolean          ms_osk_layout_prefs_add_for_locale (MsOskLayoutPrefs *self,
                                                      const char       *locale,
                                                      const char       *flavor);

G_END_DECLS
