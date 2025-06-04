/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-alerts-panel"

#include "mobile-settings-config.h"

#include "mobile-settings-application.h"
#include "ms-alerts-panel.h"
#include "ms-enum-types.h"
#include "ms-scale-to-fit-row.h"
#include "ms-util.h"

#define CBD_SCHEMA_ID "org.freedesktop.cbd"
#define CBD_CHANNELS_KEY "channels"
#define CBD_LEVELS_KEY "levels"

#define CBD_CBS_SUPPORTED "CbsSupported"

enum {
  PROP_0,
  PROP_HAS_CBS,
  PROP_LEVELS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


const char *const level_names[] = {
  "extreme",
  "severe",
  "public_safety",
  "amber",
  "test",
};


struct _MsAlertsPanel {
  AdwBin          parent;

  GtkStack       *stack;

  gboolean        has_cbs;
  gboolean        setting_levels;
  GSettings      *settings;
  MsChannelLevel  levels;
  MsChannelMode   mode;

  GDBusProxy     *cbd_proxy;
  GCancellable   *cancel;

  AdwSwitchRow   *rows[G_N_ELEMENTS (level_names)];
};

G_DEFINE_TYPE (MsAlertsPanel, ms_alerts_panel, ADW_TYPE_BIN)


static void
set_has_cbs (MsAlertsPanel *self, gboolean has_cbs)
{
  if (self->has_cbs == has_cbs)
    return;

  self->has_cbs = has_cbs;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HAS_CBS]);
}


static void
on_cbd_dbus_properties_changed (MsAlertsPanel *self,
                                GVariant      *changed_properties,
                                char         **invalidated_properties,
                                GDBusProxy    *proxy)
{
  gboolean success, cbs_supported;

  success = g_variant_lookup (changed_properties, CBD_CBS_SUPPORTED, "b", &cbs_supported);
  if (!success) {
    g_warning ("Failed to read " CBD_CBS_SUPPORTED " property from cbd");
    return;
  }
  set_has_cbs (self, cbs_supported);
}


static void
on_cbd_proxy_ready (GObject *source_object, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (GVariant) var = NULL;
  MsAlertsPanel *self = MS_ALERTS_PANEL (data);
  gboolean cbs_supported;

  self->cbd_proxy = g_dbus_proxy_new_for_bus_finish (res, &err);
  if (!self->cbd_proxy) {
    g_warning ("Failed to get Cell Broadcast daemon proxy");
    return;
  }

  g_signal_connect_swapped (self->cbd_proxy,
                            "g-properties-changed",
                            G_CALLBACK (on_cbd_dbus_properties_changed),
                            self);
  var = g_dbus_proxy_get_cached_property (self->cbd_proxy, CBD_CBS_SUPPORTED);
  if (!var) {
    g_warning ("Failed to read initial " CBD_CBS_SUPPORTED " property from cbd");
    return;
  }
  cbs_supported = g_variant_get_boolean (var);
  set_has_cbs (self, cbs_supported);
}


static gboolean
has_cbs_to_visible_child_transform (GBinding     *binding,
                                    const GValue *from_value,
                                    GValue       *to_value,
                                    gpointer      user_data)
{
  gboolean has_cbs = g_value_get_boolean (from_value);
  const char *visible_child;

  visible_child = has_cbs ? "alerts" : "empty";
  g_value_set_string (to_value, visible_child);

  return TRUE;
}


static MsChannelLevel
row_pos_to_level (guint pos)
{
  MsChannelLevel level;
  /* The order in level_names matches the order in MsChannelLevels but the later is
     a set of flags and has CHANNEL_LEVEL_UNKONWN as first element */
  level = 1 << (pos + 1);

  g_assert (level <= MS_CHANNEL_LEVEL_TEST);

  return level;
}


static void
ms_alerts_panel_set_levels (MsAlertsPanel *self, MsChannelLevel levels)
{
  if (self->levels == levels)
    return;

  self->levels = levels;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LEVELS]);

  if (self->setting_levels)
    return;

  for (guint i = 0; i < G_N_ELEMENTS (level_names); i++) {
    gboolean active;
    MsChannelLevel level = row_pos_to_level (i);

    active = self->levels & level;
    adw_switch_row_set_active (self->rows[i], active);
  }
}


static MsChannelLevel
ms_alerts_panel_get_levels (MsAlertsPanel *self)
{
  return self->levels;
}


static void
on_switch_active_changed (MsAlertsPanel *self, GParamSpec *spec, AdwSwitchRow  *switch_)
{
  for (guint i = 0; i < G_N_ELEMENTS (level_names); i++) {
    if (switch_ == self->rows[i]) {
      gboolean active = adw_switch_row_get_active (switch_);
      MsChannelLevel levels = ms_alerts_panel_get_levels (self);
      MsChannelLevel changed_level = row_pos_to_level (i);

      self->setting_levels = TRUE;

      if (active) {
        levels |= changed_level;
      } else {
        levels &= ~changed_level;
      }

      /* Nation wide/presidential alerts must always be enabled */
      levels |= MS_CHANNEL_LEVEL_PRESIDENTIAL;
      ms_alerts_panel_set_levels (self, levels);

      self->setting_levels = FALSE;
      return;
    }
  }

  g_assert_not_reached ();
}


static void
ms_alerts_panel_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MsAlertsPanel *self = MS_ALERTS_PANEL (object);

  switch (property_id) {
  case PROP_HAS_CBS:
    self->has_cbs = g_value_get_boolean (value);
    break;
  case PROP_LEVELS:
    ms_alerts_panel_set_levels (self, g_value_get_flags (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_alerts_panel_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MsAlertsPanel *self = MS_ALERTS_PANEL (object);

  switch (property_id) {
  case PROP_HAS_CBS:
    g_value_set_boolean (value, self->has_cbs);
    break;
  case PROP_LEVELS:
    g_value_set_flags (value, self->levels);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_alerts_panel_finalize (GObject *object)
{
  MsAlertsPanel *self = MS_ALERTS_PANEL (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->cbd_proxy);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (ms_alerts_panel_parent_class)->finalize (object);
}


static void
ms_alerts_panel_class_init (MsAlertsPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ms_alerts_panel_get_property;
  object_class->set_property = ms_alerts_panel_set_property;
  object_class->finalize = ms_alerts_panel_finalize;

  /**
   * MsAlertsPanel:has-cbs:
   *
   * Whether a device on this system supports receiving Cell Broadcast
   * messages
   */
  props[PROP_HAS_CBS] =
    g_param_spec_boolean ("has-cbs", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * MsAlertsPanel:levels:
   *
   * The currently enabled alert levels.
   */
  props[PROP_LEVELS] =
    g_param_spec_flags ("levels", "", "",
                        MS_TYPE_CHANNEL_LEVEL,
                        MS_CHANNEL_LEVEL_UNKNOWN,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-alerts-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, MsAlertsPanel, stack);

  for (guint i = 0; i < G_N_ELEMENTS (level_names); i++) {
    g_autofree char *name = g_strdup_printf ("%s_alerts", level_names[i]);
    gtk_widget_class_bind_template_child_full (widget_class,
                                               name,
                                               FALSE,
                                               G_STRUCT_OFFSET (MsAlertsPanel, rows[i]));
  }

  gtk_widget_class_bind_template_callback (widget_class, on_switch_active_changed);
}


static void
ms_alerts_panel_init (MsAlertsPanel *self)
{
  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  g_autoptr (GSettingsSchema) schema = NULL;
  g_autoptr (GSettings) settings = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property_full (self, "has-cbs",
                               self->stack, "visible-child-name",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               has_cbs_to_visible_child_transform,
                               NULL,
                               NULL,
                               NULL);

  /* Without a schema there's no point to even check for cbd */
  schema = g_settings_schema_source_lookup (source, CBD_SCHEMA_ID, TRUE);
  if (!schema) {
    g_message ("Necessary schema for Cell Broadcasts not found");
    return;
  }

  self->settings = g_settings_new (CBD_SCHEMA_ID);
  g_settings_bind (self->settings, CBD_LEVELS_KEY, self, "levels", G_SETTINGS_BIND_DEFAULT);

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,
                            "org.freedesktop.cbd",
                            "/org/freedesktop/cbd",
                            "org.freedesktop.cbd",
                            self->cancel,
                            on_cbd_proxy_ready,
                            self);
}


MsAlertsPanel *
ms_alerts_panel_new (void)
{
  return MS_ALERTS_PANEL (g_object_new (MS_TYPE_ALERTS_PANEL, NULL));
}
