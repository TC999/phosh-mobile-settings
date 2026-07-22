/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/ms-tweaks-mappings.c"


typedef struct {
  GValue          *value;
  MsTweaksSetting *setting;
} MappingTestFixture;


static void
test_mappings_stringify_boolean_true (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value_to_stringify, TRUE);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "true");
}


static void
test_mappings_stringify_boolean_false (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value_to_stringify, FALSE);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "false");
}


static void
test_mappings_stringify_double (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_DOUBLE);
  g_value_set_double (&value_to_stringify, 6.7);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "6.700000");
}


static void
test_mappings_stringify_flags (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_FLAGS);
  /* Flags are just unsigned integers. */
  g_value_set_flags (&value_to_stringify, 3);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "3");
}


static void
test_mappings_stringify_float (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_FLOAT);
  g_value_set_float (&value_to_stringify, 3.14f);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "3.140000");
}


static void
test_mappings_stringify_int (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_INT);
  g_value_set_int (&value_to_stringify, -20);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "-20");
}


static void
test_mappings_stringify_string (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_STRING);
  g_value_set_string (&value_to_stringify, "Tannefors");

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "Tannefors");
}


static void
test_mappings_stringify_uint (void)
{
  g_auto (GValue) value_to_stringify = G_VALUE_INIT;
  g_autofree char *value_stringified = NULL;

  g_value_init (&value_to_stringify, G_TYPE_UINT);
  g_value_set_uint (&value_to_stringify, 1984);

  value_stringified = stringify_gvalue (&value_to_stringify);

  g_assert_cmpstr (value_stringified, ==, "1984");
}


static void
test_mappings_fixture_setup (MappingTestFixture *fixture, gconstpointer unused)
{
  fixture->value = g_new0 (GValue, 1);
  fixture->setting = g_new0 (MsTweaksSetting, 1);
}


static void
test_mappings_fixture_setup_with_map (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup (fixture, unused);
  fixture->setting->map = g_hash_table_new (g_str_hash, g_str_equal);
}


#define BOOLEAN_TRUE_STRING "straight fax"
#define BOOLEAN_FALSE_STRING "that's cap"


static void
test_mappings_fixture_setup_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup_with_map (fixture, unused);

  fixture->setting->type = MS_TWEAKS_TYPE_BOOLEAN;

  /* Valid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "true", BOOLEAN_TRUE_STRING);
  g_hash_table_insert (fixture->setting->map, "false", BOOLEAN_FALSE_STRING);

  /* Invalid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "what", "what the heck");
  g_hash_table_insert (fixture->setting->map, "sorry", "it is okay");
}


#define CHOICE_KEY_STRING_1 "a key"
#define CHOICE_KEY_STRING_2 "yet another"
#define CHOICE_VALUE_STRING_1 "some value"
#define CHOICE_VALUE_STRING_2 "anyhow, another one"


static void
test_mappings_fixture_setup_choice (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup_with_map (fixture, unused);

  fixture->setting->type = MS_TWEAKS_TYPE_CHOICE;

  g_hash_table_insert (fixture->setting->map, CHOICE_KEY_STRING_1, CHOICE_VALUE_STRING_1);
  g_hash_table_insert (fixture->setting->map, CHOICE_KEY_STRING_2, CHOICE_VALUE_STRING_2);
}


static void
test_mappings_fixture_setup_number (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup (fixture, unused);

  fixture->setting->type = MS_TWEAKS_TYPE_NUMBER;
}


static void
test_mappings_fixture_setup_gtype_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  test_mappings_fixture_setup_with_map (fixture, unused);

  /* Test GType handling. */
  fixture->setting->backend = MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS;

  /* Test boolean GType handling. */
  fixture->setting->gtype = MS_TWEAKS_GTYPE_BOOLEAN;

  /* Valid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "tea_is_nice", "true");
  g_hash_table_insert (fixture->setting->map, "pollution_is_nice", "false");

  /* Invalid string -> boolean mappings. */
  g_hash_table_insert (fixture->setting->map, "towels", "what the heck");
  g_hash_table_insert (fixture->setting->map, "sorry", "it is okay");
}


static void
test_mappings_fixture_teardown (MappingTestFixture *fixture, gconstpointer unused)
{
  if (fixture->setting->map)
    g_hash_table_destroy (fixture->setting->map);

  g_value_unset (fixture->value);
  g_free (fixture->value);
  g_free (fixture->setting);
}


#define MAPPING_TEST_ADD(name, setup_func, test_func) g_test_add ((name), \
                                                                  MappingTestFixture, \
                                                                  NULL, \
                                                                  (setup_func), \
                                                                  (test_func), \
                                                                  test_mappings_fixture_teardown)

/**
 * test_mappings_handle_get_boolean_type_choice:
 * Ensures that mappings aren't applied for "choice" type widgets.
 */
static void
test_mappings_handle_get_boolean_type_choice (MappingTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) error = NULL;
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, CHOICE_VALUE_STRING_1);
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_cmpstr (g_value_get_string (value), ==, CHOICE_VALUE_STRING_1);
  g_assert_no_error (error);
}


static void
test_mappings_handle_set_boolean_type_choice (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, CHOICE_KEY_STRING_2);
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_cmpstr (g_value_get_string (value), ==, CHOICE_KEY_STRING_2);
}


#define TEST_FLOAT_DOUBLE_REPR 34.4
#define TEST_FLOAT_STRING_REPR "34.400000"
#define TEST_FLOAT_STRING_REPR_SHORT "34.4"


static void
test_mappings_handle_get_type_number (MappingTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) error = NULL;
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_FLOAT_STRING_REPR);
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_cmpfloat_with_epsilon (g_value_get_double (value), TEST_FLOAT_DOUBLE_REPR, FLT_EPSILON);
  g_assert_no_error (error);

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_FLOAT_STRING_REPR_SHORT);
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_cmpfloat_with_epsilon (g_value_get_double (value), TEST_FLOAT_DOUBLE_REPR, FLT_EPSILON);
  g_assert_no_error (error);
}


static void
test_mappings_handle_set_type_number (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, TEST_FLOAT_DOUBLE_REPR);
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_cmpstr (TEST_FLOAT_STRING_REPR, ==, g_value_get_string (value));
}


static void
test_mappings_handle_get_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) error = NULL;
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  /* Test valid mappings. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, BOOLEAN_TRUE_STRING);
  ms_tweaks_mappings_handle_get (value, setting, &error);
  g_assert_true (g_value_get_boolean (value));
  g_assert_no_error (error);

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, BOOLEAN_FALSE_STRING);
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_false (g_value_get_boolean (value));
  g_assert_no_error (error);

  /* Test invalid mappings. These always result in the value being set to false and no error being
   * logged. Might be worth logging some error and testing that it happens in the future. */
  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "what the heck");
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_false (g_value_get_boolean (value));
  g_assert_no_error (error);

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "it is okay");
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_false (g_value_get_boolean (value));
  g_assert_no_error (error);

  g_value_unset (value);

  /* Test nonexistent mappings. These always result in an error being returned. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "this_is_the_wrong_place_for_me");
  g_assert_false (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_error (error,
                  MS_TWEAKS_MAPPINGS_ERROR,
                  MS_TWEAKS_MAPPINGS_ERROR_FAILED_TO_FIND_KEY_BY_VALUE);
}

/**
 * test_mappings_handle_set_gtype_boolean:
 * `gtype` affects the value rather than the key (compared to `type`), so we should expect to input
 * anything and get a boolean back.
 */
static void
test_mappings_handle_set_gtype_boolean (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  /* Test valid mappings. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "tea_is_nice");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_true (g_value_get_boolean (value));

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "pollution_is_nice");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_false (g_value_get_boolean (value));

  g_value_unset (value);

  /* Test invalid mappings. These always result in the value being set to false and no error being
   * logged. Might be worth logging some error and testing that it happens in the future. */
  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "towels");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_false (g_value_get_boolean (value));

  g_value_unset (value);

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, "sorry");
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_false (g_value_get_boolean (value));
}


#define TEST_STRING "Stadsbana"


static void
test_mappings_handle_get_null_map (MappingTestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) error = NULL;
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_STRING);
  g_assert_true (ms_tweaks_mappings_handle_get (value, setting, &error));
  g_assert_cmpstr (g_value_get_string (value), ==, TEST_STRING);
  g_assert_no_error (error);
}


static void
test_mappings_handle_set_null_map (MappingTestFixture *fixture, gconstpointer unused)
{
  GValue *value = fixture->value;
  MsTweaksSetting *setting = fixture->setting;

  g_value_init (value, G_TYPE_STRING);
  g_value_set_string (value, TEST_STRING);
  ms_tweaks_mappings_handle_set (value, setting);
  g_assert_cmpstr (g_value_get_string (value), ==, TEST_STRING);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-boolean-true",
                   test_mappings_stringify_boolean_true);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-boolean-false",
                   test_mappings_stringify_boolean_false);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-double",
                   test_mappings_stringify_double);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-flags",
                   test_mappings_stringify_flags);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-float",
                   test_mappings_stringify_float);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-int",
                   test_mappings_stringify_int);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-string",
                   test_mappings_stringify_string);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-mappings-stringify-uint",
                   test_mappings_stringify_uint);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-boolean-type-choice",
                    test_mappings_fixture_setup_choice,
                    test_mappings_handle_get_boolean_type_choice);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-boolean-type-choice",
                    test_mappings_fixture_setup_choice,
                    test_mappings_handle_set_boolean_type_choice);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-type-number",
                    test_mappings_fixture_setup_number,
                    test_mappings_handle_get_type_number);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-type-number",
                    test_mappings_fixture_setup_number,
                    test_mappings_handle_set_type_number);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-boolean",
                    test_mappings_fixture_setup_boolean,
                    test_mappings_handle_get_boolean);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-gtype-boolean",
                    test_mappings_fixture_setup_gtype_boolean,
                    test_mappings_handle_set_gtype_boolean);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-get-null-map",
                    test_mappings_fixture_setup,
                    test_mappings_handle_get_null_map);
  MAPPING_TEST_ADD ("/phosh-mobile-settings/test-tweaks-mappings-handle-set-null-map",
                    test_mappings_fixture_setup,
                    test_mappings_handle_set_null_map);

  return g_test_run ();
}
