/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "ms-tweaks-backend-interface.h"

#include "ms-tweaks-utils.h"


enum {
  SAVE_AS_ADMINISTRATOR,
  N_SIGNALS,
};
guint signals[N_SIGNALS];


G_DEFINE_INTERFACE (MsTweaksBackend, ms_tweaks_backend, G_TYPE_OBJECT)


static void
ms_tweaks_backend_default_init (MsTweaksBackendInterface *iface)
{
  signals[SAVE_AS_ADMINISTRATOR] =
    g_signal_new ("save-as-administrator",
                  MS_TYPE_TWEAKS_BACKEND,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_STRING, /* From. */
                  G_TYPE_STRING); /* To. */
}


GValue *
ms_tweaks_backend_get_value (MsTweaksBackend *self)
{
  g_assert (MS_IS_TWEAKS_BACKEND (self));
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (self)->get_value);

  return MS_TWEAKS_BACKEND_GET_IFACE (self)->get_value (self);
}


gboolean
ms_tweaks_backend_set_value (MsTweaksBackend *self, GValue *value, GError **error)
{
  g_assert (MS_IS_TWEAKS_BACKEND (self));
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (self)->set_value);

  return MS_TWEAKS_BACKEND_GET_IFACE (self)->set_value (self, value, error);
}


const MsTweaksSetting *
ms_tweaks_backend_get_setting_data (MsTweaksBackend *self)
{
  g_assert (MS_IS_TWEAKS_BACKEND (self));
  g_assert (MS_TWEAKS_BACKEND_GET_IFACE (self)->get_setting_data);

  return MS_TWEAKS_BACKEND_GET_IFACE (self)->get_setting_data (self);
}


const char *
ms_tweaks_backend_get_key (MsTweaksBackend *self)
{
  const MsTweaksSetting *setting_data = ms_tweaks_backend_get_setting_data (self);

  return ms_tweaks_util_get_single_key (setting_data->key);
}
