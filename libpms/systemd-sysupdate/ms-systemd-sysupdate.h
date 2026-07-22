/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ms-os-updater.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_SYSTEMD_SYSUPDATE (ms_systemd_sysupdate_get_type ())

G_DECLARE_FINAL_TYPE (MsSystemdSysupdate, ms_systemd_sysupdate, MS, SYSTEMD_SYSUPDATE, MsOsUpdater)

MsSystemdSysupdate *ms_systemd_sysupdate_new (void);

G_END_DECLS
