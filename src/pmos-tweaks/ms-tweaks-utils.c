/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-utils"

#include "ms-tweaks-utils.h"


[[gnu::format (gnu_printf, 4, 5)]]
void
ms_tweaks_log (const char *restrict log_domain,
               GLogLevelFlags       log_level,
               const char *restrict name,
               const char *restrict format,
               ...)
{
  va_list args;
  char *restrict format_with_prefix = g_strconcat ("[Setting \"", name, "\"] ", format, NULL);

  va_start (args, format);
  g_logv (log_domain, log_level, format_with_prefix, args);
  va_end (args);
  g_free (format_with_prefix);
}
