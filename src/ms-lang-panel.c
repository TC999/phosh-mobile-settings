/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Guido Günther <agx@sigxcpu.org>
 *
 * Based on g-c-c's 'cc-region-page.c' which is:
 * Copyright (C) 2010 Intel, Inc
 */

#define G_LOG_DOMAIN "ms-lang-panel"

#include "mobile-settings-config.h"
#include "ms-lang-panel.h"
#include "ms-util.h"

#include "libpms.h"

#include <gmobile.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-languages.h>

#include <act/act.h>
#include <polkit/polkit.h>

#include <glib/gi18n.h>

#include <locale.h>

#define DEFAULT_LOCALE "en_US.UTF-8"

struct _MsLangPanel {
  MsPanel            parent;

  AdwBanner         *restart_banner;
  AdwActionRow      *lang_row;
  char              *lang;
  char              *system_lang;
  char              *system_region;

  MsLanguageChooser *lang_chooser;

  ActUserManager    *user_manager;
  ActUser           *user;

  GCancellable      *cancel;
  GDBusProxy        *localed;
  GPermission       *localed_perm;

  AdwToastOverlay   *toast_overlay;
};

G_DEFINE_TYPE (MsLangPanel, ms_lang_panel, MS_TYPE_PANEL)


static void
on_logout_button_clicked (MsLangPanel *self)
{
  ms_util_end_session (MS_END_SESSION_MODE_LOGOUT);
}


static void
on_set_locale_ready (GObject* source_object, GAsyncResult* res, gpointer data)
{
  MsLangPanel *self = MS_LANG_PANEL (data);
  g_autoptr (GVariant) v = NULL;
  g_autoptr (GError) err = NULL;

  v = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &err);
  if (!v) {
    AdwToast *toast;

    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      return;

    g_warning ("Failed to set locale in localed: %s", err->message);
    toast = adw_toast_new (_("Failed to set system locale"));
    adw_toast_overlay_add_toast (self->toast_overlay, toast);
    return;
  }

  g_debug ("Locale set via localed");
}


static void
on_localed_properties_changed (MsLangPanel *self,
                               GVariant    *changed_properties,
                               const char **invalidated_properties,
                               GDBusProxy  *localed_proxy)
{
  g_autoptr (GVariant) v = NULL;
  /* We own the container but not the data */
  g_autofree const char **strv = NULL;
  const char *lang = NULL, *messages = NULL, *time = NULL;

  v = g_dbus_proxy_get_cached_property (localed_proxy, "Locale");
  if (!v)
    return;

  strv = g_variant_get_strv (v, NULL);
  for (int i = 0; strv[i]; i++) {
    if (g_str_has_prefix (strv[i], "LANG="))
      lang = strv[i] + strlen ("LANG=");
    else if (g_str_has_prefix (strv[i], "LC_MESSAGES="))
      messages = strv[i] + strlen ("LC_MESSAGES=");
    else if (g_str_has_prefix (strv[i], "LC_TIME="))
      time = strv[i] + strlen ("LC_TIME=");
  }

  if (!messages)
    messages = lang;

  g_set_str (&self->system_lang, messages);
  g_set_str (&self->system_region, time);

  g_debug ("System lang: %s, region: %s", self->system_lang, self->system_region);
}


static void
on_localed_proxy_ready (GObject* source_object, GAsyncResult* res, gpointer data)
{
  MsLangPanel *self;
  GDBusProxy *proxy;
  g_autoptr (GError) err = NULL;

  proxy = g_dbus_proxy_new_finish (res, &err);
  if (!proxy) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      g_warning ("Failed get localed proxy: %s", err->message);
    return;
  }

  self = MS_LANG_PANEL (data);
  self->localed = proxy;
  g_signal_connect_object (self->localed,
                           "g-properties-changed",
                           G_CALLBACK (on_localed_properties_changed),
                           self,
                           G_CONNECT_SWAPPED);

  on_localed_properties_changed (self, NULL, NULL, self->localed);
}


static void
ms_lang_panel_set_localed_locale (MsLangPanel *self)
{
  g_autoptr (GVariantBuilder) builder = NULL;
  g_autofree char *lang_category = NULL;
  g_autofree char *time_category = NULL;
  g_autofree char *numeric_category = NULL;
  g_autofree char *monetary_category = NULL;
  g_autofree char *measurement_category = NULL;
  g_autofree char *paper_category = NULL;
  const char *region = self->lang;

  g_return_if_fail (self->lang);
  g_return_if_fail (self->localed);

  if (!self->localed_perm)
    return;

  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

  lang_category = g_strdup_printf ("LANG=%s", self->lang);
  g_variant_builder_add (builder, "s", lang_category);

  time_category = g_strdup_printf ("LC_TIME=%s", region);
  g_variant_builder_add (builder, "s", time_category);

  numeric_category = g_strdup_printf ("LC_NUMERIC=%s", region);
  g_variant_builder_add (builder, "s", numeric_category);

  monetary_category = g_strdup_printf ("LC_MONETARY=%s", region);
  g_variant_builder_add (builder, "s", monetary_category);

  measurement_category = g_strdup_printf ("LC_MEASUREMENT=%s", region);
  g_variant_builder_add (builder, "s", measurement_category);

  paper_category = g_strdup_printf ("LC_PAPER=%s", region);
  g_variant_builder_add (builder, "s", paper_category);

  g_dbus_proxy_call (self->localed,
                     "SetLocale",
                     g_variant_new ("(asb)", builder, TRUE),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     self->cancel,
                     on_set_locale_ready,
                     self);
}


static void
on_acquire_set_locale_perm_ready (GObject *source, GAsyncResult *res, gpointer user_data)
{
  MsLangPanel *self = MS_LANG_PANEL (user_data);
  g_autoptr (GError) err = NULL;

  if (!g_permission_acquire_finish (G_PERMISSION (source), res, &err)) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      g_warning ("Failed to acquire set-locale permission: %s", err->message);
    return;
  }

  ms_lang_panel_set_localed_locale  (self);
}


static void
ms_lang_panel_set_localed_locale_with_perm (MsLangPanel *self)
{
  if (!self->localed_perm)
    return;

  if (g_permission_get_allowed (self->localed_perm)) {
    g_debug ("Have set-locale permission");
    ms_lang_panel_set_localed_locale  (self);
    return;
  }

  if (!g_permission_get_can_acquire (self->localed_perm)) {
    g_warning ("Can't acquire set-locale permission");
    return;
  }

  g_debug ("Acquiring set-locale perm");
  g_permission_acquire_async (self->localed_perm,
                              self->cancel,
                              on_acquire_set_locale_perm_ready,
                              self);
}


static void
ms_lang_panel_update_lang_row (MsLangPanel *self)
{
  g_autofree char *name = NULL;

  if (self->lang)
    name = ms_lang_get_language_from_locale (self->lang, self->lang);

  if (!name)
    name = ms_lang_get_language_from_locale (DEFAULT_LOCALE, DEFAULT_LOCALE);

  adw_action_row_set_subtitle (self->lang_row, name);
}


static void
ms_lang_panel_set_accountservice_langs (MsLangPanel *self)
{
  const char *languages[] = { self->lang, NULL };

  g_return_if_fail (self->user);

  act_user_set_languages (self->user, languages);
}


static void
ms_lang_panel_update_lang (MsLangPanel *self, const char *lang)
{
  if (!g_set_str (&self->lang, lang))
    return;

  ms_lang_panel_set_accountservice_langs (self);
  /* TODO: Skip this for multiuser setups */
  ms_lang_panel_set_localed_locale_with_perm (self);

  ms_lang_panel_update_lang_row (self);
  adw_banner_set_revealed (self->restart_banner, TRUE);
}


static void
on_language_selected (MsLangPanel *self)
{
  const char *language;

  language = ms_language_chooser_get_language (self->lang_chooser);
  ms_lang_panel_update_lang (self, language);
  g_debug ("Selected lang: %s", language);

  adw_dialog_close (ADW_DIALOG (self->lang_chooser));
  self->lang_chooser = NULL;
}


static void
on_lang_button_clicked (MsLangPanel *self)
{
  self->lang_chooser = ms_language_chooser_new ();

  ms_language_chooser_set_language (self->lang_chooser, self->lang);
  g_signal_connect_object (self->lang_chooser, "language-selected",
                           G_CALLBACK (on_language_selected),
                           self,
                           G_CONNECT_SWAPPED);

  adw_dialog_present (ADW_DIALOG (self->lang_chooser), GTK_WIDGET (self));
}


static void
ms_lang_panel_get_user_lang (MsLangPanel *self)
{
  const char *language = NULL;

  if (!self->user)
    return;

  if (act_user_is_loaded (self->user))
    language = act_user_get_language (self->user);

  if (gm_str_is_null_or_empty (language))
    language = setlocale (LC_MESSAGES, NULL);

  g_set_str (&self->lang, language);
  ms_lang_panel_update_lang_row (self);
}


static void
ms_lang_panel_init_localed (MsLangPanel *self)
{
  g_autoptr (GDBusConnection) bus = NULL;
  g_autoptr (GError) err = NULL;

  self->localed_perm = polkit_permission_new_sync ("org.freedesktop.locale1.set-locale",
                                                   NULL,
                                                   NULL,
                                                   &err);
  if (self->localed_perm == NULL) {
    g_warning ("Could not get localed set-locale permission: %s", err->message);
    return;
  }

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                            NULL,
                            "org.freedesktop.locale1",
                            "/org/freedesktop/locale1",
                            "org.freedesktop.locale1",
                            self->cancel,
                            on_localed_proxy_ready,
                            self);
}


static void
ms_lang_panel_init_accountsservice (MsLangPanel *self)
{
  self->user_manager = act_user_manager_get_default ();
  self->user = act_user_manager_get_user_by_id (self->user_manager, getuid ());
  g_signal_connect_object (self->user,
                           "notify::language",
                           G_CALLBACK (ms_lang_panel_get_user_lang),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->user,
                           "notify::is-loaded",
                           G_CALLBACK (ms_lang_panel_get_user_lang),
                           self,
                           G_CONNECT_SWAPPED);
}


static void
ms_lang_panel_finalize (GObject *object)
{
  MsLangPanel *self = MS_LANG_PANEL (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);
  g_clear_object (&self->localed);
  g_clear_object (&self->localed_perm);

  if (self->user_manager) {
    g_signal_handlers_disconnect_by_data (self->user_manager, self);
    self->user_manager = NULL;
  }

  if (self->user) {
    g_signal_handlers_disconnect_by_data (self->user, self);
    self->user = NULL;
  }

  g_clear_pointer (&self->lang, g_free);
  g_clear_pointer (&self->system_lang, g_free);
  g_clear_pointer (&self->system_region, g_free);

  G_OBJECT_CLASS (ms_lang_panel_parent_class)->finalize (object);
}


static void
ms_lang_panel_class_init (MsLangPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_lang_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-lang-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsLangPanel, lang_row);
  gtk_widget_class_bind_template_child (widget_class, MsLangPanel, restart_banner);
  gtk_widget_class_bind_template_child (widget_class, MsLangPanel, toast_overlay);

  gtk_widget_class_bind_template_callback (widget_class, on_lang_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_logout_button_clicked);
}


static void
ms_lang_panel_init (MsLangPanel *self)
{
  g_autoptr (GDBusConnection) conn = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->cancel = g_cancellable_new ();

  /* Avoid warning in tests when there's no system bus */
  conn = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, NULL);
  if (conn) {
    ms_lang_panel_init_accountsservice (self);
    ms_lang_panel_init_localed (self);
  }

  ms_lang_panel_get_user_lang (self);
}


MsLangPanel *
ms_lang_panel_new (void)
{
  return MS_LANG_PANEL (g_object_new (MS_TYPE_LANG_PANEL, NULL));
}
