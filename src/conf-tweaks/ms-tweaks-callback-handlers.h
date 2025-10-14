/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "ms-tweaks-backend-interface.h"
#include "ms-tweaks-parser.h"

#include <adwaita.h>


/**
 * MsTweaksPreferencesPageFilePickerMeta:
 *
 * Yes, the name is ridiculously long. Improvements welcome.
 *
 * Used by file_widget_open_file_picker () in ms-tweaks-preferences-page.c, but has to be here in a
 * different header to avoid circular imports when also used in backends.
 */
typedef struct {
  MsTweaksBackend *backend_state;
  GtkWidget *file_picker_label;
  AdwToastOverlay *toast_overlay;
} MsTweaksPreferencesPageFilePickerMeta;


typedef struct {
  MsTweaksBackend *backend_state;
  AdwToastOverlay *toast_overlay;
} MsTweaksCallbackMeta;


void ms_tweaks_callback_handlers_type_boolean (AdwSwitchRow         *switch_row,
                                               GParamSpec           *unused,
                                               MsTweaksCallbackMeta *callback_meta);
void ms_tweaks_callback_handlers_type_choice (AdwComboRow          *combo_row,
                                              GParamSpec           *unused,
                                              MsTweaksCallbackMeta *callback_meta);
void ms_tweaks_callback_handlers_type_color (GtkColorDialogButton *widget,
                                             GParamSpec           *unused,
                                             MsTweaksCallbackMeta *callback_meta);
void ms_tweaks_callback_handlers_type_file (GObject      *source_object,
                                            GAsyncResult *result,
                                            gpointer      data);
void ms_tweaks_callback_handlers_type_font (GtkFontDialogButton  *widget,
                                            GParamSpec           *unused,
                                            MsTweaksCallbackMeta *callback_meta);
void ms_tweaks_callback_handlers_type_number (AdwSpinRow *spin_row,
                                              MsTweaksCallbackMeta *callback_meta);
