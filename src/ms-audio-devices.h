/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

#include <pulse/pulseaudio.h>
#include "gvc-mixer-control.h"

G_BEGIN_DECLS

#define MS_TYPE_AUDIO_DEVICES (ms_audio_devices_get_type ())

G_DECLARE_FINAL_TYPE (MsAudioDevices, ms_audio_devices, MS, AUDIO_DEVICES, GObject)

MsAudioDevices         *ms_audio_devices_new       (GvcMixerControl *mixer_control,
                                                    gboolean         is_input);

G_END_DECLS
