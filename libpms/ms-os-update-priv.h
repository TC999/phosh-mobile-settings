/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ms-os-update.h"

G_BEGIN_DECLS

MsOsUpdate *ms_os_update_new (const char *version);
void        ms_os_update_set_state (MsOsUpdate *self, MsOsUpdateState state);
void        ms_os_update_set_progress (MsOsUpdate *self, guint progress);

G_END_DECLS
