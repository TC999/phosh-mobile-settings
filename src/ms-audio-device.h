/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <pulse/pulseaudio.h>

#include "gvc-mixer-stream.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define MS_TYPE_AUDIO_DEVICE (ms_audio_device_get_type ())

G_DECLARE_FINAL_TYPE (MsAudioDevice, ms_audio_device, MS, AUDIO_DEVICE, GObject)

MsAudioDevice           *ms_audio_device_new             (guint           id,
                                                          GvcMixerStream *stream,
                                                          const char     *icon_name,
                                                          const char     *description);
guint                    ms_audio_device_get_id          (MsAudioDevice *self);
const char *             ms_audio_device_get_description (MsAudioDevice *self);
GvcMixerStream *         ms_audio_device_get_stream      (MsAudioDevice *self);

G_END_DECLS
