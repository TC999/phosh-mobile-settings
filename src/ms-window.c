/*
 * Copyright (C) 2022 Purism SPC
 *               2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */


#define G_LOG_DOMAIN "ms-window"

#include "mobile-settings-config.h"

#include "ms-application.h"
#include "ms-window.h"

#include "ms-plugin-panel.h"

#include "conf-tweaks/ms-tweaks-parser.h"
#include "conf-tweaks/ms-tweaks-preferences-page.h"

#include <glib/gi18n.h>


struct _MsWindow {
  AdwApplicationWindow    parent_instance;

  GtkSearchBar           *search_bar;
  GtkSearchEntry         *search_entry;

  AdwNavigationSplitView *split_view;
  AdwViewStack           *stack;
  MsPanelSwitcher        *panel_switcher;
  GListModel *enabled_pages;

  GSettings *settings;
  MsTweaksParser         *ms_tweaks_parser;
};

G_DEFINE_TYPE (MsWindow, ms_window, ADW_TYPE_APPLICATION_WINDOW)


static void
on_search_entry_changed (GtkSearchEntry *search_entry,
                         MsWindow       *self)
{
  ms_panel_switcher_set_search_query (self->panel_switcher,
                                      gtk_editable_get_text (GTK_EDITABLE (search_entry)));
}


static void
on_search_entry_activated (GtkSearchEntry *search_entry,
                           MsWindow       *self)
{
  ms_panel_switcher_set_active_panel_index (self->panel_switcher, 0);
}


static void
on_search_activated (GtkWidget *widget, const char *action_name, GVariant *param)
{
  MsWindow *self = MS_WINDOW (widget);

  adw_navigation_split_view_set_show_content (self->split_view, FALSE);
  gtk_search_bar_set_search_mode (self->search_bar, TRUE);
}


static void
show_content_cb (MsWindow *self)
{
  const char *panelname;

  adw_navigation_split_view_set_show_content (self->split_view, TRUE);

  panelname = adw_view_stack_get_visible_child_name (self->stack);

  g_settings_set_string (self->settings, "last-panel", panelname);

  /* Clear search entry to display all panels again */
  if (gtk_search_bar_get_search_mode (self->search_bar)) {
    gtk_search_bar_set_search_mode (self->search_bar, FALSE);
    gtk_editable_delete_text (GTK_EDITABLE (self->search_entry), 0, -1);
  }
}


static char *
stack_child_to_title (gpointer target, AdwViewStack *stack, GtkWidget *child)
{
  const char *title;
  AdwViewStackPage *page;

  g_assert (ADW_IS_VIEW_STACK (stack));
  g_assert (GTK_IS_WIDGET (child));

  page = adw_view_stack_get_page (stack, child);
  title = adw_view_stack_page_get_title (page);
  if (title == NULL)
    title = adw_view_stack_page_get_name (page);

  return g_strdup (title);
}


static void
add_ms_tweaks_page (gpointer value, gpointer user_data)
{
  MsWindow *self = MS_WINDOW (user_data);
  MsTweaksPage *page_data = (MsTweaksPage *) value;
  MsTweaksPreferencesPage *page_widget = ms_tweaks_preferences_page_new (page_data);
  AdwViewStackPage *stack_page;
  static gboolean section_started;

  if (!page_widget)
    return;

  stack_page = adw_view_stack_add_titled (self->stack,
                                          GTK_WIDGET (page_widget),
                                          page_data->name_i18n,
                                          page_data->name_i18n);

  /* TODO: Read icon from base64 property of settings definitions. */
  adw_view_stack_page_set_icon_name (stack_page, "conf-tweaks-symbolic");
  if (section_started)
    return;

  section_started = TRUE;
  adw_view_stack_page_set_section_title (stack_page, _("Configurable Tweaks"));
  adw_view_stack_page_set_starts_section (stack_page, TRUE);
}


static void
do_toggle_conf_tweaks (GSettings *settings, char *key, gpointer user_data)
{
  MsWindow *self = MS_WINDOW (user_data);
  gboolean conf_tweaks_enabled = g_settings_get_boolean (settings, key);

  /* Flip! */
  conf_tweaks_enabled = !conf_tweaks_enabled;

  ms_panel_switcher_refilter (self->panel_switcher,
                              conf_tweaks_enabled ? GTK_FILTER_CHANGE_LESS_STRICT : GTK_FILTER_CHANGE_MORE_STRICT);
}


static void
on_panel_enabled_changed (MsWindow *self, GParamSpec *pspec, MsPanel *panel)
{
  GtkFilter *filter;
  gboolean enabled = ms_panel_get_enabled (panel);
  GtkFilterChange change = enabled ? GTK_FILTER_CHANGE_LESS_STRICT : GTK_FILTER_CHANGE_MORE_STRICT;

  filter = gtk_filter_list_model_get_filter (GTK_FILTER_LIST_MODEL (self->enabled_pages));
  ms_panel_switcher_refilter (self->panel_switcher, change);
  gtk_filter_changed (filter, change);
}


static void
ms_settings_window_constructed (GObject *object)
{
  MsWindow *self = MS_WINDOW (object);
  MsApplication *app = MS_APPLICATION (g_application_get_default ());
  GtkWidget *device_panel;
  GHashTable *parser_page_table = NULL;

  G_OBJECT_CLASS (ms_window_parent_class)->constructed (object);

  if (adw_view_stack_get_child_by_name (self->stack, "device") == NULL) {
    const char *title;

    g_assert (GTK_IS_APPLICATION (app));
    device_panel = ms_application_get_device_panel (app);
    if (device_panel) {
      AdwViewStackPage *page;

      title = ms_plugin_panel_get_title (MS_PLUGIN_PANEL (device_panel));
      page = adw_view_stack_add_titled (self->stack, device_panel, "device", title ?: _("Device"));
      adw_view_stack_page_set_icon_name (page, "phone-symbolic");
    }
  }

  ms_tweaks_parser_parse_definition_files (self->ms_tweaks_parser, TWEAKS_DATA_DIR);
  parser_page_table = ms_tweaks_parser_get_page_table (self->ms_tweaks_parser);

  if (g_hash_table_size (parser_page_table) != 0) {
    g_autoptr (GList) pages_sorted_by_weight = ms_tweaks_parser_sort_by_weight (parser_page_table);
    g_autoptr (GAction) toggle_conf_tweaks = NULL;

    g_list_foreach (pages_sorted_by_weight, add_ms_tweaks_page, self);

    toggle_conf_tweaks = g_settings_create_action (self->settings, "enable-conf-tweaks");
    g_signal_connect (self->settings,
                      "changed::enable-conf-tweaks",
                      G_CALLBACK (do_toggle_conf_tweaks),
                      self);
    g_action_map_add_action (G_ACTION_MAP (app), G_ACTION (toggle_conf_tweaks));
  }
}


static void
ms_settings_window_dispose (GObject *object)
{
  MsWindow *self = MS_WINDOW (object);

  g_clear_object (&self->enabled_pages);
  g_clear_object (&self->settings);
  g_clear_object (&self->ms_tweaks_parser);

  G_OBJECT_CLASS (ms_window_parent_class)->dispose (object);
}


static void
ms_window_class_init (MsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ms_settings_window_constructed;
  object_class->dispose = ms_settings_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-window.ui");
  gtk_widget_class_bind_template_child (widget_class, MsWindow, search_bar);
  gtk_widget_class_bind_template_child (widget_class, MsWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, MsWindow, split_view);
  gtk_widget_class_bind_template_child (widget_class, MsWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, MsWindow, panel_switcher);

  gtk_widget_class_bind_template_callback (widget_class, on_panel_enabled_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_search_entry_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_search_entry_activated);
  gtk_widget_class_bind_template_callback (widget_class, show_content_cb);
  gtk_widget_class_bind_template_callback (widget_class, stack_child_to_title);

  gtk_widget_class_install_action (widget_class, "win.search", NULL, on_search_activated);
}


static gboolean
match_enabled_filter (gpointer item, gpointer user_data)
{
  AdwViewStackPage *view_stack_page = ADW_VIEW_STACK_PAGE (item);
  MsPanel *panel;

  g_return_val_if_fail (ADW_IS_VIEW_STACK_PAGE (item), FALSE);

  panel = MS_PANEL (adw_view_stack_page_get_child (view_stack_page));

  return ms_panel_get_enabled (panel);
}


static void
ms_window_init (MsWindow *self)
{
  GListModel *pages;
  GtkFilter *enabled_filter;

  self->settings = g_settings_new ("mobi.phosh.MobileSettings");
  self->ms_tweaks_parser = ms_tweaks_parser_new ();

  gtk_widget_init_template (GTK_WIDGET (self));

  pages = G_LIST_MODEL (adw_view_stack_get_pages (self->stack));
  enabled_filter = GTK_FILTER (gtk_custom_filter_new (match_enabled_filter, NULL, NULL));
  self->enabled_pages = G_LIST_MODEL (gtk_filter_list_model_new (pages,
                                                                 GTK_FILTER (enabled_filter)));

  show_content_cb (self);

  gtk_search_bar_set_key_capture_widget (self->search_bar, GTK_WIDGET (self));
}


GListModel *
ms_window_get_stack_pages (MsWindow *self)
{
  g_assert (MS_IS_WINDOW (self));

  return self->enabled_pages;
}


MsPanelSwitcher *
ms_window_get_panel_switcher (MsWindow *self)
{
  g_assert (MS_IS_WINDOW (self));

  return self->panel_switcher;
}
