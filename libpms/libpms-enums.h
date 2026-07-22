/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  MS_OS_UPDATE_STATE_FAILED = -1,
  MS_OS_UPDATE_STATE_UNKNOWN = 0,
  MS_OS_UPDATE_STATE_READY = 1,
  MS_OS_UPDATE_STATE_FETCHING = 2,
  MS_OS_UPDATE_STATE_FETCHED = 3,
  MS_OS_UPDATE_STATE_INSTALLING = 4,
  MS_OS_UPDATE_STATE_INSTALLED = 5,
} MsOsUpdateState;

G_END_DECLS
