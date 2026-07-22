/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

char *ms_lang_get_language_from_locale (const char *locale,
                                        const char *translation);
char *ms_lang_get_country_from_locale  (const char *locale,
                                        const char *translation);

G_END_DECLS
