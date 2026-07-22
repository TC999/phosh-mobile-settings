/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#ifndef MS_TWEAKS_NO_LOCALE_FORMATTING
# error "This file is meant for locale-unaware conf-tweaks format handling only"
#endif

/* Tweaks files use C locale so make sure we don't accidentally use locale-aware functions. */
#pragma GCC poison g_strtod strtof strtod strtold
