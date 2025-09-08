/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include "../ms-tweaks-backend-interface.h"
#include "../ms-tweaks-parser.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Used for remapping integers to strings with the AdwComboRow widget. */
typedef struct {
  const MsTweaksSetting *setting_data;
  GtkStringList *choice_model;
} ChoiceBindMappingData;

#define MS_TYPE_TWEAKS_BACKEND_GSETTINGS ms_tweaks_backend_gsettings_get_type ()
G_DECLARE_FINAL_TYPE (MsTweaksBackendGsettings, ms_tweaks_backend_gsettings, MS, TWEAKS_BACKEND_GSETTINGS, GObject)

MsTweaksBackend *ms_tweaks_backend_gsettings_new (const MsTweaksSetting *const setting_data);

G_END_DECLS
