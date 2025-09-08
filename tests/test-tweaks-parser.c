/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#include "conf-tweaks/ms-tweaks-parser.c"


typedef struct {
  MsTweaksParser *parser;
  GError         *error;
} ParserTestFixture;


#define parse(yaml_definition) ms_tweaks_parser_parse_fragment (fixture->parser, \
                                                                (guchar *) (yaml_definition), \
                                                                strlen (yaml_definition), \
                                                                &fixture->error)


static void
test_copy_setting (void)
{
  MsTweaksSetting *setting = g_new0 (MsTweaksSetting, 1);
  MsTweaksSetting *setting_copied = NULL;

  g_assert_true (setting);

  setting->weight = 999;
  setting->name = g_strdup ("Treeness");
  setting->type = MS_TWEAKS_TYPE_BOOLEAN;
  setting->gtype = MS_TWEAKS_GTYPE_FLAGS;
  setting->stype = MS_TWEAKS_STYPE_INT;
  setting->map = g_hash_table_new (g_str_hash, g_str_equal);
  setting->backend = MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS;
  setting->help = g_strdup ("Whether the device is a tree");
  setting->default_ = g_strdup ("Yes!!!!");
  setting->key = g_ptr_array_new ();
  setting->readonly = TRUE;
  setting->source_ext = TRUE;
  setting->selector = g_strdup ("is-tree");
  setting->guard = g_strdup ("IS-TREE");
  setting->multiplier = 9001;
  setting->min = 0.5;
  setting->max = 1.5;
  setting->step = 0.1;
  setting->css = g_hash_table_new (g_str_hash, g_str_equal);

  setting_copied = ms_tweaks_setting_copy (setting);

  g_assert_true (setting_copied);
  g_assert_true (setting != setting_copied);

  /* The properties map, key, and css should intentionally point to the same data structures
   * and be memory managed via refcounts as we do not care about making deep copies. */
  g_assert_cmpint (setting->weight, ==, setting_copied->weight);
  g_assert_true (setting->name != setting_copied->name);
  g_assert_cmpstr (setting->name, ==, setting_copied->name);
  g_assert_cmpint (setting->type, ==, setting_copied->type);
  g_assert_cmpint (setting->gtype, ==, setting_copied->gtype);
  g_assert_cmpint (setting->stype, ==, setting_copied->stype);
  g_assert_true (setting->map == setting_copied->map);
  g_assert_cmpint (setting->backend, ==, setting_copied->backend);
  g_assert_true (setting->help != setting_copied->help);
  g_assert_cmpstr (setting->help, ==, setting_copied->help);
  g_assert_true (setting->default_ != setting_copied->default_);
  g_assert_cmpstr (setting->default_, ==, setting_copied->default_);
  g_assert_true (setting->key == setting_copied->key);
  g_assert_cmpint (setting->readonly, ==, setting_copied->readonly);
  g_assert_cmpint (setting->source_ext, ==, setting_copied->source_ext);
  g_assert_true (setting->selector != setting_copied->selector);
  g_assert_cmpstr (setting->selector, ==, setting_copied->selector);
  g_assert_true (setting->guard != setting_copied->guard);
  g_assert_cmpstr (setting->guard, ==, setting_copied->guard);
  g_assert_cmpint (setting->multiplier, ==, setting_copied->multiplier);
  g_assert_cmpfloat_with_epsilon (setting->min, setting_copied->min, FLT_TRUE_MIN);
  g_assert_cmpfloat_with_epsilon (setting->max, setting_copied->max, FLT_TRUE_MIN);
  g_assert_cmpfloat_with_epsilon (setting->step, setting_copied->step, FLT_TRUE_MIN);
  g_assert_true (setting->css == setting_copied->css);

  ms_tweaks_setting_free (setting_copied);
  ms_tweaks_setting_free (setting);
}


static void
test_copy_setting_empty (void)
{
  MsTweaksSetting *setting = g_new0 (MsTweaksSetting, 1);
  MsTweaksSetting *setting_copied = NULL;

  g_assert_true (setting);

  /* The key field is expected to never be NULL, so populate it. */
  setting->key = g_ptr_array_new ();

  setting_copied = ms_tweaks_setting_copy (setting);

  g_assert_true (setting_copied);
  g_assert_true (setting != setting_copied);

  g_assert_cmpint (setting->weight, ==, setting_copied->weight);
  g_assert_true (setting->name == setting_copied->name);
  g_assert_cmpstr (setting->name, ==, setting_copied->name);
  g_assert_cmpint (setting->type, ==, setting_copied->type);
  g_assert_cmpint (setting->gtype, ==, setting_copied->gtype);
  g_assert_cmpint (setting->stype, ==, setting_copied->stype);
  g_assert_true (setting->map == setting_copied->map);
  g_assert_cmpint (setting->backend, ==, setting_copied->backend);
  g_assert_true (setting->help == setting_copied->help);
  g_assert_cmpstr (setting->help, ==, setting_copied->help);
  g_assert_true (setting->default_ == setting_copied->default_);
  g_assert_cmpstr (setting->default_, ==, setting_copied->default_);
  g_assert_true (setting->key == setting_copied->key);
  g_assert_cmpint (setting->readonly, ==, setting_copied->readonly);
  g_assert_cmpint (setting->source_ext, ==, setting_copied->source_ext);
  g_assert_true (setting->selector == setting_copied->selector);
  g_assert_cmpstr (setting->selector, ==, setting_copied->selector);
  g_assert_true (setting->guard == setting_copied->guard);
  g_assert_cmpstr (setting->guard, ==, setting_copied->guard);
  g_assert_cmpint (setting->multiplier, ==, setting_copied->multiplier);
  g_assert_cmpfloat_with_epsilon (setting->min, setting_copied->min, FLT_TRUE_MIN);
  g_assert_cmpfloat_with_epsilon (setting->max, setting_copied->max, FLT_TRUE_MIN);
  g_assert_cmpfloat_with_epsilon (setting->step, setting_copied->step, FLT_TRUE_MIN);
  g_assert_true (setting->css == setting_copied->css);

  ms_tweaks_setting_free (setting_copied);
  ms_tweaks_setting_free (setting);
}


static void
test_copy_section (void)
{
  MsTweaksSection *section = g_new0 (MsTweaksSection, 1);
  MsTweaksSection *section_copied = NULL;

  g_assert_true (section);

  section->weight = 42;
  section->name = g_strdup ("Fauna");
  section->setting_table = g_hash_table_new (g_str_hash, g_str_equal);

  section_copied = ms_tweaks_section_copy (section);

  g_assert_true (section_copied);
  g_assert_true (section != section_copied);

  g_assert_cmpint (section->weight, ==, section_copied->weight);
  g_assert_true (section->name != section_copied->name);
  g_assert_cmpstr (section->name, ==, section_copied->name);
  g_assert_true (section->setting_table == section_copied->setting_table);

  ms_tweaks_section_free (section_copied);
  ms_tweaks_section_free (section);
}

static void
test_copy_page (void)
{
  MsTweaksPage *page = g_new0 (MsTweaksPage, 1);
  MsTweaksPage *page_copied = NULL;

  g_assert_true (page);

  page->weight = 2000;
  page->name = g_strdup ("Pagure");
  page->section_table = g_hash_table_new (g_str_hash, g_str_equal);

  page_copied = ms_tweaks_page_copy (page);

  g_assert_true (page_copied);
  g_assert_true (page != page_copied);

  g_assert_cmpint (page->weight, ==, page->weight);
  g_assert_true (page->name != page_copied->name);
  g_assert_cmpstr (page->name, ==, page_copied->name);
  g_assert_true (page->section_table == page_copied->section_table);

  ms_tweaks_page_free (page_copied);
  ms_tweaks_page_free (page);
}

/**
 * test_parse_nothing:
 * Ensures that trying to parse a nonexistent path is handled gracefully.
 */
static void
test_parse_nothing (ParserTestFixture *fixture, gconstpointer unused)
{
  ms_tweaks_parser_parse_definition_files (fixture->parser, "nonexistent path");

  g_assert_true (fixture->parser->page_table);
  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 0);
}


static void
test_parse_basic (ParserTestFixture *fixture, gconstpointer unused)
{
  parse ("- name: Phosh");

  g_assert_true (fixture->parser->page_table);
  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);
  g_assert_true (g_hash_table_lookup (fixture->parser->page_table, "Phosh"));
}


static void
test_parse_section (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *soup_page = NULL;
  MsTweaksSection *vegetable_section = NULL;

  parse (
    "- name: Soup\n"
    "  weight: 25\n"
    "  sections:\n"
    "    - name: Vegetable\n"
    "      weight: 99\n");

  g_assert_true (fixture->parser->page_table);
  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);

  soup_page = g_hash_table_lookup (fixture->parser->page_table, "Soup");

  g_assert_true (soup_page);
  g_assert_cmpint (soup_page->weight, ==, 25);
  g_assert_cmpint (g_hash_table_size (soup_page->section_table), ==, 1);

  vegetable_section = g_hash_table_lookup (soup_page->section_table, "Vegetable");

  g_assert_true (vegetable_section);
  g_assert_cmpint (vegetable_section->weight, ==, 99);
  g_assert_true (vegetable_section->setting_table);
  g_assert_cmpint (g_hash_table_size (vegetable_section->setting_table), ==, 0);
}


static void
test_parse_setting (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *knome_page = NULL;
  MsTweaksSection *menus_section = NULL;
  MsTweaksSetting *knomeness_setting = NULL;

  parse (
    "- name: Knome\n"
    "  weight: 42\n"
    "  sections:\n"
    "    - name: Menus\n"
    "      weight: 33\n"
    "      settings:\n"
    "        - name: Knomeness\n"
    "          weight: 100\n");

  knome_page = g_hash_table_lookup (fixture->parser->page_table, "Knome");

  g_assert_true (knome_page);

  menus_section = g_hash_table_lookup (knome_page->section_table, "Menus");

  g_assert_true (menus_section);

  knomeness_setting = g_hash_table_lookup (menus_section->setting_table, "Knomeness");

  g_assert_true (knomeness_setting);
  g_assert_cmpint (knomeness_setting->weight, ==, 100);
}


static void
test_parse_multiple_settings (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *appearance_page = NULL;
  MsTweaksSection *gtk_section = NULL;
  MsTweaksSetting *legacy_prefer_dark_setting = NULL;
  MsTweaksSetting *icons_setting = NULL;

  parse (
    "- name: Appearance\n"
    "  weight: 30\n"
    "  sections:\n"
    "    - name: GTK\n"
    "      weight: 0\n"
    "      settings:\n"
    "        - name: Style\n"
    "          type: choice\n"
    "          gtype: string\n"
    "          help: Prefer dark or light for Adwaita applications\n"
    "          backend: gsettings\n"
    "          key: org.gnome.desktop.interface.color-scheme\n"
    "          default: \"default\"\n"
    "          map:\n"
    "            Default: default\n"
    "            Light: prefer-light\n"
    "            Dark: prefer-dark\n"
    "        - name: Legacy prefer dark\n"
    "          type: boolean\n"
    "          help: Use dark version of the theme. Only some GTK3 applications will react to this setting.\n"
    "          backend: gtk3settings\n"
    "          key: gtk-application-prefer-dark-theme\n"
    "          default: \"0\"\n"
    "          map:\n"
    "            true: \"1\"\n"
    "            false: \"0\"\n"
    "        - name: Theme\n"
    "          type: choice\n"
    "          backend: gsettings\n"
    "          default: Adwaita\n"
    "          help: The theme for GTK applications\n"
    "          gtype: string\n"
    "          key: org.gnome.desktop.interface.gtk-theme\n"
    "          data: gtk3themes\n"
    "        - name: Icons\n"
    "          type: choice\n"
    "          backend: gsettings\n"
    "          default: Adwaita\n"
    "          help: The icon theme for GTK applications\n"
    "          gtype: string\n"
    "          key: org.gnome.desktop.interface.icon-theme\n"
    "          data: iconthemes\n");

  appearance_page = g_hash_table_lookup (fixture->parser->page_table, "Appearance");

  g_assert_true (appearance_page);

  gtk_section = g_hash_table_lookup (appearance_page->section_table, "GTK");

  g_assert_true (gtk_section);

  legacy_prefer_dark_setting = g_hash_table_lookup (gtk_section->setting_table,
                                                    "Legacy prefer dark");
  icons_setting = g_hash_table_lookup (gtk_section->setting_table, "Icons");

  g_assert_true (legacy_prefer_dark_setting);
  g_assert_cmpstr (legacy_prefer_dark_setting->name, ==, "Legacy prefer dark");
  g_assert_cmpint (legacy_prefer_dark_setting->type, ==, MS_TWEAKS_TYPE_BOOLEAN);
  g_assert_cmpstr (legacy_prefer_dark_setting->help,
                   ==,
                   "Use dark version of the theme. Only some GTK3 applications will react to this setting.");
  g_assert_cmpint (legacy_prefer_dark_setting->backend,
                   ==,
                   MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS);
  g_assert_true (legacy_prefer_dark_setting->key);
  g_assert_cmpint (legacy_prefer_dark_setting->key->len, ==, 1);
  g_assert_cmpstr (g_ptr_array_index (legacy_prefer_dark_setting->key, 0),
                   ==,
                   "gtk-application-prefer-dark-theme");
  g_assert_cmpstr (legacy_prefer_dark_setting->default_, ==, "0");
  g_assert_true (legacy_prefer_dark_setting->map);
  g_assert_cmpint (g_hash_table_size (legacy_prefer_dark_setting->map), ==, 2);
  g_assert_cmpstr (g_hash_table_lookup (legacy_prefer_dark_setting->map, "true"), ==, "1");
  g_assert_cmpstr (g_hash_table_lookup (legacy_prefer_dark_setting->map, "false"), ==, "0");

  g_assert_true (icons_setting);
  g_assert_cmpint (icons_setting->gtype, ==, MS_TWEAKS_GTYPE_STRING);
  g_assert_true (icons_setting->map);
}


static void
test_parse_invalid_hierarchy (ParserTestFixture *fixture, gconstpointer unused)
{
  parse (
    "- name:\n");

  g_assert_cmpstr (fixture->error->message, ==, "Empty page name declarations are not allowed");
  g_assert_cmpint (fixture->error->code, ==, MS_TWEAKS_PARSER_ERROR_EMPTY_PAGE_DECLARATION);
  g_assert_cmpint (fixture->error->domain, ==, MS_TWEAKS_PARSER_ERROR);

  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 0);
}


static void
test_parse_invalid_hierarchy_2 (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *merp_page = NULL;

  parse (
    "- name: Merp\n"
    "  sections:\n"
    "    - name: So far so good\n"
    "      sections:\n"
    "        - name: Something feels wrong");

  g_assert_cmpstr (fixture->error->message, ==, "Unexpected scalar in section: sections");
  g_assert_cmpint (fixture->error->code, ==, MS_TWEAKS_PARSER_ERROR_UNEXPECTED_SCALAR_IN_SECTION);
  g_assert_cmpint (fixture->error->domain, ==, MS_TWEAKS_PARSER_ERROR);

  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);

  merp_page = g_hash_table_lookup (fixture->parser->page_table, "Merp");

  g_assert_true (merp_page);
  g_assert_cmpint (g_hash_table_size (merp_page->section_table), ==, 0);
}


static void
test_parse_invalid_hierarchy_3 (ParserTestFixture *fixture, gconstpointer unused)
{
  parse (
    "- name: Boop\n"
    "  settings:\n"
    "    - name: What????");

  g_assert_cmpstr (fixture->error->message, ==, "Unexpected scalar in page: settings");
  g_assert_cmpint (fixture->error->code, ==, MS_TWEAKS_PARSER_ERROR_UNEXPECTED_SCALAR_IN_PAGE);
  g_assert_cmpint (fixture->error->domain, ==, MS_TWEAKS_PARSER_ERROR);

  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 0);
}


static void
test_parse_invalid_hierarchy_4 (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *boop_page = NULL;
  MsTweaksSection *zoop_section = NULL;

  parse (
    "- name: Boop\n"
    "  sections:\n"
    "    - name: Zoop\n"
    "      settings:\n"
    "        - name: Oh no\n"
    "          amogus: sus");

  g_assert_cmpstr (fixture->error->message, ==, "Unexpected scalar in setting: amogus");
  g_assert_cmpint (fixture->error->code, ==, MS_TWEAKS_PARSER_ERROR_UNEXPECTED_SCALAR_IN_SETTING);
  g_assert_cmpint (fixture->error->domain, ==, MS_TWEAKS_PARSER_ERROR);

  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);

  boop_page = g_hash_table_lookup (fixture->parser->page_table, "Boop");

  g_assert_true (boop_page);
  g_assert_cmpint (g_hash_table_size (boop_page->section_table), ==, 1);

  zoop_section = g_hash_table_lookup (boop_page->section_table, "Zoop");

  g_assert_true (zoop_section);
  g_assert_cmpint (g_hash_table_size (zoop_section->setting_table), ==, 0);
}


static void
test_parse_invalid_boolean_readonly (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *here_page = NULL;
  MsTweaksSection *it_section = NULL;

  parse (
    "- name: Here\n"
    "  sections:\n"
    "    - name: It\n"
    "      settings:\n"
    "        - name: Comes\n"
    "          readonly: Norway");

  g_assert_cmpstr (fixture->error->message, ==, "Invalid YAML bool value 'Norway' in readonly property");
  g_assert_cmpint (fixture->error->code, ==, MS_TWEAKS_PARSER_ERROR_INVALID_BOOLEAN_VALUE_IN_READONLY_PROPERTY);
  g_assert_cmpint (fixture->error->domain, ==, MS_TWEAKS_PARSER_ERROR);

  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);

  here_page = g_hash_table_lookup (fixture->parser->page_table, "Here");

  g_assert_true (here_page);
  g_assert_cmpint (g_hash_table_size (here_page->section_table), ==, 1);

  it_section = g_hash_table_lookup (here_page->section_table, "It");

  g_assert_true (it_section);
  g_assert_cmpint (g_hash_table_size (it_section->setting_table), ==, 0);
}


static void
test_parse_invalid_boolean_source_ext (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *here_page = NULL;
  MsTweaksSection *it_section = NULL;

  parse (
    "- name: Here\n"
    "  sections:\n"
    "    - name: It\n"
    "      settings:\n"
    "        - name: Comes\n"
    "          source_ext: Sweden");

  g_assert_cmpstr (fixture->error->message, ==, "Invalid YAML bool value 'Sweden' in source_ext property");
  g_assert_cmpint (fixture->error->code, ==, MS_TWEAKS_PARSER_ERROR_INVALID_BOOLEAN_VALUE_IN_SOURCE_EXT_PROPERTY);
  g_assert_cmpint (fixture->error->domain, ==, MS_TWEAKS_PARSER_ERROR);

  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);

  here_page = g_hash_table_lookup (fixture->parser->page_table, "Here");

  g_assert_true (here_page);
  g_assert_cmpint (g_hash_table_size (here_page->section_table), ==, 1);

  it_section = g_hash_table_lookup (here_page->section_table, "It");

  g_assert_true (it_section);
  g_assert_cmpint (g_hash_table_size (it_section->setting_table), ==, 0);
}


static void
test_parse_two_empty_calls (ParserTestFixture *fixture, gconstpointer unused)
{
  parse ("");
  parse ("");
}


static void
test_parse_multiple_calls_and_pages (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *page_one = NULL;
  MsTweaksPage *page_two = NULL;

  parse ("- name: One\n");
  parse ("- name: Two\n");

  g_assert_true (fixture->parser->page_table);
  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 2);

  page_one = g_hash_table_lookup (fixture->parser->page_table, "One");
  page_two = g_hash_table_lookup (fixture->parser->page_table, "Two");

  g_assert_true (page_one);
  g_assert_true (page_two);
}


static void
test_parse_multiple_calls (ParserTestFixture *fixture, gconstpointer unused)
{
  MsTweaksPage *fonts_page = NULL;

  parse (
    "- name: Fonts\n"
    "  weight: 40\n"
    "  sections:\n"
    "    - name: Interface font\n"
    "      weight: 0\n"
    "      settings:\n"
    "        - name: Interface\n"
    "          type: font\n"
    "          help: The default interface text font\n"
    "          backend: gsettings\n"
    "          gtype: string\n"
    "          key: org.gnome.desktop.interface.font-name\n"
    "        - name: Document\n"
    "          type: font\n"
    "          help: The default font for reading documents\n"
    "          backend: gsettings\n"
    "          gtype: string\n"
    "          key: org.gnome.desktop.interface.document-font-name\n"
    "        - name: Monospace\n"
    "          type: font\n"
    "          help: Name of a monospaced (fixed-width) font for use in locations like terminals.\n"
    "          backend: gsettings\n"
    "          gtype: string\n"
    "          key: org.gnome.desktop.interface.monospace-font-name)\n");
  parse (
    "- name: Fonts\n"
    "  weight: 40\n"
    "  sections:\n"
    "    - name: Font rendering\n"
    "      weight: 10\n"
    "      settings:\n"
    "        - name: Hinting\n"
    "          type: choice\n"
    "          help: The type of hinting to use when rendering fonts.\n"
    "          gtype: string\n"
    "          key:\n"
    "            - org.gnome.settings-daemon.plugins.xsettings.hinting\n"
    "            - org.gnome.desktop.interface.font-hinting\n"
    "          map:\n"
    "            None: none\n"
    "            Slight: slight\n"
    "            Medium: medium\n"
    "            Full: full\n"
    "        - name: Scaling\n"
    "          type: number\n"
    "          gtype: double\n"
    "          help: Scaling factor for all font sizes\n"
    "          key: org.gnome.desktop.interface.text-scaling-factor\n"
    "          min: 0.5\n"
    "          max: 3\n"
    "          step: 0.1\n");

  g_assert_true (fixture->parser->page_table);
  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 1);

  fonts_page = g_hash_table_lookup (fixture->parser->page_table, "Fonts");

  g_assert_true (fonts_page);
  g_assert_true (fonts_page->section_table);
  g_assert_cmpint (g_hash_table_size (fonts_page->section_table), ==, 2);
}


static void
test_parse_with_sort (ParserTestFixture *fixture, gconstpointer unused)
{
  GList *sorted_settings = NULL;
  MsTweaksPage *first_page = NULL;
  MsTweaksPage *second_page = NULL;

  parse (
    "- name: Second\n"
    "  weight: 10\n"
    "- name: First\n"
    "  weight: 1");

  g_assert_true (fixture->parser->page_table);
  g_assert_cmpint (g_hash_table_size (fixture->parser->page_table), ==, 2);

  sorted_settings = ms_tweaks_parser_sort_by_weight (fixture->parser->page_table);
  first_page = sorted_settings->data;
  second_page = sorted_settings->next->data;

  g_assert_true (first_page);
  g_assert_cmpstr (first_page->name, ==, "First");
  g_assert_cmpint (first_page->weight, ==, 1);

  g_assert_true (second_page);
  g_assert_cmpstr (second_page->name, ==, "Second");
  g_assert_cmpint (second_page->weight, ==, 10);

  g_list_free (sorted_settings);
}


static void
test_parse_multiple_calls_with_sort (ParserTestFixture *fixture, gconstpointer unused)
{
  GList *sorted_settings = NULL;

  parse (
    "- name: Tarp\n"
    "  weight: 2\n"
    "  sections:\n"
    "    - name: Larp\n"
    "      weight: 10\n"
    "      settings:\n"
    "        - name: Carp\n"
    "          type: choice\n"
    "          gtype: string\n"
    "          backend: gsettings\n"
    "          key: org.gnome.desktop.interface.color-scheme\n"
    "          default: \"default\"\n"
    "          map:\n"
    "            Default: default\n"
    "            Light: prefer-dark\n"
    "            Dark: prefer-dark\n"
    "\n"
    "- name: Pork\n"
    "  weight: 9\n"
    "  sections:\n"
    "    - name: Torque\n"
    "      weight: 45\n"
    "      settings:\n"
    "        - name: Cork\n"
    "          type: file\n"
    "          backend: css\n"
    "          key: ~/.config/gtk-3.0/gtk.css\n"
    "          selector: phosh-lockscreen, .phosh-lockshield\n"
    "          guard: phosh-lockscreen-background\n"
    "          css:\n"
    "            background-image: \"%\"\n"
    "            background-size: cover\n"
    "            background-position: center\n");
  parse (
    "- name: Pork\n"
    "  weight: 9\n"
    "  sections:\n"
    "    - name: York\n"
    "      weight: 99\n"
    "      settings:\n"
    "        - name: Spork\n"
    "          type: boolean\n"
    "          backend: gsettings\n"
    "          key: org.gnome.desktop.interface.clock-show-weekday");

  sorted_settings = ms_tweaks_parser_sort_by_weight (fixture->parser->page_table);

  g_assert_true (sorted_settings);
  g_assert_true (sorted_settings->next);
  g_assert_false (sorted_settings->next->next);

  g_list_free (sorted_settings);
}


#define SETTING_COUNT 3


static void
test_sort_settings_by_weight (void)
{
  /* This code was written with ease of reading in mind rather than optimising for using GList in a
   * performant way. */
  GHashTable *unsorted_settings = g_hash_table_new (g_direct_hash, g_direct_equal);
  GList *sorted_settings_result = NULL;
  GList *sorted_settings_good = NULL;
  MsTweaksSetting settings[SETTING_COUNT];

  settings[0].weight = 10;
  settings[1].weight = 0;
  settings[2].weight = 90;

  for (int i = 0; i < SETTING_COUNT; i++)
    g_hash_table_add (unsorted_settings, &settings[i]);

  sorted_settings_good = g_list_append (sorted_settings_good, &settings[1]);
  sorted_settings_good = g_list_append (sorted_settings_good, &settings[0]);
  sorted_settings_good = g_list_append (sorted_settings_good, &settings[2]);

  sorted_settings_result = ms_tweaks_parser_sort_by_weight (unsorted_settings);

  g_assert_cmpint (g_list_length (sorted_settings_result), ==, SETTING_COUNT);
  g_assert_cmpint (g_list_length (sorted_settings_good), ==, SETTING_COUNT);

  for (int i = 0; i < SETTING_COUNT; i++)
    g_assert_true (g_list_nth_data (sorted_settings_result, i) == g_list_nth_data (sorted_settings_good, i));

  g_list_free (sorted_settings_good);
  g_list_free (sorted_settings_result);
  g_hash_table_destroy (unsorted_settings);
}


static void
test_parser_fixture_setup (ParserTestFixture *fixture, gconstpointer unused)
{
  fixture->parser = ms_tweaks_parser_new ();
}


static void
test_parser_fixture_teardown (ParserTestFixture *fixture, gconstpointer unused)
{
  g_object_unref (fixture->parser);
  g_clear_error (&fixture->error);
}


#define PARSER_TEST_ADD(name, test_func) g_test_add ((name), \
                                                     ParserTestFixture, \
                                                     NULL, \
                                                     test_parser_fixture_setup, \
                                                     (test_func), \
                                                     test_parser_fixture_teardown)


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/test-tweaks-parser-copy-setting",
                   test_copy_setting);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-parser-copy-setting-empty",
                   test_copy_setting_empty);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-parser-copy-section",
                   test_copy_section);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-parser-copy-page",
                   test_copy_page);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-nothing",
                   test_parse_nothing);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-basic",
                   test_parse_basic);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-section",
                   test_parse_section);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-setting",
                   test_parse_setting);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-multiple-settings",
                   test_parse_multiple_settings);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-invalid-hierarchy",
                   test_parse_invalid_hierarchy);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-invalid-hierarchy-2",
                   test_parse_invalid_hierarchy_2);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-invalid-hierarchy-3",
                   test_parse_invalid_hierarchy_3);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-invalid-hierarchy-4",
                   test_parse_invalid_hierarchy_4);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-invalid-boolean-readonly",
                   test_parse_invalid_boolean_readonly);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-invalid-boolean-source-ext",
                   test_parse_invalid_boolean_source_ext);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-two-empty-calls",
                   test_parse_two_empty_calls);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-multiple-calls-and-pages",
                   test_parse_multiple_calls_and_pages);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-multiple-calls",
                   test_parse_multiple_calls);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-with-sort",
                   test_parse_with_sort);
  PARSER_TEST_ADD ("/phosh-mobile-settings/test-tweaks-parser-parse-multiple-calls-with-sort",
                   test_parse_multiple_calls_with_sort);
  g_test_add_func ("/phosh-mobile-settings/test-tweaks-parser-sort-by-weight",
                   test_sort_settings_by_weight);

  return g_test_run ();
}
