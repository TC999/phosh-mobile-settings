/*
 * Copyright (C) 2025 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Rudra Pratap Singh <rudrasingh900@gmail.com>
 */

#include "ms-welcome-panel.h"

struct _MsWelcomePanel {
  MsPanel parent;
};

G_DEFINE_TYPE (MsWelcomePanel, ms_welcome_panel, MS_TYPE_PANEL)


static void
ms_welcome_panel_class_init (MsWelcomePanelClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/MobileSettings/ui/ms-welcome-panel.ui");
}


static void
ms_welcome_panel_init (MsWelcomePanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
