/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ms-os-updater.h"

G_BEGIN_DECLS

void ms_os_updater_set_supported (MsOsUpdater *self, gboolean supported);
void ms_os_updater_set_latest_update (MsOsUpdater *self, MsOsUpdate *update);

G_END_DECLS
