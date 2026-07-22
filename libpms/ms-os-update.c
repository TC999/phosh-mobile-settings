/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-os-update"

#include "mobile-settings-config.h"

#include "libpms-enums.h"
#include "libpms-enum-types.h"
#include "ms-os-update-priv.h"

/**
 * MsOsUpdate:
 *
 * An os update.
 */

enum {
  PROP_0,
  PROP_VERSION,
  PROP_STATE,
  PROP_PROGRESS,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

typedef struct _MsOsUpdatePrivate {
  GObject parent;

  MsOsUpdateState state;
  char   *version;
  guint   progress;
} MsOsUpdatePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (MsOsUpdate, ms_os_update, G_TYPE_OBJECT)


static void
ms_os_update_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MsOsUpdate *self = MS_OS_UPDATE (object);
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  switch (property_id) {
  case PROP_VERSION:
    priv->version = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_os_update_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MsOsUpdate *self = MS_OS_UPDATE (object);
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  switch (property_id) {
  case PROP_STATE:
    g_value_set_enum (value, priv->state);
    break;
  case PROP_VERSION:
    g_value_set_string (value, priv->version);
    break;
  case PROP_PROGRESS:
    g_value_set_uint (value, priv->progress);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_os_update_dispose (GObject *object)
{
  MsOsUpdate *self = MS_OS_UPDATE (object);
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  g_clear_pointer (&priv->version, g_free);

  G_OBJECT_CLASS (ms_os_update_parent_class)->dispose (object);
}


static void
ms_os_update_class_init (MsOsUpdateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ms_os_update_set_property;
  object_class->get_property = ms_os_update_get_property;
  object_class->dispose = ms_os_update_dispose;

  /**
   * MsOsUpdate::version:
   *
   * The OS version this update would update the system to
   */
  props[PROP_VERSION] =
    g_param_spec_string ("version", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * MsOsUpdate::state:
   *
   * The state of the update (e.g. if it is being currently installed).
   */
  props[PROP_STATE] =
    g_param_spec_enum ("state", "", "",
                       MS_TYPE_OS_UPDATE_STATE,
                       MS_OS_UPDATE_STATE_UNKNOWN,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * MsOsUpdate::progress:
   *
   * The progress of the currently executed step (e.g. fetch or install)
   */
  props[PROP_PROGRESS] =
    g_param_spec_uint ("progress", "", "",
                       0, 100, 0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
ms_os_update_init (MsOsUpdate *self)
{
}


MsOsUpdate *
ms_os_update_new (const char *version)
{
  return g_object_new (MS_TYPE_OS_UPDATE,
                       "version", version,
                       NULL);
}


const char *
ms_os_update_get_version (MsOsUpdate *self)
{
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  g_return_val_if_fail (MS_IS_OS_UPDATE (self), NULL);

  return priv->version;
}


void
ms_os_update_set_state (MsOsUpdate *self, MsOsUpdateState state)
{
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  g_return_if_fail (MS_IS_OS_UPDATE (self));

  if (priv->state == state)
    return;

  priv->state = state;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STATE]);
}


MsOsUpdateState
ms_os_update_get_state (MsOsUpdate *self)
{
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  g_return_val_if_fail (MS_IS_OS_UPDATE (self), MS_OS_UPDATE_STATE_UNKNOWN);

  return priv->state;
}


void
ms_os_update_set_progress (MsOsUpdate *self, guint progress)
{
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  g_return_if_fail (MS_IS_OS_UPDATE (self));

  if (priv->progress == progress)
    return;

  priv->progress = progress;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PROGRESS]);
}


guint
ms_os_update_get_progress (MsOsUpdate *self)
{
  MsOsUpdatePrivate *priv = ms_os_update_get_instance_private (self);

  g_return_val_if_fail (MS_IS_OS_UPDATE (self), 0);

  return priv->progress;
}
