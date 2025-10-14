/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/ms-tweaks-preferences-page.c"

#include "utils/ms-tweaks-backend-dummy.h"


typedef struct {
  MsTweaksSetting      *setting;
  GValue *              value;
  MsTweaksCallbackMeta *callback_meta;
} PreferencesPageTestFixture;


#define DEBUG_SETTING_NAME "Debug"


static void
test_preferences_page_fixture_setup (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  fixture->setting = g_new0 (MsTweaksSetting, 1);
  fixture->value = g_new0 (GValue, 1);
  fixture->callback_meta = g_new (MsTweaksCallbackMeta, 1);
  fixture->callback_meta->backend_state = ms_tweaks_backend_dummy_new (fixture->setting);
  fixture->callback_meta->toast_overlay = ADW_TOAST_OVERLAY (adw_toast_overlay_new ());

  fixture->setting->name = DEBUG_SETTING_NAME;

  fixture->setting->min = 0;
  fixture->setting->max = 10;
  fixture->setting->step = 1;

  g_value_init (fixture->value, G_TYPE_STRING);
  g_value_set_string (fixture->value, "Put Your Money On Me");

  g_object_ref_sink (fixture->callback_meta->toast_overlay);
}


static void
test_preferences_page_fixture_setup_with_map (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  test_preferences_page_fixture_setup (fixture, unused);

  fixture->setting->map = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (fixture->setting->map, "Option 0", "Value 0");
  g_hash_table_insert (fixture->setting->map, "Option 1", "Value 1");
  g_hash_table_insert (fixture->setting->map, "Option 2", "Value 2");
}


static void
test_preferences_page_fixture_teardown (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  if (fixture->setting->map)
    g_hash_table_destroy (fixture->setting->map);
  g_free (fixture->setting);
  g_clear_object (&fixture->callback_meta->toast_overlay);
  g_clear_object (&fixture->callback_meta->backend_state);
  g_free (fixture->callback_meta);
  g_value_unset (fixture->value);
  g_free (fixture->value);
}


#define PREFERENCES_PAGE_TEST_ADD(name, test_func) g_test_add ((name), \
                                                               PreferencesPageTestFixture, \
                                                               NULL, \
                                                               test_preferences_page_fixture_setup, \
                                                               (test_func), \
                                                               test_preferences_page_fixture_teardown)

#define PREFERENCES_PAGE_TEST_ADD_WITH_MAP(name, test_func) g_test_add ((name), \
                                                                        PreferencesPageTestFixture, \
                                                                        NULL, \
                                                                        test_preferences_page_fixture_setup_with_map, \
                                                                        (test_func), \
                                                                        test_preferences_page_fixture_teardown)


static void
test_get_keys_from_hash_table (void)
{
  GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
  g_autoptr (GtkStringList) key_list = NULL;

  g_hash_table_insert (hash_table, "Key 1", "unused");
  g_hash_table_insert (hash_table, "Keysus 2", "unused");
  g_hash_table_insert (hash_table, "3 Keyzzz", "unused");

  key_list = get_keys_from_hashtable (hash_table);

  g_assert_true (key_list);
  /* Hash tables don't guarantee order, so just ensure that the keys are in the table at all. */
  g_assert_cmpint (gtk_string_list_find (key_list, "Key 1"), <, 3);
  g_assert_cmpint (gtk_string_list_find (key_list, "Keysus 2"), <, 3);
  g_assert_cmpint (gtk_string_list_find (key_list, "3 Keyzzz"), <, 3);

  g_hash_table_destroy (hash_table);
}


static void
test_setting_data_to_boolean_widget (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_boolean_widget (fixture->setting,
                                                      NULL,
                                                      fixture->callback_meta);

  g_assert_true (ADW_IS_SWITCH_ROW (widget));
  g_assert_false (adw_switch_row_get_active (ADW_SWITCH_ROW (widget)));

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_boolean_widget_with_value (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GValue *value = g_new0 (GValue, 1);
  GtkWidget *widget_1 = NULL;
  GtkWidget *widget_2 = NULL;

  g_value_init (value, G_TYPE_BOOLEAN);

  g_value_set_boolean (value, TRUE);
  widget_1 = setting_data_to_boolean_widget (fixture->setting, value, fixture->callback_meta);

  g_value_set_boolean (value, FALSE);
  widget_2 = setting_data_to_boolean_widget (fixture->setting, value, fixture->callback_meta);

  g_assert_true (ADW_IS_SWITCH_ROW (widget_1));
  g_assert_true (ADW_IS_SWITCH_ROW (widget_2));
  g_assert_true (adw_switch_row_get_active (ADW_SWITCH_ROW (widget_1)));
  g_assert_false (adw_switch_row_get_active (ADW_SWITCH_ROW (widget_2)));

  g_object_ref_sink (widget_1);
  g_object_ref_sink (widget_2);
  g_object_unref (widget_1);
  g_object_unref (widget_2);
  g_free (value);
}


static void
test_setting_data_to_choice_widget (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  g_test_expect_message ("ms-tweaks-preferences-page",
                         G_LOG_LEVEL_WARNING,
                         "[Setting '" DEBUG_SETTING_NAME "'] Choice widget with NULL map — either the datasource failed or the markup is wrong");


  g_assert_false (setting_data_to_choice_widget (fixture->setting, NULL, fixture->callback_meta));

  g_test_assert_expected_messages ();
}


static void
test_setting_data_to_choice_widget_with_map (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = NULL;

  widget = setting_data_to_choice_widget (fixture->setting, NULL, fixture->callback_meta);

  g_assert_true (ADW_IS_COMBO_ROW (widget));

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_choice_widget_with_map_and_value (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkStringObject *string_object = NULL;
  GtkWidget *widget_1 = NULL;
  GtkWidget *widget_2 = NULL;

  widget_1 = setting_data_to_choice_widget (fixture->setting,
                                            fixture->value,
                                            fixture->callback_meta);

  g_value_set_string (fixture->value, "Value 2");
  widget_2 = setting_data_to_choice_widget (fixture->setting,
                                            fixture->value,
                                            fixture->callback_meta);

  g_assert_true (ADW_IS_COMBO_ROW (widget_1));
  g_assert_true (ADW_IS_COMBO_ROW (widget_2));

  string_object = adw_combo_row_get_selected_item (ADW_COMBO_ROW (widget_1));
  g_assert_cmpstr (gtk_string_object_get_string (string_object), ==, "Option 1");

  string_object = adw_combo_row_get_selected_item (ADW_COMBO_ROW (widget_2));
  g_assert_cmpstr (gtk_string_object_get_string (string_object), ==, "Option 2");

  g_object_ref_sink (widget_1);
  g_object_ref_sink (widget_2);
  g_object_unref (widget_1);
  g_object_unref (widget_2);
}


static void
test_setting_data_to_file_widget (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPreferencesPageFilePickerMeta *metadata = g_new (MsTweaksPreferencesPageFilePickerMeta, 1);
  GtkWidget *widget = setting_data_to_file_widget (fixture->setting,
                                                   fixture->callback_meta->backend_state,
                                                   NULL,
                                                   ADW_TOAST_OVERLAY (fixture->callback_meta->toast_overlay),
                                                   metadata);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
  g_object_ref_sink (metadata->file_picker_label);
  g_object_unref (metadata->file_picker_label);
  g_free (metadata);
}


static void
test_setting_data_to_file_widget_with_value (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPreferencesPageFilePickerMeta *metadata = g_new (MsTweaksPreferencesPageFilePickerMeta, 1);
  GtkWidget *widget = setting_data_to_file_widget (fixture->setting,
                                                   fixture->callback_meta->backend_state,
                                                   fixture->value,
                                                   ADW_TOAST_OVERLAY (fixture->callback_meta->toast_overlay),
                                                   metadata);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
  g_object_ref_sink (metadata->file_picker_label);
  g_object_unref (metadata->file_picker_label);
  g_free (metadata);
}


static void
test_setting_data_to_font_widget (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_font_widget (fixture->setting, NULL, fixture->callback_meta);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_font_widget_with_value (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_font_widget (fixture->setting,
                                                   fixture->value,
                                                   fixture->callback_meta);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_info_widget (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  g_test_expect_message ("ms-tweaks-preferences-page",
                         G_LOG_LEVEL_WARNING,
                         "[Setting '" DEBUG_SETTING_NAME "'] widget_value was NULL in setting_data_to_boolean_widget ()");

  g_assert_false (setting_data_to_info_widget (fixture->setting, NULL));
}


static void
test_setting_data_to_info_widget_with_value (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_info_widget (fixture->setting, fixture->value);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_number_widget (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GtkWidget *widget = setting_data_to_number_widget (fixture->setting,
                                                     NULL,
                                                     fixture->callback_meta);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
}


static void
test_setting_data_to_number_widget_with_value (PreferencesPageTestFixture *fixture, gconstpointer unused)
{
  GValue value = G_VALUE_INIT;
  GtkWidget *widget = NULL;

  g_value_init (&value, G_TYPE_DOUBLE);
  g_value_set_double (&value, 5);
  widget = setting_data_to_number_widget (fixture->setting, &value, fixture->callback_meta);

  g_assert_true (widget);

  g_object_ref_sink (widget);
  g_object_unref (widget);
  g_value_unset (&value);
}


static void
test_construct_empty_preferences_page (void)
{
  MsTweaksPage page = {
    .weight = 999999,
    .name = "Really heavy page",
    .section_table = g_hash_table_new (g_str_hash, g_str_equal),
    .name_i18n = "Riktigt tung sida",
  };

  g_assert_null (ms_tweaks_preferences_page_new (&page));

  g_hash_table_unref (page.section_table);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-get-keys-from-hash-table",
                   test_get_keys_from_hash_table);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-boolean-widget",
                             test_setting_data_to_boolean_widget);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-boolean-widget-with-value",
                             test_setting_data_to_boolean_widget_with_value);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-choice-widget",
                             test_setting_data_to_choice_widget);
  PREFERENCES_PAGE_TEST_ADD_WITH_MAP ("/phosh-mobile-settings/test-tweaks-setting-data-to-choice-widget-with-map",
                                      test_setting_data_to_choice_widget_with_map);
  PREFERENCES_PAGE_TEST_ADD_WITH_MAP ("/phosh-mobile-settings/test-tweaks-setting-data-to-choice-widget-with-map-and-value",
                                      test_setting_data_to_choice_widget_with_map_and_value);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-file-widget",
                             test_setting_data_to_file_widget);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-file-widget-with-value",
                             test_setting_data_to_file_widget_with_value);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-font-widget",
                             test_setting_data_to_font_widget);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-font-widget-with-value",
                             test_setting_data_to_font_widget_with_value);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-info-widget",
                             test_setting_data_to_info_widget);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-info-widget-with-value",
                             test_setting_data_to_info_widget_with_value);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-number-widget",
                             test_setting_data_to_number_widget);
  PREFERENCES_PAGE_TEST_ADD ("/phosh-mobile-settings/test-tweaks-setting-data-to-number-widget-with-value",
                             test_setting_data_to_number_widget_with_value);
  g_test_add_func ("/phosh-mobile-settings/test-construct-empty-preferences-page",
                   test_construct_empty_preferences_page);

  return g_test_run ();
}
