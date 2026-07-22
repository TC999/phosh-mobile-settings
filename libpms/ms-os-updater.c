/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-os-updater"

#include "mobile-settings-config.h"

#include "ms-os-update.h"
#include "ms-os-updater-priv.h"

/**
 * MsOsUpdater:
 *
 * Class performing OS updates
 */

enum {
  PROP_0,
  PROP_SUPPORTED,
  PROP_LATEST_UPDATE,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

typedef struct _MsOsUpdaterPrivate {
  MsOsUpdate *latest_update;
  gboolean    supported;
} MsOsUpdaterPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (MsOsUpdater, ms_os_updater, G_TYPE_OBJECT)


static void
ms_os_updater_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MsOsUpdater *self = MS_OS_UPDATER (object);
  MsOsUpdaterPrivate *priv = ms_os_updater_get_instance_private (self);

  switch (property_id) {
  case PROP_SUPPORTED:
    g_value_set_boolean (value, priv->supported);
    break;
  case PROP_LATEST_UPDATE:
    g_value_set_object (value, priv->latest_update);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
ms_os_updater_fetch_update_finish_default (MsOsUpdater *self, GAsyncResult *res, GError **error)
{
  g_assert (G_IS_TASK (res));
  g_assert (!error || !*error);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static gboolean
ms_os_updater_cancel_fetch_update_finish_default (MsOsUpdater  *self,
                                                  GAsyncResult *res,
                                                  GError      **error)
{
  g_assert (G_IS_TASK (res));
  g_assert (!error || !*error);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static gboolean
ms_os_updater_install_update_finish_default (MsOsUpdater *self, GAsyncResult *res, GError **error)
{
  g_assert (G_IS_TASK (res));
  g_assert (!error || !*error);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static gboolean
ms_os_updater_cancel_install_update_finish_default (MsOsUpdater  *self,
                                                    GAsyncResult *res,
                                                    GError      **error)
{
  g_assert (G_IS_TASK (res));
  g_assert (!error || !*error);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static void
ms_os_updater_class_init (MsOsUpdaterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MsOsUpdaterClass *os_updater_class = MS_OS_UPDATER_CLASS (klass);

  object_class->get_property = ms_os_updater_get_property;

  os_updater_class->fetch_update_finish = ms_os_updater_fetch_update_finish_default;
  os_updater_class->cancel_fetch_update_finish = ms_os_updater_cancel_fetch_update_finish_default;
  os_updater_class->install_update_finish = ms_os_updater_install_update_finish_default;
  os_updater_class->cancel_install_update_finish = ms_os_updater_cancel_install_update_finish_default;

  /**
   * MsOsUpdater:supported:
   *
   * Whether the system supports this service.
   */
  props[PROP_SUPPORTED] =
    g_param_spec_boolean ("supported", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * MsOsUpdater:latest-update:
   *
   * The latest update to install
   */
  props[PROP_LATEST_UPDATE] =
    g_param_spec_object ("latest-update", "", "",
                         MS_TYPE_OS_UPDATE,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
ms_os_updater_init (MsOsUpdater *self)
{
}


void
ms_os_updater_set_supported (MsOsUpdater *self, gboolean supported)
{
  MsOsUpdaterPrivate *priv = ms_os_updater_get_instance_private (self);

  g_return_if_fail (MS_IS_OS_UPDATER (self));

  if (priv->supported == supported)
    return;

  g_debug ("OS updates %s", supported ? "supported" : "unsupported");
  priv->supported = supported;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SUPPORTED]);
}


gboolean
ms_os_updater_get_supported (MsOsUpdater *self)
{
  MsOsUpdaterPrivate *priv = ms_os_updater_get_instance_private (self);

  g_return_val_if_fail (MS_IS_OS_UPDATER (self), FALSE);

  return priv->supported;
}


void
ms_os_updater_set_latest_update (MsOsUpdater *self, MsOsUpdate *update)
{
  MsOsUpdaterPrivate *priv = ms_os_updater_get_instance_private (self);

  g_return_if_fail (MS_IS_OS_UPDATER (self));

  if (priv->latest_update == update)
    return;

  g_set_object (&priv->latest_update, update);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LATEST_UPDATE]);
}

/**
 * ms_os_updater_get_latest_update:
 * @self: The updater
 *
 * Get the latest update. This update, when applied would update the system
 * to the most recent version.
 *
 * Returns:(transfer none): the update
 */
MsOsUpdate *
ms_os_updater_get_latest_update (MsOsUpdater *self)
{
  MsOsUpdaterPrivate *priv = ms_os_updater_get_instance_private (self);

  g_return_val_if_fail (MS_IS_OS_UPDATER (self), FALSE);

  return priv->latest_update;
}


void
ms_os_updater_fetch_update (MsOsUpdater        *self,
                            MsOsUpdate         *update,
                            MsOsUpdateFlags     flags,
                            GCancellable       *cancel,
                            GAsyncReadyCallback callback,
                            gpointer            userdata)

{
  g_return_if_fail (MS_IS_OS_UPDATER (self));
  g_return_if_fail (MS_IS_OS_UPDATE (update));

  MS_OS_UPDATER_GET_CLASS (self)->fetch_update (self, update, flags, cancel, callback, userdata);
}


gboolean
ms_os_updater_fetch_update_finish (MsOsUpdater *self, GAsyncResult *res, GError **error)
{
  g_assert (MS_IS_OS_UPDATER (self));

  return MS_OS_UPDATER_GET_CLASS (self)->fetch_update_finish (self, res, error);
}


void
ms_os_updater_cancel_fetch_update (MsOsUpdater        *self,
                                   MsOsUpdate         *update,
                                   MsOsUpdateFlags     flags,
                                   GCancellable       *cancel,
                                   GAsyncReadyCallback callback,
                                   gpointer            userdata)

{
  g_return_if_fail (MS_IS_OS_UPDATER (self));
  g_return_if_fail (MS_IS_OS_UPDATE (update));

  MS_OS_UPDATER_GET_CLASS (self)->cancel_fetch_update (self,
                                                       update,
                                                       flags,
                                                       cancel,
                                                       callback,
                                                       userdata);
}


gboolean
ms_os_updater_cancel_fetch_update_finish (MsOsUpdater *self, GAsyncResult *res, GError **error)
{
  g_assert (MS_IS_OS_UPDATER (self));

  return MS_OS_UPDATER_GET_CLASS (self)->fetch_update_finish (self, res, error);
}


void
ms_os_updater_install_update (MsOsUpdater        *self,
                              MsOsUpdate         *update,
                              MsOsUpdateFlags     flags,
                              GCancellable       *cancel,
                              GAsyncReadyCallback callback,
                              gpointer            userdata)

{
  g_return_if_fail (MS_IS_OS_UPDATER (self));
  g_return_if_fail (MS_IS_OS_UPDATE (update));

  MS_OS_UPDATER_GET_CLASS (self)->install_update (self, update, flags, cancel, callback, userdata);
}


gboolean
ms_os_updater_install_update_finish (MsOsUpdater *self, GAsyncResult *res, GError **error)
{
  g_assert (MS_IS_OS_UPDATER (self));

  return MS_OS_UPDATER_GET_CLASS (self)->install_update_finish (self, res, error);
}


void
ms_os_updater_cancel_install_update (MsOsUpdater        *self,
                                     MsOsUpdate         *update,
                                     MsOsUpdateFlags     flags,
                                     GCancellable       *cancel,
                                     GAsyncReadyCallback callback,
                                     gpointer            userdata)

{
  g_return_if_fail (MS_IS_OS_UPDATER (self));
  g_return_if_fail (MS_IS_OS_UPDATE (update));

  MS_OS_UPDATER_GET_CLASS (self)->cancel_install_update (self,
                                                         update,
                                                         flags,
                                                         cancel,
                                                         callback,
                                                         userdata);
}


gboolean
ms_os_updater_cancel_install_update_finish (MsOsUpdater *self, GAsyncResult *res, GError **error)
{
  g_assert (MS_IS_OS_UPDATER (self));

  return MS_OS_UPDATER_GET_CLASS (self)->install_update_finish (self, res, error);
}
