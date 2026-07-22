/*
 * Copyright (C) 2022 Purism SPC
 *               2025 GNOME Foundation Inc.
 *               2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-panel-switcher"

#include "mobile-settings-config.h"

#include "ms-panel-switcher.h"
#include "ms-panel.h"
#include "ms-tweaks-preferences-page.h"
#include "ms-util.h"

enum {
  ROW_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

enum {
  PROP_0,
  PROP_STACK,
  PROP_MODE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsPanelSwitcher {
  AdwBin         parent;

  AdwSidebar    *sidebar;
  AdwViewStack  *stack;
  GListModel    *pages;
  GSettings     *settings;
  GHashTable    *items;
  AdwSidebarMode mode;

  char          *query;

  gboolean       only_tweaks;
};
G_DEFINE_TYPE (MsPanelSwitcher, ms_panel_switcher, ADW_TYPE_BIN)


static void
add_item (MsPanelSwitcher   *self,
          AdwSidebarSection *section,
          AdwViewStackPage  *page,
          guint              index)
{
  AdwSidebarItem *item = adw_sidebar_item_new ("");

  g_hash_table_insert (self->items, page, item);

  g_object_bind_property (page, "title", item, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (page, "icon-name", item, "icon-name", G_BINDING_SYNC_CREATE);
  g_object_bind_property (page, "use-underline", item, "use-underline", G_BINDING_SYNC_CREATE);
  g_object_bind_property (page, "visible", item, "visible", G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (item), "ms-page", page);

  adw_sidebar_section_append (section, item);
}


static void
populate_sidebar (MsPanelSwitcher *self)
{
  guint i, n = g_list_model_get_n_items (self->pages);
  GtkWidget *visible_child;
  AdwViewStackPage *page;
  guint index = GTK_INVALID_LIST_POSITION;
  AdwSidebarSection *section = NULL;

  for (i = 0; i < n; i++) {
    page = g_list_model_get_item (self->pages, i);

    if (!section || adw_view_stack_page_get_starts_section (page)) {
      if (section)
        adw_sidebar_append (self->sidebar, section);

      section = adw_sidebar_section_new ();
      g_object_bind_property (page, "section-title", section, "title", G_BINDING_SYNC_CREATE);
    }

    add_item (self, section, page, i);
  }

  if (section)
    adw_sidebar_append (self->sidebar, section);

  visible_child = adw_view_stack_get_visible_child (self->stack);
  if (visible_child) {
    AdwSidebarItem *item;

    page = adw_view_stack_get_page (self->stack, visible_child);
    item = g_hash_table_lookup (self->items, page);
    index = adw_sidebar_item_get_index (item);
  }

  adw_sidebar_set_selected (self->sidebar, index);
}


static void
repopulate_sidebar (MsPanelSwitcher *self)
{
  adw_sidebar_remove_all (self->sidebar);
  populate_sidebar (self);
}


static void
on_selection_changed (MsPanelSwitcher   *self,
                      guint              position,
                      guint              n_items,
                      GtkSelectionModel *model)
{
  GtkWidget *visible_child = adw_view_stack_get_visible_child (self->stack);
  guint index = GTK_INVALID_LIST_POSITION;

  if (visible_child) {
    AdwViewStackPage *page = adw_view_stack_get_page (self->stack, visible_child);
    AdwSidebarItem *item = g_hash_table_lookup (self->items, page);
    index = adw_sidebar_item_get_index (item);
  }

  adw_sidebar_set_selected (self->sidebar, index);
}


static void
set_stack (MsPanelSwitcher *self, AdwViewStack *stack)
{
  guint i, n;

  if (!stack)
    return;

  self->stack = g_object_ref (stack);
  self->pages = g_object_ref (G_LIST_MODEL (adw_view_stack_get_pages (stack)));

  populate_sidebar (self);

  n = g_list_model_get_n_items (self->pages);

  for (i = 0; i < n; i++) {
    AdwViewStackPage *page = g_list_model_get_item (self->pages, i);

    g_signal_connect_swapped (page, "notify::visible", G_CALLBACK (repopulate_sidebar), self);

    g_object_unref (page);
  }

  g_signal_connect_swapped (self->pages, "items-changed", G_CALLBACK (repopulate_sidebar), self);
  g_signal_connect_swapped (self->pages, "sections-changed", G_CALLBACK (repopulate_sidebar), self);
  g_signal_connect_swapped (self->pages,
                            "selection-changed",
                            G_CALLBACK (on_selection_changed),
                            self);
}

static void
unset_stack (MsPanelSwitcher *self)
{
  guint i, n;

  if (!self->stack)
    return;

  adw_sidebar_remove_all (self->sidebar);

  n = g_list_model_get_n_items (self->pages);

  for (i = 0; i < n; i++) {
    AdwViewStackPage *page = g_list_model_get_item (self->pages, i);

    g_signal_handlers_disconnect_by_func (page, repopulate_sidebar, self);

    g_object_unref (page);
  }

  g_signal_handlers_disconnect_by_func (self->pages, repopulate_sidebar, self);
  g_signal_handlers_disconnect_by_func (self->pages, on_selection_changed, self);
  g_clear_object (&self->pages);
  g_clear_object (&self->stack);
}


static void
on_activated (MsPanelSwitcher *self, guint index)
{
  const char *name;

  gtk_selection_model_select_item (GTK_SELECTION_MODEL (self->pages), index, TRUE);

  name = adw_view_stack_get_visible_child_name (self->stack);
  g_debug ("Activating '%s' (%d)", name, index);

  g_signal_emit (self, signals[ROW_ACTIVATED], 0);
}


static gboolean
panels_filter_func (gpointer item_, gpointer user_data)
{
  MsPanelSwitcher *self = MS_PANEL_SWITCHER (user_data);
  AdwSidebarItem *item = ADW_SIDEBAR_ITEM (item_);
  AdwViewStackPage *page = ADW_VIEW_STACK_PAGE (g_object_get_data (G_OBJECT (item), "ms-page"));
  const char *panelname = adw_view_stack_page_get_name (page);
  g_autofree char *query_normalized = NULL;
  g_auto (GStrv) query_words = NULL;
  GtkWidget *stack_child;
  MsPanel *panel;
  GtkStringList *keywords;

  stack_child = adw_view_stack_page_get_child (page);
  if (MS_IS_TWEAKS_PREFERENCES_PAGE (stack_child)
      && !g_settings_get_boolean (self->settings, "enable-conf-tweaks"))
    return FALSE;

  if (self->only_tweaks && !MS_IS_TWEAKS_PREFERENCES_PAGE (stack_child))
    return FALSE;

  panel = MS_PANEL (stack_child);
  if (!ms_panel_get_enabled (panel))
    return FALSE;

  /* Search is empty, show all enabled panels */
  if (GM_STR_IS_NULL_OR_EMPTY (self->query))
    return TRUE;

  if (g_strcmp0 (panelname, "welcome") == 0)
    return FALSE;

  keywords = ms_panel_get_keywords (panel);
  query_normalized = ms_normalize_casefold_and_unaccent (self->query);
  query_words = g_strsplit (g_strstrip (query_normalized), " ", 0);

  /* Search is empty, show all enabled panels */
  if (query_words[0] == NULL)
    return TRUE;

  if (!keywords) {
    g_warning ("Could not get `keywords` for panel: %s", panelname);

    return FALSE;
  }

  for (uint i = 0; query_words[i] != NULL; i++) {
    for (uint j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (keywords)); j++) {
      const char *keyword = gtk_string_list_get_string (keywords, j);

      if (g_strstr_len (keyword, -1, query_words[i]) == keyword)
        return TRUE;
    }

    /* Try finally the name */
    if (g_strstr_len (panelname, -1, query_words[i]) != NULL)
      return TRUE;
  }

  return FALSE;
}


static void
ms_panel_switcher_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MsPanelSwitcher *self = MS_PANEL_SWITCHER (object);

  switch (property_id) {
  case PROP_STACK:
    ms_panel_switcher_set_stack (self, g_value_get_object (value));
    break;
  case PROP_MODE:
    adw_sidebar_set_mode (self->sidebar, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_panel_switcher_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MsPanelSwitcher *self = MS_PANEL_SWITCHER (object);

  switch (property_id) {
  case PROP_STACK:
    g_value_set_object (value, self->stack);
    break;
  case PROP_MODE:
    g_value_set_enum (value, self->mode);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_panel_switcher_dispose (GObject *object)
{
  MsPanelSwitcher *self = MS_PANEL_SWITCHER (object);

  unset_stack (self);

  g_clear_pointer (&self->items, g_hash_table_unref);

  g_clear_object (&self->settings);
  g_clear_pointer (&self->query, g_free);

  G_OBJECT_CLASS (ms_panel_switcher_parent_class)->dispose (object);
}


static void
ms_panel_switcher_class_init (MsPanelSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_panel_switcher_get_property;
  object_class->set_property = ms_panel_switcher_set_property;
  object_class->dispose = ms_panel_switcher_dispose;

  props[PROP_STACK] =
    g_param_spec_object ("stack", "", "",
                         ADW_TYPE_VIEW_STACK,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  props[PROP_MODE] =
    g_param_spec_enum ("mode", "", "",
                       ADW_TYPE_SIDEBAR_MODE,
                       ADW_SIDEBAR_MODE_SIDEBAR,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[ROW_ACTIVATED] = g_signal_new ("row-activated",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST, 0,
                                         NULL, NULL, NULL,
                                         G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-panel-switcher.ui");
  gtk_widget_class_bind_template_child (widget_class, MsPanelSwitcher, sidebar);
  gtk_widget_class_bind_template_callback (widget_class, on_activated);
}


static void
ms_panel_switcher_init (MsPanelSwitcher *self)
{
  g_autoptr (GtkCustomFilter) filter = gtk_custom_filter_new (panels_filter_func, self, NULL);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->items = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);
  self->settings = g_settings_new ("mobi.phosh.MobileSettings");

  adw_sidebar_set_filter (self->sidebar, GTK_FILTER (filter));
}


MsPanelSwitcher *
ms_panel_switcher_new (void)
{
  return MS_PANEL_SWITCHER (g_object_new (MS_TYPE_PANEL_SWITCHER, NULL));
}


void
ms_panel_switcher_set_stack (MsPanelSwitcher *self, AdwViewStack *stack)
{
  g_return_if_fail (MS_IS_PANEL_SWITCHER (self));
  g_return_if_fail (ADW_IS_VIEW_STACK (stack));

  if (self->stack == stack)
    return;

  unset_stack (self);
  set_stack (self, stack);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_STACK]);
}


gboolean
ms_panel_switcher_set_active_panel_name (MsPanelSwitcher *self, const char *panelname)
{
  GtkWidget *panel;

  g_assert (MS_IS_PANEL_SWITCHER (self));

  panel = adw_view_stack_get_child_by_name (self->stack, panelname);
  if (panel == NULL) {
    g_debug ("No panel '%s'", panelname);
    return FALSE;
  }

  if (!ms_panel_get_enabled (MS_PANEL (panel))) {
    g_debug ("Panel '%s' not enabled", panelname);
    return FALSE;
  }

  if (g_strcmp0 (adw_view_stack_get_visible_child_name (self->stack), panelname) == 0) {
    /* Panel is already active, make sure split view shows the panel, not the view switcher */
    g_signal_emit (self, signals[ROW_ACTIVATED], 0);
  } else {
    adw_view_stack_set_visible_child_name (self->stack, panelname);
  }

  return TRUE;
}

/**
 * ms_panel_switcher_set_active_panel_index:
 * @self: The panel switcher
 * @idx: The index
 *
 * Select a panel by its index. This takes any filters into accout so e.g.
 * `idx` `0` always selects the topmost item currently visible to the user.
 */
void
ms_panel_switcher_set_active_panel_index (MsPanelSwitcher *self, uint idx)
{
  g_autoptr (GtkFilterListModel) filtered = NULL;
  g_autoptr (GObject) idx_item = NULL;
  GListModel *items;
  GtkFilter *filter;

  g_assert (MS_IS_PANEL_SWITCHER (self));

  /* We can't get the filtered item directly so reapply the same filter AdwSidebar applies */
  /* https://gitlab.gnome.org/GNOME/libadwaita/-/merge_requests/1773 */
  items = G_LIST_MODEL (adw_sidebar_get_items (self->sidebar));
  filter = adw_sidebar_get_filter (self->sidebar);
  filtered = gtk_filter_list_model_new (g_object_ref (items),  g_object_ref (filter));

  idx_item = g_list_model_get_item (G_LIST_MODEL (filtered), idx);
  if (idx_item == NULL)
    return;

  /* Find the position of the indexed item in the unfiltered list */
  for (guint i = 0; i < g_list_model_get_n_items (items); i++) {
    g_autoptr (GObject) item = g_list_model_get_item (items, i);

    if (item == idx_item) {
      AdwViewStackPage *page;
      GtkWidget *child;

      /* Get the page and select the item */
      g_debug ("Selecting panel: %u", i);
      page = g_object_get_data (item, "ms-page");
      g_return_if_fail (ADW_IS_VIEW_STACK_PAGE (page));
      child = adw_view_stack_page_get_child (page);
      adw_view_stack_set_visible_child (self->stack, child);
      return;
    }
  }
}


void
ms_panel_switcher_set_only_tweaks (MsPanelSwitcher *self, const gboolean only_tweaks)
{
  GtkFilterChange change;

  g_assert (MS_IS_PANEL_SWITCHER (self));

  self->only_tweaks = only_tweaks;

  change = only_tweaks ? GTK_FILTER_CHANGE_MORE_STRICT : GTK_FILTER_CHANGE_LESS_STRICT;
  ms_panel_switcher_refilter (self, change);
}


void
ms_panel_switcher_set_search_query (MsPanelSwitcher *self, const char *cur_query)
{
  GtkFilter *filter = adw_sidebar_get_filter (self->sidebar);

  g_assert (MS_IS_PANEL_SWITCHER (self));

  if (g_strcmp0 (self->query, cur_query) == 0)
    return;

  g_set_str (&self->query, cur_query);
  gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}


void
ms_panel_switcher_refilter (MsPanelSwitcher *self, GtkFilterChange filter_change_hint)
{
  GtkFilter *filter = adw_sidebar_get_filter (self->sidebar);

  gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}


AdwViewStack *
ms_panel_switcher_get_stack (MsPanelSwitcher *self)
{
  g_return_val_if_fail (MS_IS_PANEL_SWITCHER (self), NULL);

  return self->stack;
}
