/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2020 Canonical Ltd.
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
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MS_TYPE_LANGUAGE_ROW (ms_language_row_get_type ())
G_DECLARE_FINAL_TYPE (MsLanguageRow, ms_language_row, MS, LANGUAGE_ROW, GtkListBoxRow)

MsLanguageRow *ms_language_row_new                (const gchar *locale_id);

const gchar   *ms_language_row_get_locale_id      (MsLanguageRow *row);

const gchar   *ms_language_row_get_language       (MsLanguageRow *row);

const gchar   *ms_language_row_get_language_local (MsLanguageRow *row);

const gchar   *ms_language_row_get_country        (MsLanguageRow *row);

const gchar   *ms_language_row_get_country_local  (MsLanguageRow *row);

void           ms_language_row_set_checked        (MsLanguageRow *row, gboolean checked);

void           ms_language_row_set_is_extra       (MsLanguageRow *row, gboolean is_extra);

gboolean       ms_language_row_get_is_extra       (MsLanguageRow *row);

G_END_DECLS
