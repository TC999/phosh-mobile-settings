/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-lang"

#include "ms-lang.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-languages.h>

/**
 * ms_lang_get_language_from_locale:
 * @locale: a locale string
 * @translation: (nullable): a locale string
 *
 * Gets the language description for @locale. If @translation is
 * provided the returned string is translated accordingly.
 *
 * This just wraps gnome_get_language_from_locale().
 *
 * Return value: (transfer full): the language description. Caller
 * takes ownership.
 */
char *
ms_lang_get_language_from_locale (const char *locale, const char *translation)
{
  return gnome_get_language_from_locale (locale, translation);
}

/**
 * ms_lang_get_country_from_locale:
 * @locale: a locale string
 * @translation: (nullable): a locale string
 *
 * Gets the country description for @locale. If @translation is
 * provided the returned string is translated accordingly.
 *
 * This just wraps gnome_get_country_from_locale().
 *
 * Return value: (transfer full): the country description. Caller
 * takes ownership.
 */
char *
ms_lang_get_country_from_locale  (const char *locale, const char *translation)
{
  return gnome_get_country_from_locale (locale, translation);
}
