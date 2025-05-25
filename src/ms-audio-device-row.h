/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ms-audio-device.h"

#include <adwaita.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MS_TYPE_AUDIO_DEVICE_ROW (ms_audio_device_row_get_type ())

G_DECLARE_FINAL_TYPE (MsAudioDeviceRow, ms_audio_device_row, MS, AUDIO_DEVICE_ROW,
                      AdwPreferencesRow)

MsAudioDeviceRow      *ms_audio_device_row_new                   (MsAudioDevice    *audio_device);
MsAudioDevice         *ms_audio_device_row_get_audio_device      (MsAudioDeviceRow *self);

G_END_DECLS
