/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-updates-panel"

#include "mobile-settings-config.h"
#include "ms-updates-panel.h"
#include "ms-util.h"

#include "libpms.h"

#include <glib/gi18n.h>

struct _MsUpdatesPanel {
  MsPanel          parent;

  GtkStack        *stack;
  GtkStack        *update_stack;
  GtkButton       *start_update_btn;
  AdwStatusPage   *fetch_page;
  GtkProgressBar  *fetch_progress;
  AdwStatusPage   *install_page;
  GtkProgressBar  *install_progress;
  AdwToastOverlay *toast_overlay;
  GtkSvg          *download_svg;

  MsOsUpdater     *os_updater;
  MsOsUpdate      *os_update;
  AdwBanner       *reboot_banner;
  GCancellable    *cancel;

  guint inhibitor;
};

G_DEFINE_TYPE (MsUpdatesPanel, ms_updates_panel, MS_TYPE_PANEL)


static void
uninhibit (MsUpdatesPanel *self)
{
  GtkApplication *app = GTK_APPLICATION (g_application_get_default ());

  if (self->inhibitor == 0)
    return;

  gtk_application_uninhibit (app, self->inhibitor);
  self->inhibitor = 0;
}


static void
update_stack_page (MsUpdatesPanel *self)
{
  MsOsUpdateState state;
  const char *stack = "needsupdate";

  gtk_svg_pause (self->download_svg);

  if (self->os_update == NULL) {
    gtk_stack_set_visible_child_name (self->stack, "uptodate");
    return;
  }

  state = ms_os_update_get_state (self->os_update);
  g_debug ("OS update state %d", state);
  switch (state) {
  case MS_OS_UPDATE_STATE_FETCHING:
    stack = "fetching";
    gtk_svg_set_frame_clock (self->download_svg, gtk_widget_get_frame_clock (GTK_WIDGET (self)));
    gtk_svg_play (self->download_svg);
    break;
  case MS_OS_UPDATE_STATE_INSTALLING:
    stack = "installing";
    break;
  case MS_OS_UPDATE_STATE_READY:
    stack = "needsupdate";
    break;
  case MS_OS_UPDATE_STATE_FAILED:
  case MS_OS_UPDATE_STATE_FETCHED:
  case MS_OS_UPDATE_STATE_INSTALLED:
  case MS_OS_UPDATE_STATE_UNKNOWN:
  default:
    return;
  }

  gtk_stack_set_visible_child_name (self->stack, stack);
}


static void
on_update_state_changed (MsUpdatesPanel *self, GParamSpec *pspec, MsOsUpdate *update)
{
  update_stack_page (self);
}


static void
on_update_progress_changed (MsUpdatesPanel *self, GParamSpec *pspec, MsOsUpdate *update)
{
  GtkProgressBar *bar;
  guint progress = ms_os_update_get_progress (update);
  MsOsUpdateState state = ms_os_update_get_state (update);

  if (state == MS_OS_UPDATE_STATE_FETCHING)
    bar = self->fetch_progress;
  else if (state == MS_OS_UPDATE_STATE_INSTALLING)
    bar = self->install_progress;
  else
    g_return_if_reached ();

  gtk_progress_bar_set_fraction (bar, progress / 100.0);
}


static void
set_latest_os_update (MsUpdatesPanel *self, MsOsUpdate *update)
{
  g_autofree char *label = NULL;

  if (self->os_update == update)
    return;

  if (self->os_update)
    g_signal_handlers_disconnect_by_data (self->os_update, self);

  g_set_object (&self->os_update, update);
  update_stack_page (self);

  /* No updates available */
  if (!self->os_update)
    return;

  g_signal_connect_object (self->os_update,
                           "notify::state",
                           G_CALLBACK (on_update_state_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->os_update,
                           "notify::progress",
                           G_CALLBACK (on_update_progress_changed),
                           self,
                           G_CONNECT_SWAPPED);

  /* Translators: %s is a OS version number like 0.20260614 */
  label = g_strdup_printf (_("Update to %s"), ms_os_update_get_version (update));
  gtk_button_set_label (self->start_update_btn, label);
}


static void
on_reboot_button_clicked (MsUpdatesPanel *self)
{
  g_message ("Rebooting…");

  ms_util_end_session (MS_END_SESSION_MODE_REBOOT);
}


static void
on_os_updater_install_update_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  MsUpdatesPanel *self;
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = ms_os_updater_install_update_finish (MS_OS_UPDATER (source_object), res, &err);
  if (!success) {
    AdwToast *toast;
    g_autofree char *msg = NULL;

    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug ("OS Update cancelled");
      return;
    }

    self = MS_UPDATES_PANEL (user_data);
    g_warning ("Failed to update OS image: %s", err->message);
    msg = g_strdup_printf (_("OS update failed: %s"), err->message);
    toast = adw_toast_new (msg);
    adw_toast_overlay_add_toast (self->toast_overlay, toast);

    uninhibit (self);
    return;
  }

  self = MS_UPDATES_PANEL (user_data);
  g_debug ("Updates OS image");
  set_latest_os_update (self, NULL);
  adw_banner_set_revealed (self->reboot_banner, TRUE);
  uninhibit (self);
}


static void
on_os_updater_fetch_update_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  MsUpdatesPanel *self;
  MsOsUpdateState state;
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = ms_os_updater_install_update_finish (MS_OS_UPDATER (source_object), res, &err);
  if (!success) {
    AdwToast *toast;
    g_autofree char *msg = NULL;

    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug ("OS Update cancelled");
      return;
    }

    self = MS_UPDATES_PANEL (user_data);
    g_warning ("Failed to fetch OS image: %s", err->message);
    msg = g_strdup_printf (_("OS update failed: %s"), err->message);
    toast = adw_toast_new (msg);
    adw_toast_overlay_add_toast (self->toast_overlay, toast);

    uninhibit (self);
    return;
  }


  self = MS_UPDATES_PANEL (user_data);
  state = ms_os_update_get_state (self->os_update);
  /* Update not ready for install, e.g. cancelled */
  if (state != MS_OS_UPDATE_STATE_FETCHED) {
    g_debug ("Update not fetched (state %d), giving up.", state);

    uninhibit (self);
    return;
  }

  g_debug ("Fetched OS image");
  ms_os_updater_install_update (self->os_updater,
                                self->os_update,
                                MS_OS_UPDATE_FLAG_NONE,
                                self->cancel,
                                on_os_updater_install_update_ready,
                                self);
}


static void
on_os_updater_cancel_fetch_update_ready (GObject      *source_object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  MsUpdatesPanel *self;
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = ms_os_updater_cancel_fetch_update_finish (MS_OS_UPDATER (source_object), res, &err);
  if (!success) {
    AdwToast *toast;
    g_autofree char *msg = NULL;

    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_debug ("OS Update canceled");
      return;
    }

    self = MS_UPDATES_PANEL (user_data);
    g_warning ("Failed to cancel OS update: %s", err->message);
    msg = g_strdup_printf (_("Canceling OS update failed: %s"), err->message);
    toast = adw_toast_new (msg);
    adw_toast_overlay_add_toast (self->toast_overlay, toast);
    uninhibit (self);
    return;
  }

  g_debug ("Canceled fetch");
}


static void
on_cancel_update_activated (GtkWidget *widget,  const char* action_name, GVariant *parameter)
{
  MsUpdatesPanel *self = MS_UPDATES_PANEL (widget);
  MsOsUpdateState state;
  const char *what;

  g_return_if_fail (MS_IS_OS_UPDATE (self->os_update));
  state = ms_os_update_get_state (self->os_update);

  what = g_variant_get_string (parameter, NULL);
  g_debug ("Cancelling OS update '%s'", what);

  if (g_strcmp0 (what, "fetch") == 0) {
    g_return_if_fail (state == MS_OS_UPDATE_STATE_FETCHING);
    ms_os_updater_cancel_fetch_update (self->os_updater,
                                       self->os_update,
                                       MS_OS_UPDATE_FLAG_NONE,
                                       self->cancel,
                                       on_os_updater_cancel_fetch_update_ready,
                                       self);
  } else if (g_strcmp0 (what, "install") == 0) {
    g_return_if_fail (state == MS_OS_UPDATE_STATE_INSTALLING);
    ms_os_updater_cancel_install_update (self->os_updater,
                                         self->os_update,
                                         MS_OS_UPDATE_FLAG_NONE,
                                         self->cancel,
                                         on_os_updater_cancel_fetch_update_ready,
                                         self);
  } else {
    g_return_if_reached ();
  }
}


static void
on_update_activated (GtkWidget *widget,  const char* action_name, GVariant *parameter)
{
  GtkApplication *app = GTK_APPLICATION (g_application_get_default ());
  MsUpdatesPanel *self = MS_UPDATES_PANEL (widget);
  MsOsUpdateState state;
  GtkWindow *window;
  const char *version;
  g_autofree char *label = NULL;

  g_return_if_fail (MS_IS_OS_UPDATE (self->os_update));

  state = ms_os_update_get_state (self->os_update);
  if (state != MS_OS_UPDATE_STATE_READY && state != MS_OS_UPDATE_STATE_FAILED) {
    AdwToast *toast;

    g_warning ("OS Update already in progress");
    toast = adw_toast_new (_("OS Update already in progress"));
    adw_toast_overlay_add_toast (self->toast_overlay, toast);
    return;
  }

  version = ms_os_update_get_version (self->os_update);
  if (version) {
    /* Translators %s is an os version number like 0.20260614 */
    label = g_strdup_printf (_("Updating to %s…"), version);
  } else {
    label = g_strdup ("");
  }
  adw_status_page_set_description (self->fetch_page, label);
  adw_status_page_set_description (self->install_page, label);

  ms_os_updater_fetch_update (self->os_updater,
                              self->os_update,
                              MS_OS_UPDATE_FLAG_NONE,
                              self->cancel,
                              on_os_updater_fetch_update_ready,
                              self);

  window = GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self)));
  self->inhibitor = gtk_application_inhibit (app,
                                             window,
                                             (GTK_APPLICATION_INHIBIT_LOGOUT |
                                              GTK_APPLICATION_INHIBIT_SWITCH |
                                              GTK_APPLICATION_INHIBIT_SUSPEND),
                                             _("OS update in progress"));
}


static void
on_latest_update_changed (MsUpdatesPanel *self, GParamSpec *pspec, MsOsUpdater *os_updater)
{
  MsOsUpdate *update = ms_os_updater_get_latest_update (os_updater);

  if (self->os_update && ms_os_update_get_state (self->os_update) != MS_OS_UPDATE_STATE_READY)
    return;

  set_latest_os_update (self, update);
}


static void
ms_updates_panel_finalize (GObject *object)
{
  MsUpdatesPanel *self = MS_UPDATES_PANEL (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->os_updater);

  uninhibit (self);

  G_OBJECT_CLASS (ms_updates_panel_parent_class)->finalize (object);
}


static void
ms_updates_panel_class_init (MsUpdatesPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_updates_panel_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-updates-panel.ui");

  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, download_svg);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, fetch_page);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, fetch_progress);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, install_page);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, install_progress);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, reboot_banner);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, stack);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, start_update_btn);
  gtk_widget_class_bind_template_child (widget_class, MsUpdatesPanel, toast_overlay);

  gtk_widget_class_install_action (widget_class,
                                   "updates-panel.start-update",
                                   NULL,
                                   on_update_activated);
  gtk_widget_class_install_action (widget_class,
                                   "updates-panel.cancel-update",
                                   "s",
                                   on_cancel_update_activated);

  gtk_widget_class_bind_template_callback (widget_class, on_reboot_button_clicked);
}


static void
ms_updates_panel_init (MsUpdatesPanel *self)
{
  g_autoptr (GDBusConnection) conn = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->cancel = g_cancellable_new ();
  self->os_updater = ms_get_default_os_updater_sync ();
  g_object_bind_property (self->os_updater, "supported",
                          self, "enabled",
                          G_BINDING_SYNC_CREATE);

  /* No property binding as we don't want newly available updates to change text during an
   * update operation */
  g_signal_connect_swapped (self->os_updater,
                            "notify::latest-update",
                            G_CALLBACK (on_latest_update_changed),
                            self);
  on_latest_update_changed (self, NULL, self->os_updater);
}


MsUpdatesPanel *
ms_updates_panel_new (void)
{
  return MS_UPDATES_PANEL (g_object_new (MS_TYPE_UPDATES_PANEL, NULL));
}
