/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "ms-tweaks-parser.h"

G_BEGIN_DECLS

enum {
  MS_TWEAKS_MAPPINGS_ERROR_FAILED_TO_FIND_KEY_BY_VALUE,
};

GQuark ms_tweaks_mappings_error_quark (void);
#define MS_TWEAKS_MAPPINGS_ERROR ms_tweaks_mappings_error_quark ()

gboolean ms_tweaks_mappings_handle_get (GValue *from,
                                        const MsTweaksSetting *setting_data,
                                        GError **error);
void ms_tweaks_mappings_handle_set (GValue *value, const MsTweaksSetting *setting_data);

G_END_DECLS
