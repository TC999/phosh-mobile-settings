/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "libpms-enums.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_OS_UPDATE (ms_os_update_get_type ())

G_DECLARE_DERIVABLE_TYPE (MsOsUpdate, ms_os_update, MS, OS_UPDATE, GObject)

struct _MsOsUpdateClass {
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ms_reserved1) (void);
  void (*_ms_reserved2) (void);
  void (*_ms_reserved3) (void);
  void (*_ms_reserved4) (void);
  void (*_ms_reserved5) (void);
  void (*_ms_reserved6) (void);
  void (*_ms_reserved7) (void);
  void (*_ms_reserved8) (void);
};

const char      *ms_os_update_get_version (MsOsUpdate *self);
MsOsUpdateState  ms_os_update_get_state (MsOsUpdate *self);
guint            ms_os_update_get_progress (MsOsUpdate *self);

G_END_DECLS
