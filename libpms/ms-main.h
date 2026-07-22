/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "ms-os-updater.h"

#include <glib.h>

G_BEGIN_DECLS

void ms_init (void);

MsOsUpdater *ms_get_default_os_updater_sync (void);

G_END_DECLS
