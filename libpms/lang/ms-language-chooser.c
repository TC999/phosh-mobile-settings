/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Written by: Matthias Clasen <mclasen@redhat.com>
 */

#define _GNU_SOURCE

#include "mobile-settings-config.h"

#include "ms-language-chooser.h"
#include "ms-language-row.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "ms-common-language.h"
#include "ms-util.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-languages.h>

struct _MsLanguageChooser {
        AdwDialog parent_instance;

        GtkSearchEntry *language_filter_entry;
        GtkListBox     *language_listbox;
        GtkListBoxRow  *more_row;
        GtkSearchBar   *search_bar;
        GtkButton      *select_button;

        gboolean showing_extra;
        gchar *language;
        gchar **filter_words;
};

enum {
        LANGUAGE_SELECTED,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MsLanguageChooser, ms_language_chooser, ADW_TYPE_DIALOG)

static void
add_all_languages (MsLanguageChooser *self)
{
        g_auto(GStrv) locale_ids = NULL;
        g_autoptr(GHashTable) initial = NULL;

        locale_ids = gnome_get_all_locales ();
        initial = ms_common_language_get_initial_languages ();
        for (int i = 0; locale_ids[i] != NULL; i++) {
                MsLanguageRow *row;
                gboolean is_initial;

                if (!ms_common_language_has_font (locale_ids[i]))
                        continue;

                row = ms_language_row_new (locale_ids[i]);
                is_initial = (g_hash_table_lookup (initial, locale_ids[i]) != NULL);
                ms_language_row_set_is_extra (row, !is_initial);
                gtk_list_box_prepend (self->language_listbox, GTK_WIDGET (row));
        }
}

static gboolean
match_all (gchar       **words,
           const gchar  *str)
{
        gchar **w;

        if (str == NULL)
                return FALSE;

        for (w = words; *w; ++w)
                if (!strstr (str, *w))
                        return FALSE;

        return TRUE;
}

static gboolean
language_visible (GtkListBoxRow *row,
                  gpointer   user_data)
{
        MsLanguageChooser *self = user_data;
        g_autofree gchar *language = NULL;
        g_autofree gchar *country = NULL;
        g_autofree gchar *language_local = NULL;
        g_autofree gchar *country_local = NULL;
        gboolean visible;

        if (row == self->more_row)
                return !self->showing_extra;

        if (!MS_IS_LANGUAGE_ROW (row))
                return TRUE;

        if (!self->showing_extra && ms_language_row_get_is_extra (MS_LANGUAGE_ROW (row)))
                return FALSE;

        if (!self->filter_words)
                return TRUE;

        language =
                ms_util_normalize_casefold_and_unaccent (ms_language_row_get_language (MS_LANGUAGE_ROW (row)));
        visible = match_all (self->filter_words, language);
        if (visible)
                return TRUE;

        country =
                ms_util_normalize_casefold_and_unaccent (ms_language_row_get_country (MS_LANGUAGE_ROW (row)));
        visible = match_all (self->filter_words, country);
        if (visible)
                return TRUE;

        language_local =
                ms_util_normalize_casefold_and_unaccent (ms_language_row_get_language_local (MS_LANGUAGE_ROW (row)));
        visible = match_all (self->filter_words, language_local);
        if (visible)
                return TRUE;

        country_local =
                ms_util_normalize_casefold_and_unaccent (ms_language_row_get_country_local (MS_LANGUAGE_ROW (row)));
        return match_all (self->filter_words, country_local);
}

static gint
sort_languages (GtkListBoxRow *a,
                GtkListBoxRow *b,
                gpointer   data)
{
        int d;

        if (!MS_IS_LANGUAGE_ROW (a))
                return 1;
        if (!MS_IS_LANGUAGE_ROW (b))
                return -1;

        d = g_strcmp0 (ms_language_row_get_language (MS_LANGUAGE_ROW (a)), ms_language_row_get_language (MS_LANGUAGE_ROW (b)));
        if (d != 0)
                return d;

        return g_strcmp0 (ms_language_row_get_country (MS_LANGUAGE_ROW (a)), ms_language_row_get_country (MS_LANGUAGE_ROW (b)));
}

static void
language_filter_entry_search_changed_cb (MsLanguageChooser *self)
{
        g_autofree gchar *filter_contents = NULL;

        g_clear_pointer (&self->filter_words, g_strfreev);

        filter_contents =
                ms_util_normalize_casefold_and_unaccent (gtk_editable_get_text (GTK_EDITABLE (self->language_filter_entry)));
        if (!filter_contents) {
                gtk_list_box_invalidate_filter (self->language_listbox);
                return;
        }
        self->filter_words = g_strsplit_set (g_strstrip (filter_contents), " ", 0);
        gtk_list_box_invalidate_filter (self->language_listbox);
}

static void
show_more (MsLanguageChooser *self, gboolean visible)
{
        gtk_search_bar_set_search_mode (self->search_bar, visible);
        gtk_widget_grab_focus (visible ? GTK_WIDGET (self->language_filter_entry) : GTK_WIDGET (self->language_listbox));

        self->showing_extra = visible;

        gtk_list_box_invalidate_filter (self->language_listbox);
}

static void
set_locale_id (MsLanguageChooser *self,
               const gchar       *locale_id)
{
        GtkWidget *child;

        gtk_widget_set_sensitive (GTK_WIDGET (self->select_button), FALSE);

        for (child = gtk_widget_get_first_child (GTK_WIDGET (self->language_listbox));
             child;
             child = gtk_widget_get_next_sibling (child)) {
                MsLanguageRow *row;

                if (!MS_IS_LANGUAGE_ROW (child))
                        continue;

                row = MS_LANGUAGE_ROW (child);
                if (g_strcmp0 (locale_id, ms_language_row_get_locale_id (row)) == 0) {
                        ms_language_row_set_checked (row, TRUE);
                        gtk_widget_set_sensitive (GTK_WIDGET (self->select_button), TRUE);

                        /* make sure the selected language is shown */
                        if (!self->showing_extra && ms_language_row_get_is_extra (row)) {
                                ms_language_row_set_is_extra (row, FALSE);
                                gtk_list_box_invalidate_filter (self->language_listbox);
                        }
                } else {
                        ms_language_row_set_checked (row, FALSE);
                }
        }

        g_free (self->language);
        self->language = g_strdup (locale_id);
}

static void
language_listbox_row_activated_cb (MsLanguageChooser *self, GtkListBoxRow *row)
{
        const gchar *new_locale_id;

        if (row == self->more_row) {
                show_more (self, TRUE);
                return;
        }

        if (!MS_IS_LANGUAGE_ROW (row))
                return;

        new_locale_id = ms_language_row_get_locale_id (MS_LANGUAGE_ROW (row));
        if (g_strcmp0 (new_locale_id, self->language) == 0) {
                g_signal_emit (self, signals[LANGUAGE_SELECTED], 0);
        } else {
                set_locale_id (self, new_locale_id);
        }
}

static void
select_button_clicked_cb (MsLanguageChooser *self)
{
        g_signal_emit (self, signals[LANGUAGE_SELECTED], 0);
}

void
ms_language_chooser_init (MsLanguageChooser *self)
{
        gtk_widget_init_template (GTK_WIDGET (self));

        gtk_list_box_set_sort_func (self->language_listbox,
                                    sort_languages, self, NULL);
        gtk_list_box_set_filter_func (self->language_listbox,
                                      language_visible, self, NULL);
        add_all_languages (self);

        gtk_list_box_invalidate_filter (self->language_listbox);
}

static void
ms_language_chooser_dispose (GObject *object)
{
        MsLanguageChooser *self = MS_LANGUAGE_CHOOSER (object);

        g_clear_pointer (&self->filter_words, g_strfreev);
        g_clear_pointer (&self->language, g_free);

        G_OBJECT_CLASS (ms_language_chooser_parent_class)->dispose (object);
}

void
ms_language_chooser_class_init (MsLanguageChooserClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->dispose = ms_language_chooser_dispose;

        signals[LANGUAGE_SELECTED] =
                g_signal_new ("language-selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              0);

        gtk_widget_class_set_template_from_resource (widget_class,
                                                     "/mobi/phosh/LibMobileSettings/ui/"
                                                     "ms-language-chooser.ui");

        gtk_widget_class_bind_template_child (widget_class, MsLanguageChooser, language_filter_entry);
        gtk_widget_class_bind_template_child (widget_class, MsLanguageChooser, language_listbox);
        gtk_widget_class_bind_template_child (widget_class, MsLanguageChooser, more_row);
        gtk_widget_class_bind_template_child (widget_class, MsLanguageChooser, search_bar);
        gtk_widget_class_bind_template_child (widget_class, MsLanguageChooser, select_button);

        gtk_widget_class_bind_template_callback (widget_class, language_filter_entry_search_changed_cb);
        gtk_widget_class_bind_template_callback (widget_class, language_listbox_row_activated_cb);
        gtk_widget_class_bind_template_callback (widget_class, select_button_clicked_cb);
}

MsLanguageChooser *
ms_language_chooser_new (void)
{
        return g_object_new (MS_TYPE_LANGUAGE_CHOOSER, NULL);
}

void
ms_language_chooser_clear_filter (MsLanguageChooser *self)
{
        g_return_if_fail (MS_IS_LANGUAGE_CHOOSER (self));
        gtk_editable_set_text (GTK_EDITABLE (self->language_filter_entry), "");
        show_more (self, FALSE);
}

const gchar *
ms_language_chooser_get_language (MsLanguageChooser *self)
{
        g_return_val_if_fail (MS_IS_LANGUAGE_CHOOSER (self), NULL);
        return self->language;
}

void
ms_language_chooser_set_language (MsLanguageChooser *self,
                                  const gchar       *language)
{
        g_return_if_fail (MS_IS_LANGUAGE_CHOOSER (self));
        set_locale_id (self, language);
}
