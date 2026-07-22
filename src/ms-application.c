/*
 * Copyright (C) 2022 Purism SPC
 *               2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-application"

#include "mobile-settings-config.h"

#include "ms-application.h"
#include "ms-window.h"
#include "ms-plugin.h"
#include "ms-plugin-loader.h"
#include "ms-toplevel-tracker.h"
#include "ms-head-tracker.h"
#include "ms-debug-info.h"
#include "ms-panel.h"

#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "wlr-output-management-unstable-v1-client-protocol.h"

#include <phosh-plugin.h>

#include <libfeedback.h>
#include <gmobile.h>

#include <gdk/wayland/gdkwayland.h>
#include <wayland-client.h>
#include <glib/gi18n.h>

#define PHOC_LAYER_SHELL_EFFECTS_PROTOCOL_NAME "zphoc_layer_shell_effects_v1"
#define PHOSH_PRIVATE_PROTOCOL_NAME "phosh_private"
#define PHOSH_MOBILE_SETTINGS_DESCRIPTION _("- Manage your mobile settings")

enum {
  PROP_0,
  PROP_TOPLEVEL_TRACKER,
  PROP_HEAD_TRACKER,
  PROP_ACTIVE_PANEL,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _MsApplication {
  AdwApplication  parent_instance;

  MsPluginLoader *device_plugin_loader;
  GtkWidget      *device_panel;

  struct wl_display  *wl_display;
  struct wl_registry *wl_registry;
  struct zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager;
  struct zwlr_output_manager_v1 *output_manager;
  MsToplevelTracker  *toplevel_tracker;
  MsHeadTracker     *head_tracker;

  GHashTable        *wayland_protocols;

  GtkWidget         *active_panel;
};

G_DEFINE_TYPE (MsApplication, ms_application, ADW_TYPE_APPLICATION)

static const GOptionEntry entries[] = {
  {
    "version", 'v',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
    NULL, "Current version of phosh-mobile-settings", NULL
  },
  {
    "debug-info", 'd',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
    NULL, "Print debugging information", NULL
  },
  {
    "list", 'l',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
    NULL, "List the available panels", NULL
  },
  {
    "only-conf-tweaks", 'c',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
    NULL, "Only show conf-tweaks panels, hide all built-in panels", NULL
  },
  {
    G_OPTION_REMAINING, '\0',
    G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME_ARRAY,
    NULL, "Panel to display", "[PANEL]"
  },
  G_OPTION_ENTRY_NULL
};


static void
ms_application_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MsApplication *self = MS_APPLICATION (object);

  switch (property_id) {
  case PROP_TOPLEVEL_TRACKER:
    g_value_set_object (value, self->toplevel_tracker);
    break;
  case PROP_HEAD_TRACKER:
    g_value_set_object (value, self->head_tracker);
    break;
  case PROP_ACTIVE_PANEL:
    g_value_set_object (value, self->active_panel);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_application_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MsApplication *self = MS_APPLICATION (object);

  switch (property_id) {
  case PROP_ACTIVE_PANEL:
    self->active_panel = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}



static void
registry_handle_global (void               *data,
                        struct wl_registry *registry,
                        uint32_t            name,
                        const char         *interface,
                        uint32_t            version)
{
  MsApplication *self = MS_APPLICATION (data);

  if (strcmp (interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
    self->foreign_toplevel_manager =
      wl_registry_bind (registry, name, &zwlr_foreign_toplevel_manager_v1_interface, 1);
  } else if (strcmp (interface, zwlr_output_manager_v1_interface.name) == 0) {
    self->output_manager =
      wl_registry_bind (registry, name, &zwlr_output_manager_v1_interface, 2);
  }

  if (self->foreign_toplevel_manager && self->output_manager &&
      self->toplevel_tracker == NULL) {
    g_debug ("Found all wayland protocols. Creating listeners.");

    self->toplevel_tracker = ms_toplevel_tracker_new (self->foreign_toplevel_manager);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TOPLEVEL_TRACKER]);

    self->head_tracker = ms_head_tracker_new (self->output_manager);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HEAD_TRACKER]);
  }

  g_hash_table_insert (self->wayland_protocols, g_strdup (interface), GUINT_TO_POINTER(version));
}


static void
registry_handle_global_remove (void               *data,
                               struct wl_registry *registry,
                               uint32_t            name)
{
  g_warning ("Global %d removed but not handled", name);
}


static const struct wl_registry_listener registry_listener = {
  registry_handle_global,
  registry_handle_global_remove
};


static void
setup_wayland (MsApplication *self)
{
  GdkDisplay *gdk_display;

  if (self->wl_display == NULL) {
    gdk_display = gdk_display_get_default ();
    self->wl_display = gdk_wayland_display_get_wl_display (gdk_display);
    if (self->wl_display != NULL) {
      self->wl_registry = wl_display_get_registry (self->wl_display);
      wl_registry_add_listener (self->wl_registry, &registry_listener, self);
    } else {
      g_critical ("Failed to get display: %m\n");
    }
  }
}


static gboolean
transform_to_active_panel (GBinding     *binding,
                           const GValue *from,
                           GValue       *to,
                           gpointer      user_data)
{
  g_autoptr (AdwViewStack) stack = ADW_VIEW_STACK (g_binding_dup_source (binding));
  GtkWidget *stack_child = g_value_get_object (from);
  AdwViewStackPage *page = adw_view_stack_get_page (stack, stack_child);

  g_value_set_object (to, adw_view_stack_page_get_child (page));

  return TRUE;
}


static GtkWindow *
get_active_window (MsApplication *self)
{
  GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (self));

  if (window == NULL) {
    AdwViewStack *panel_stack;
    MsPanelSwitcher *panel_switcher;

    window = g_object_new (MS_TYPE_WINDOW, "application", self, NULL);
    panel_switcher = ms_window_get_panel_switcher (MS_WINDOW (window));
    panel_stack = ms_panel_switcher_get_stack (panel_switcher);

    /* Bind here so that we do it just once per window */
    g_object_bind_property_full (panel_stack, "visible-child",
                                 self, "active-panel",
                                 G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                                 transform_to_active_panel, NULL,
                                 NULL, NULL);
  }

  return window;
}


static void
print_version (void)
{
  g_print ("Phosh Mobile Settings %s %s\n",
           MOBILE_SETTINGS_VERSION,
           PHOSH_MOBILE_SETTINGS_DESCRIPTION);
}


static void
print_system_information (GApplication *app)
{
  g_autofree char *debug_info = NULL;
  MsApplication *self = MS_APPLICATION (app);

  /* We need this to init components and generate debug info */
  adw_init ();

  setup_wayland (self);
  /* Make sure all pending events got processed by the server */
  wl_display_roundtrip (self->wl_display);
  wl_display_dispatch (self->wl_display);

  debug_info = ms_generate_debug_info ();
  g_print ("Debugging information:\n%s", debug_info);
}


static void
list_available_panels (GApplication *app)
{
  MsWindow *window;
  GListModel *list;
  const char *name;

  /* Since we're in the local instance, just get us a window */
  adw_init ();

  window = g_object_new (MS_TYPE_WINDOW, NULL);
  list = ms_window_get_stack_pages (window);

  g_print ("Available panels:\n");

  for (uint i = 0; i < g_list_model_get_n_items (list); i++) {
    g_autoptr (AdwViewStackPage) page = g_list_model_get_item (list, i);

    name = adw_view_stack_page_get_name (page);
    g_print ("- %s\n", name);
  }

  gtk_window_destroy (GTK_WINDOW (window));
}


static void
set_panel_activated (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  MsApplication *self = MS_APPLICATION (user_data);
  MsWindow *window;
  MsPanelSwitcher *panel_switcher;
  char *panel;
  g_autoptr (GVariant) params = NULL;

  g_variant_get (parameter, "(&s@av)", &panel, &params);

  g_debug ("'set-panel' '%s'", panel);

  window = MS_WINDOW (get_active_window (self));
  panel_switcher = ms_window_get_panel_switcher (window);

  if (!ms_panel_switcher_set_active_panel_name (panel_switcher, panel))
    g_warning ("Error: panel `%s` not available, launching with default options.", panel);

  gtk_window_present (GTK_WINDOW (window));

  if (!ms_panel_handle_options (MS_PANEL (self->active_panel), params))
    g_debug ("Panel '%s' failed to parse given options", panel);
}


static void
only_tweaks_panels_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  g_autoptr (GSettings) settings = g_settings_new ("mobi.phosh.MobileSettings");
  MsApplication *self = MS_APPLICATION (user_data);
  MsPanelSwitcher *panel_switcher;
  MsWindow *window;

  window = MS_WINDOW (get_active_window (self));
  panel_switcher = ms_window_get_panel_switcher (window);

  g_settings_set_boolean (settings, "enable-conf-tweaks", TRUE);
  g_action_map_remove_action (G_ACTION_MAP (self), "enable-conf-tweaks");
  ms_panel_switcher_set_only_tweaks (panel_switcher, TRUE);
}


MsApplication *
ms_application_new (char *application_id)
{
  return g_object_new (MS_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", G_APPLICATION_DEFAULT_FLAGS,
                       NULL);
}

/**
 * ms_application_set_panel:
 * @self: the application
 * @panel: the panel to set
 *
 * Convenience wrapper to activate the given panel via the `set-panel` action.
 */
static void
ms_application_set_panel (MsApplication *self, const char *panel)
{
  GVariantBuilder builder;
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));

  g_action_group_activate_action (G_ACTION_GROUP (self),
                                  "set-panel",
                                  g_variant_new ("(s@av)",
                                                 panel, g_variant_builder_end (&builder)));
}


static int
ms_application_handle_local_options (GApplication *app, GVariantDict *options)
{
  MsApplication *self = MS_APPLICATION (app);
  gboolean only_tweaks_panels = FALSE;
  g_autofree GStrv panels = NULL;
  GApplicationClass *app_class = G_APPLICATION_CLASS (ms_application_parent_class);
  g_autofree char *panel = NULL;

  if (g_variant_dict_contains (options, "version")) {
    print_version ();

    return 0;
  } else if (g_variant_dict_contains (options, "debug-info")) {
    print_system_information (app);

    return 0;
  } else if (g_variant_dict_contains (options, "list")) {
    list_available_panels (app);

    return 0;
  } else if (g_variant_dict_contains (options, "only-conf-tweaks")) {
    only_tweaks_panels = TRUE;
  } else if (g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&ay", &panels)) {

    g_return_val_if_fail (panels && panels[0], EXIT_FAILURE);
    panel = g_strdup (panels[0]);
  }

  if (!panel) {
    g_autoptr (GSettings) settings = g_settings_new ("mobi.phosh.MobileSettings");

    panel = g_settings_get_string (settings, "last-panel");
  }

  if (only_tweaks_panels) {
    g_application_register (G_APPLICATION (app), NULL, NULL);
    g_action_group_activate_action (G_ACTION_GROUP (self), "only-tweaks-panels", NULL);
  }

  if (!gm_str_is_null_or_empty (panel)) {
    g_application_register (G_APPLICATION (app), NULL, NULL);
    ms_application_set_panel (self, panel);
  }

  return app_class->handle_local_options (app, options);
}


static void
ms_application_activate (GApplication *app)
{
  GtkWindow *window;
  MsApplication *self = MS_APPLICATION (app);

  g_assert (GTK_IS_APPLICATION (app));

  window = get_active_window (self);

  setup_wayland (self);

  gtk_window_present (window);
}


static const GActionEntry actions[] = {
  { "set-panel", set_panel_activated, "(sav)", NULL, NULL, { 0 } },
  { "only-tweaks-panels", only_tweaks_panels_activated, NULL, NULL, NULL, { 0 } },
};


static void
ms_application_startup (GApplication *app)
{
  g_autoptr (GError) err = NULL;
  MsApplication *self = MS_APPLICATION (app);

  if (!lfb_init (MOBILE_SETTINGS_APP_ID, &err))
    g_warning ("Failed to init libfeedback: %s", err->message);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   actions,
                                   G_N_ELEMENTS (actions),
                                   self);

  G_APPLICATION_CLASS (ms_application_parent_class)->startup (app);
}


static void
ms_application_shutdown (GApplication *app)
{
  G_APPLICATION_CLASS (ms_application_parent_class)->shutdown (app);

  lfb_uninit ();
}


static void
ms_application_finalize (GObject *object)
{
  MsApplication *self = MS_APPLICATION (object);

  g_clear_object (&self->device_plugin_loader);
  g_clear_pointer (&self->wayland_protocols, g_hash_table_destroy);

  G_OBJECT_CLASS (ms_application_parent_class)->finalize (object);
}


static void
ms_application_class_init (MsApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = ms_application_finalize;
  object_class->get_property = ms_application_get_property;
  object_class->set_property = ms_application_set_property;

  app_class->activate = ms_application_activate;
  app_class->startup = ms_application_startup;
  app_class->shutdown = ms_application_shutdown;
  app_class->handle_local_options = ms_application_handle_local_options;

  props[PROP_TOPLEVEL_TRACKER] =
    g_param_spec_object ("toplevel-tracker", "", "",
                         MS_TYPE_TOPLEVEL_TRACKER,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_HEAD_TRACKER] =
    g_param_spec_object ("head-tracker", "", "",
                         MS_TYPE_HEAD_TRACKER,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ACTIVE_PANEL] =
    g_param_spec_object ("active-panel", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}

static void
ms_application_show_about (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  MsApplication *self = MS_APPLICATION (user_data);
  GtkWindow *window;
  AdwAboutDialog *about_dialog;
  const char *developers[] = {"Guido Günther", "Gotam Gorabh", NULL};
  const char *artists[] = {"Sam Hewitt", NULL};

  g_return_if_fail (MS_APPLICATION (self));

  about_dialog = ADW_ABOUT_DIALOG (adw_about_dialog_new_from_appdata ("/mobi/phosh/MobileSettings/"
                                                                      "metainfo.xml",
                                                                      MOBILE_SETTINGS_VERSION));
  adw_about_dialog_set_developers (about_dialog, developers);
  adw_about_dialog_set_artists (about_dialog, artists);
  adw_about_dialog_set_translator_credits (about_dialog,
               /* Translators: Replace "translator-credits" with your names, one name per line */
                                           _("translator-credits"));
  adw_about_dialog_set_debug_info (about_dialog, ms_generate_debug_info ());

  window = gtk_application_get_active_window (GTK_APPLICATION (self));
  adw_dialog_present (ADW_DIALOG (about_dialog), GTK_WIDGET (window));
}


static void
ms_application_init (MsApplication *self)
{
  const char *plugin_dirs[] = { MOBILE_SETTINGS_PLUGINS_DIR, NULL };
  g_autoptr (GSimpleAction) about_action = NULL;
  g_autoptr (GSimpleAction) quit_action = NULL;

  quit_action = g_simple_action_new ("quit", NULL);
  g_signal_connect_swapped (quit_action, "activate", G_CALLBACK (g_application_quit), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (quit_action));

  about_action = g_simple_action_new ("about", NULL);
  g_signal_connect (about_action, "activate", G_CALLBACK (ms_application_show_about), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (about_action));

  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.quit",
                                         (const char *[]) { "<primary>q", NULL, });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "win.search",
                                         (const char *[]) { "<Primary>F", NULL, });

  g_application_set_option_context_parameter_string (G_APPLICATION (self),
                                                     PHOSH_MOBILE_SETTINGS_DESCRIPTION);
  g_application_add_main_option_entries (G_APPLICATION (self), entries);

  self->device_plugin_loader = ms_plugin_loader_new (plugin_dirs, MS_EXTENSION_POINT_DEVICE_PANEL);
  self->wayland_protocols = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   NULL);

  g_io_extension_point_register (PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS);
  g_io_extension_point_register (PHOSH_PLUGIN_EXTENSION_POINT_QUICK_SETTING_WIDGET_PREFS);
}


GtkWidget *
ms_application_get_device_panel (MsApplication *self)
{
  if (self->device_panel)
    return self->device_panel;

  self->device_panel = ms_plugin_loader_load_plugin (self->device_plugin_loader);
  return self->device_panel;
}


MsToplevelTracker *
ms_application_get_toplevel_tracker (MsApplication *self)
{
  g_assert (MS_APPLICATION (self));

  return self->toplevel_tracker;
}


MsHeadTracker *
ms_application_get_head_tracker (MsApplication *self)
{
  g_assert (MS_APPLICATION (self));

  return self->head_tracker;
}


GStrv
ms_application_get_wayland_protocols (MsApplication *self)
{
  g_autoptr (GList) keys = NULL;
  g_autoptr (GStrvBuilder) protocols = g_strv_builder_new ();

  g_assert (MS_APPLICATION (self));

  keys = g_hash_table_get_keys (self->wayland_protocols);
  g_assert (keys);

  for (GList *l = keys; l; l = l->next)
    g_strv_builder_add (protocols, l->data);

  return g_strv_builder_end (protocols);
}


guint32
ms_application_get_wayland_protocol_version (MsApplication *self, const char *protocol)
{
  gpointer *version;

  g_assert (MS_APPLICATION (self));

  version = g_hash_table_lookup (self->wayland_protocols, protocol);

  return GPOINTER_TO_UINT (version);
}
