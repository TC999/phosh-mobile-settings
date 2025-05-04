/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "mobile-settings-plugin.h"

#define DEVICE_TREE_COMPATIBLE_PATH "/sys/firmware/devicetree/base/compatible"

gboolean
ms_plugin_check_device_support (const char * const *supported)
{
  gsize len;
  const char *comp;
  g_autoptr (GError) err = NULL;
  g_autofree char *compatibles = NULL;
  const char *assume_device;

  assume_device = g_getenv("MOBILE_SETTINGS_ASSUME_DEVICE");
  g_debug ("Assuming device %s", assume_device);

  /* ALlow to override detected device for debugging */
  if (assume_device && g_strv_contains (supported, assume_device))
    return TRUE;

  if (g_file_test (DEVICE_TREE_COMPATIBLE_PATH, (G_FILE_TEST_EXISTS)) == FALSE)
    return FALSE;

  g_debug ("Found device tree device compatible at %s", DEVICE_TREE_COMPATIBLE_PATH);

  if (g_file_get_contents (DEVICE_TREE_COMPATIBLE_PATH, &compatibles, &len, &err) == FALSE) {
    g_warning ("Unable to read: %s", err->message);
    return FALSE;
  }

  comp = compatibles;
  while ((gsize)(comp - compatibles) < len) {
    if (g_strv_contains (supported, comp))
      return TRUE;

    /* Next compatible */
    comp = strchr (comp, 0);
    comp++;
  }

  return FALSE;
}
