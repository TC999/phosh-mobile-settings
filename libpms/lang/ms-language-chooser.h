/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Written by: Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_LANGUAGE_CHOOSER (ms_language_chooser_get_type ())
G_DECLARE_FINAL_TYPE (MsLanguageChooser, ms_language_chooser, MS, LANGUAGE_CHOOSER, AdwDialog)

MsLanguageChooser *ms_language_chooser_new          (void);
void               ms_language_chooser_clear_filter (MsLanguageChooser *chooser);
const gchar       *ms_language_chooser_get_language (MsLanguageChooser *chooser);
void               ms_language_chooser_set_language (MsLanguageChooser *chooser,
                                                     const gchar       *language);

G_END_DECLS
