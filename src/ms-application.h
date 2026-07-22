/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "ms-head-tracker.h"
#include "ms-toplevel-tracker.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define MS_TYPE_APPLICATION (ms_application_get_type ())

G_DECLARE_FINAL_TYPE (MsApplication, ms_application, MS, APPLICATION, AdwApplication)

MsApplication *    ms_application_new                          (char *application_id);
GtkWidget *        ms_application_get_device_panel             (MsApplication *self);
MsToplevelTracker *ms_application_get_toplevel_tracker         (MsApplication *self);
MsHeadTracker *    ms_application_get_head_tracker             (MsApplication *self);
GStrv              ms_application_get_wayland_protocols        (MsApplication *self);
guint32            ms_application_get_wayland_protocol_version (MsApplication *self,
                                                                const char    *protocol);

G_END_DECLS
