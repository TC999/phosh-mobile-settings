/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#ifndef LIBPMS_USE_UNSTABLE_API
#error    libpms is unstable API. You must define LIBPMS_USE_UNSTABLE_API before including libpms.h
#endif

#define _LIBPMS_INSIDE

#include "libpms-enums.h"
#include "libpms-enum-types.h"

#include "ms-main.h"
#include "ms-lang.h"
#include "ms-os-update.h"
#include "ms-os-updater.h"
#include "ms-osk-layout-prefs.h"
#include "lang/ms-language-chooser.h"
