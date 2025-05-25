/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "ms-audio-device"

#include "mobile-settings-config.h"

#include "ms-audio-device.h"

/**
 * MsAudioDevice:
 *
 * Audio device information stored in [class@AudioDevices].
 */

enum {
  PROP_0,
  PROP_ID,
  PROP_STREAM,
  PROP_ICON_NAME,
  PROP_DESCRIPTION,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsAudioDevice {
  GObject         parent;

  guint           id;
  GvcMixerStream *stream;
  char           *icon_name;
  char           *description;
};
G_DEFINE_TYPE (MsAudioDevice, ms_audio_device, G_TYPE_OBJECT)


static void
ms_audio_device_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MsAudioDevice *self = MS_AUDIO_DEVICE (object);

  switch (property_id) {
  case PROP_ID:
    self->id = g_value_get_uint (value);
    break;
  case PROP_STREAM:
    self->stream = g_value_dup_object (value);
    break;
  case PROP_ICON_NAME:
    self->icon_name = g_value_dup_string (value);
    break;
  case PROP_DESCRIPTION:
    self->description = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_audio_device_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MsAudioDevice *self = MS_AUDIO_DEVICE (object);

  switch (property_id) {
  case PROP_ID:
    g_value_set_uint (value, self->id);
    break;
  case PROP_STREAM:
    g_value_set_object (value, self->stream);
    break;
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_DESCRIPTION:
    g_value_set_string (value, self->description);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_audio_device_finalize (GObject *object)
{
  MsAudioDevice *self = MS_AUDIO_DEVICE (object);

  g_clear_pointer (&self->icon_name, g_free);
  g_clear_pointer (&self->description, g_free);

  G_OBJECT_CLASS (ms_audio_device_parent_class)->finalize (object);
}


static void
ms_audio_device_class_init (MsAudioDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_audio_device_get_property;
  object_class->set_property = ms_audio_device_set_property;
  object_class->finalize = ms_audio_device_finalize;

  props[PROP_ID] =
    g_param_spec_uint ("id", "", "",
                       0, G_MAXUINT, G_MAXUINT,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_STREAM] =
    g_param_spec_object ("stream", "", "",
                         GVC_TYPE_MIXER_STREAM,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_DESCRIPTION] =
    g_param_spec_string ("description", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
ms_audio_device_init (MsAudioDevice *self)
{
}


MsAudioDevice *
ms_audio_device_new (guint           id,
                     GvcMixerStream *stream,
                     const char     *icon_name,
                     const char     *description)
{
  return g_object_new (MS_TYPE_AUDIO_DEVICE,
                       "id", id,
                       "stream", stream,
                       "icon-name", icon_name,
                       "description", description,
                       NULL);
}


const char *
ms_audio_device_get_description (MsAudioDevice *self)
{
  g_return_val_if_fail (MS_IS_AUDIO_DEVICE (self), NULL);

  return self->description;
}


guint
ms_audio_device_get_id (MsAudioDevice *self)
{
  g_return_val_if_fail (MS_IS_AUDIO_DEVICE (self), 0);

  return self->id;
}


GvcMixerStream *
ms_audio_device_get_stream (MsAudioDevice *self)
{
  g_return_val_if_fail (MS_IS_AUDIO_DEVICE (self), 0);

  return self->stream;
}
