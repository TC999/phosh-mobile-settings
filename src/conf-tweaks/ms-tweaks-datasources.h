/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#pragma once

#include <glib.h>

GHashTable *ms_tweaks_datasources_get_map (const char *datasource_identifier_str);
