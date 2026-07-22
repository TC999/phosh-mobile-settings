/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ms-os-update.h"
#include <glib-object.h>

#include <gio/gio.h>

G_BEGIN_DECLS

#define MS_TYPE_OS_UPDATER (ms_os_updater_get_type ())

G_DECLARE_DERIVABLE_TYPE (MsOsUpdater, ms_os_updater, MS, OS_UPDATER, GObject)

typedef enum {
  MS_OS_UPDATE_FLAG_NONE = 0,
} MsOsUpdateFlags;

struct _MsOsUpdaterClass {
  GObjectClass parent_class;

  void (*fetch_update) (MsOsUpdater        *self,
                          MsOsUpdate         *update,
                          MsOsUpdateFlags     flags,
                          GCancellable       *cancel,
                          GAsyncReadyCallback callback,
                          gpointer            userdata);
  gboolean (*fetch_update_finish) (MsOsUpdater   *self,
                                   GAsyncResult  *res,
                                   GError       **error);

  void (*cancel_fetch_update) (MsOsUpdater        *self,
                               MsOsUpdate         *update,
                               MsOsUpdateFlags     flags,
                               GCancellable       *cancel,
                               GAsyncReadyCallback callback,
                               gpointer            userdata);
  gboolean (*cancel_fetch_update_finish) (MsOsUpdater   *self,
                                          GAsyncResult  *res,
                                          GError       **error);

  void (*install_update) (MsOsUpdater        *self,
                          MsOsUpdate         *update,
                          MsOsUpdateFlags     flags,
                          GCancellable       *cancel,
                          GAsyncReadyCallback callback,
                          gpointer            userdata);
  gboolean (*install_update_finish) (MsOsUpdater   *self,
                                     GAsyncResult  *res,
                                     GError       **error);

  void (*cancel_install_update) (MsOsUpdater        *self,
                                 MsOsUpdate         *update,
                                 MsOsUpdateFlags     flags,
                                 GCancellable       *cancel,
                                 GAsyncReadyCallback callback,
                                 gpointer            userdata);
  gboolean (*cancel_install_update_finish) (MsOsUpdater   *self,
                                            GAsyncResult  *res,
                                            GError       **error);

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

gboolean     ms_os_updater_get_supported (MsOsUpdater *self);
MsOsUpdate  *ms_os_updater_get_latest_update(MsOsUpdater *self);

void         ms_os_updater_fetch_update (MsOsUpdater          *self,
                                           MsOsUpdate         *update,
                                           MsOsUpdateFlags     flags,
                                           GCancellable       *cancel,
                                           GAsyncReadyCallback callback,
                                           gpointer            userdata);
gboolean     ms_os_updater_fetch_update_finish (MsOsUpdater  *self,
                                                GAsyncResult *res,
                                                GError      **error);

void         ms_os_updater_cancel_fetch_update (MsOsUpdater        *self,
                                                MsOsUpdate         *update,
                                                MsOsUpdateFlags     flags,
                                                GCancellable       *cancel,
                                                GAsyncReadyCallback callback,
                                                gpointer            userdata);
gboolean     ms_os_updater_cancel_fetch_update_finish (MsOsUpdater  *self,
                                                       GAsyncResult *res,
                                                       GError      **error);

void         ms_os_updater_install_update (MsOsUpdater        *self,
                                           MsOsUpdate         *update,
                                           MsOsUpdateFlags     flags,
                                           GCancellable       *cancel,
                                           GAsyncReadyCallback callback,
                                           gpointer            userdata);
gboolean     ms_os_updater_install_update_finish (MsOsUpdater  *self,
                                                  GAsyncResult *res,
                                                  GError      **error);

void         ms_os_updater_cancel_install_update (MsOsUpdater        *self,
                                                  MsOsUpdate         *update,
                                                  MsOsUpdateFlags     flags,
                                                  GCancellable       *cancel,
                                                  GAsyncReadyCallback callback,
                                                  gpointer            userdata);
gboolean     ms_os_updater_cancel_install_update_finish (MsOsUpdater  *self,
                                                         GAsyncResult *res,
                                                         GError      **error);

G_END_DECLS
