/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-audio-devices"

#include "mobile-settings-config.h"

#include "ms-audio-device.h"
#include "ms-audio-devices.h"

#include "gvc-mixer-control.h"

#include <gmobile.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

/**
 * MsAudioDevices:
 *
 * The currently available audio devices as a list model. These are
 * not real hardware but rather Pipewire looback's with media roles.
 */

enum {
  PROP_0,
  PROP_IS_INPUT,
  PROP_MIXER_CONTROL,
  PROP_HAS_DEVICES,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MsAudioDevices {
  GObject          parent;

  GListStore      *devices;
  gboolean         is_input;
  gboolean         has_devices;
  GvcMixerControl *mixer_control;
};

static void ms_list_model_iface_init (GListModelInterface *iface);
G_DEFINE_TYPE_WITH_CODE (MsAudioDevices, ms_audio_devices, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, ms_list_model_iface_init))


static void
ms_audio_devices_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MsAudioDevices *self = MS_AUDIO_DEVICES (object);

  switch (property_id) {
  case PROP_IS_INPUT:
    self->is_input = g_value_get_boolean (value);
    break;
  case PROP_MIXER_CONTROL:
    self->mixer_control = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_audio_devices_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MsAudioDevices *self = MS_AUDIO_DEVICES (object);

  switch (property_id) {
  case PROP_IS_INPUT:
    g_value_set_boolean (value, self->is_input);
    break;
  case PROP_MIXER_CONTROL:
    g_value_set_object (value, self->mixer_control);
    break;
  case PROP_HAS_DEVICES:
    g_value_set_boolean (value, self->has_devices);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static GType
ms_list_model_get_item_type (GListModel *list)
{
  return MS_TYPE_AUDIO_DEVICE;
}


static gpointer
ms_list_model_get_item (GListModel *list, guint position)
{
  MsAudioDevices *self = MS_AUDIO_DEVICES (list);

  return g_list_model_get_item (G_LIST_MODEL (self->devices), position);
}


static unsigned int
ms_list_model_get_n_items (GListModel *list)
{
  MsAudioDevices *self = MS_AUDIO_DEVICES (list);

  return g_list_model_get_n_items (G_LIST_MODEL (self->devices));
}


static void
ms_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = ms_list_model_get_item_type;
  iface->get_item = ms_list_model_get_item;
  iface->get_n_items = ms_list_model_get_n_items;
}


static void
on_device_added (MsAudioDevices *self, guint id)
{
  GvcMixerUIDevice *device = NULL;
  GvcMixerStream *stream = NULL;
  const char *description = NULL;
  const char *icon_name;
  const char *origin;
  const char *name;
  g_autoptr (MsAudioDevice) audio_device = NULL;
  guint stream_id;

  if (self->is_input)
    device = gvc_mixer_control_lookup_input_id (self->mixer_control, id);
  else
    device = gvc_mixer_control_lookup_output_id (self->mixer_control, id);

  if (device == NULL) {
    g_debug ("No device for id %u", id);
    return;
  }

  /* Our loopback devices have empty origin so ignore all others */
  origin = gvc_mixer_ui_device_get_origin (device);
  if (!gm_str_is_null_or_empty (origin))
    return;

  stream_id = gvc_mixer_ui_device_get_stream_id (device);
  stream = gvc_mixer_control_lookup_stream_id (self->mixer_control, stream_id);
  if (!stream) {
    g_debug ("No stream for id %u", stream_id);
    return;
  }

  /* Only list the loopback sinks */
  name = gvc_mixer_stream_get_name (stream);
  if (!g_str_has_prefix (name, "input.loopback.sink.role."))
    return;

  icon_name = gvc_mixer_stream_get_icon_name (stream);
  if (!icon_name)
    icon_name = "audio-speakers-symbolic";

  if (g_str_equal (name, "input.loopback.sink.role.multimedia")) {
    description = g_strdup (_("Media Volume"));
  } else if (g_str_equal (name, "input.loopback.sink.role.notification")) {
    description = g_strdup (_("Notification Volume"));
  } else if (g_str_equal (name, "input.loopback.sink.role.ringing")) {
    description = g_strdup (_("Ring Tone Volume"));
  } else if (g_str_equal (name, "input.loopback.sink.role.call")) {
    description = g_strdup (_("Call volume"));
  } else if (g_str_equal (name, "input.loopback.sink.role.alarm")) {
    description = g_strdup (_("Alarm Volume"));
  } else if (g_str_equal (name, "input.loopback.sink.role.alert")) {
    description = g_strdup (_("Emergency Alerts Volume"));
  } else {
    g_warning ("Unknown stream name '%s'", name);
    description = g_strdup (gvc_mixer_ui_device_get_description (device));
  }

  g_debug ("Adding audio device %d: %s", id, description);

  audio_device = ms_audio_device_new (id, stream, icon_name, description);
  g_list_store_append (self->devices, audio_device);
}


static void
on_device_removed (MsAudioDevices *self, guint id)
{
  g_debug ("Removing audio device %d", id);
  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->devices)); i++) {
    g_autoptr (MsAudioDevice) device = g_list_model_get_item (G_LIST_MODEL (self->devices), i);

    if (id == ms_audio_device_get_id (device)) {
      g_list_store_remove (self->devices, i);
      return;
    }
  }
  g_debug ("Device %u not present, can't remove", id);
}


static void
ms_audio_devices_constructed (GObject *object)
{
  MsAudioDevices *self = MS_AUDIO_DEVICES (object);

  /* Listen for new devices now that we have a mixer and know whether we handle input or output */
  g_assert (GVC_IS_MIXER_CONTROL (self->mixer_control));

  if (self->is_input) {
    g_object_connect (self->mixer_control,
                      "swapped-object-signal::input-added", on_device_added, self,
                      "swapped-object-signal::input-removed", on_device_removed, self,
                      NULL);
  } else {
    g_object_connect (self->mixer_control,
                      "swapped-object-signal::output-added", on_device_added, self,
                      "swapped-object-signal::output-removed", on_device_removed, self,
                      NULL);
  }

  G_OBJECT_CLASS (ms_audio_devices_parent_class)->constructed (object);
}


static void
ms_audio_devices_dispose (GObject *object)
{
  MsAudioDevices *self = MS_AUDIO_DEVICES (object);

  if (self->mixer_control)
    g_signal_handlers_disconnect_by_data (self->mixer_control, self);
  g_clear_object (&self->mixer_control);

  G_OBJECT_CLASS (ms_audio_devices_parent_class)->dispose (object);
}


static void
ms_audio_devices_class_init (MsAudioDevicesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ms_audio_devices_get_property;
  object_class->set_property = ms_audio_devices_set_property;
  object_class->constructed = ms_audio_devices_constructed;
  object_class->dispose = ms_audio_devices_dispose;

  /**
   * MsAudioDevices:is-input:
   *
   * %TRUE Whether this list model stores input devices, %FALSE for output
   * devices.
   */
  props[PROP_IS_INPUT] =
    g_param_spec_boolean ("is-input", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  /**
   * MsAudioDevices:mixer-control:
   *
   * %TRUE Whether this list model stores input devices, %FALSE for output
   * devices.
   */
  props[PROP_MIXER_CONTROL] =
    g_param_spec_object ("mixer-control", "", "",
                         GVC_TYPE_MIXER_CONTROL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  /**
   * MsAudioDevices:has-devices:
   *
   * %TRUE when there's at least on audio device present
   */
  props[PROP_HAS_DEVICES] =
    g_param_spec_boolean ("has-devices", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
on_items_changed (MsAudioDevices *self,
                  guint           position,
                  guint           removed,
                  guint           added,
                  GListModel     *list)
{
  gboolean has_devices;
  g_autoptr (MsAudioDevice) device = NULL;

  g_return_if_fail (MS_IS_AUDIO_DEVICES (self));

  device = g_list_model_get_item (list, 0);
  has_devices = !!device;

  if (self->has_devices != has_devices) {
    self->has_devices = has_devices;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HAS_DEVICES]);
  }

  g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);
}


static void
ms_audio_devices_init (MsAudioDevices *self)
{
  self->devices = g_list_store_new (MS_TYPE_AUDIO_DEVICE);

  g_signal_connect_swapped (self->devices, "items-changed", G_CALLBACK (on_items_changed), self);
}

/**
 * ms_audio_devices_new:
 * @mixer_control: A new GvcMixerControl
 * @is_input: Whether this is an input
 *
 * Gets a new audio devices object which exposes the currently known
 * input or output devices as a list model.
 */
MsAudioDevices *
ms_audio_devices_new (GvcMixerControl *mixer_control, gboolean is_input)
{
  return g_object_new (MS_TYPE_AUDIO_DEVICES,
                       "mixer-control", mixer_control,
                       "is-input", is_input,
                       NULL);
}
