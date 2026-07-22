/*
 * Copyright (C) 2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "ms-hidden-app-row"

#include "mobile-settings-config.h"

#include "ms-hidden-app-row.h"


enum {
  PROP_0,
  PROP_FILENAME,
  PROP_ICON_NAME,
  PROP_SELECTED,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _MsHiddenAppRow {
  AdwActionRow    parent;

  GtkCheckButton *selected_button;
  GtkImage       *icon;

  char *filename;
  char *icon_name;
  gboolean        selected;

};
G_DEFINE_TYPE (MsHiddenAppRow, ms_hidden_app_row, ADW_TYPE_ACTION_ROW)


static void
ms_hidden_app_row_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MsHiddenAppRow *self = MS_HIDDEN_APP_ROW (object);

  switch (property_id) {
  case PROP_FILENAME:
    g_set_str (&self->filename, g_value_get_string (value));
    break;
  case PROP_ICON_NAME:
    g_set_str (&self->icon_name, g_value_get_string (value));
    break;
  case PROP_SELECTED:
    self->selected = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_hidden_app_row_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MsHiddenAppRow *self = MS_HIDDEN_APP_ROW (object);

  switch (property_id) {
  case PROP_FILENAME:
    g_value_set_string (value, self->filename);
    break;
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_SELECTED:
    g_value_set_boolean (value, self->selected);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
ms_hidden_app_row_finalize (GObject *object)
{
  MsHiddenAppRow *self = MS_HIDDEN_APP_ROW (object);

  g_clear_pointer (&self->filename, g_free);
  g_clear_pointer (&self->icon_name, g_free);

  G_OBJECT_CLASS (ms_hidden_app_row_parent_class)->finalize (object);
}


static void
ms_hidden_app_row_class_init (MsHiddenAppRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ms_hidden_app_row_finalize;
  object_class->get_property = ms_hidden_app_row_get_property;
  object_class->set_property = ms_hidden_app_row_set_property;

  props[PROP_FILENAME] =
    g_param_spec_string ("filename", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_SELECTED] =
    g_param_spec_boolean ("selected", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/"
                                               "ui/ms-hidden-app-row.ui");

  gtk_widget_class_bind_template_child (widget_class, MsHiddenAppRow, icon);
  gtk_widget_class_bind_template_child (widget_class, MsHiddenAppRow, selected_button);
}


static void
ms_hidden_app_row_init (MsHiddenAppRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self, "icon-name",
                          self->icon, "icon-name",
                          G_BINDING_DEFAULT);

  g_object_bind_property (self->selected_button, "active",
                          self, "selected",
                          G_BINDING_DEFAULT);
}


const char *
ms_hidden_app_row_get_filename (MsHiddenAppRow *self)
{
  g_return_val_if_fail (MS_IS_HIDDEN_APP_ROW (self), NULL);

  return self->filename;
}


gboolean
ms_hidden_app_row_get_selected (MsHiddenAppRow *self)
{
  g_return_val_if_fail (MS_IS_HIDDEN_APP_ROW (self), FALSE);

  return self->selected;
}
