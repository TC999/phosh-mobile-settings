/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Gotam Gorabh <gautamy672@gmail.com>
 */

#include "ms-osk-add-shortcut-dialog.c"
#include <glib.h>

struct {
  const char *input;
  const char *expected_mod;
  const char *expected_key;
} shortcut_cases[] = {
  /* Normal and mixed-order cases */
  { "<ctrl><shift>c",        "<ctrl><shift>",       "c" },
  { "<alt><ctrl>c",          "<ctrl><alt>",         "c" },
  { "<shift><alt><super>x",  "<alt><shift><super>", "x" },

  /* Partial modifier-like words that should NOT match */
  { "<altd><ctrl>c",         "<ctrl>",        "<altd>c" },
  { "<super>salt><ctrl>c",   "<ctrl><super>", "salt>c" },
  { "ctrlx><shitc",          "",              "ctrlx><shitc" },

  /* No modifiers */
  { "a",                     "",              "a" },
  { "",                      "",              "" },

  /* Unclosed tags or malformed brackets */
  { "<ctrl<shift>c",         "<shift>",            "<ctrlc" },
  { "<<ctrl>>c",             "<ctrl>",             "<>c" },
  { "<ctrl><shift><alt",     "<ctrl><shift>",      "<alt" },

  /* Duplicates and repetition */
  { "<ctrl><ctrl><shift>v",  "<ctrl><shift>", "v" },
  { "<CTRL><Ctrl><SHIFT>z",  "<ctrl><shift>", "z" },

  { NULL, NULL, NULL }
};


struct {
  const char *base;
  const char *input;
  const char *expected;
} compute_cases[] = {
  /* Regular combinations */
  { "<ctrl>",                "<ctrl><shift>c",    "<ctrl><ctrl><shift>c" },
  { "<ctrl><shift>c",        "alt><super>d",      "<ctrl><shift><super>alt>d" },
  { "<shift><alt>c",         "<ctrl>d",           "<alt><shift><ctrl>d" },

  /* Missing key / only modifiers */
  { "<ctrl><alt>",           "<shift>",           "<ctrl><alt><shift>" },
  { "<ctrl><shift>c",        "",                  "<ctrl><shift>c" },

  /* No base modifiers */
  { "a",                     "<ctrl><alt>b",      "<ctrl><alt>b" },

  /* Overlapping / duplicate modifiers */
  { "<ctrl><alt>c",          "<ctrl><shift>d",    "<ctrl><alt><ctrl><shift>d" },
  { "<ctrl><ctrl><shift>x",  "<alt>y",            "<ctrl><shift><alt>y" },

  /* Malformed modifiers in input */
  { "<ctrl>",                "<altd>f",           "<ctrl><altd>f" },
  { "<ctrl>",                "super>c",           "<ctrl>super>c" },

  /* Upper/lower case mix */
  { "<CTRL>",                "<Alt><Shift>x",     "<ctrl><alt><shift>x" },

  /* Empty base or input or both */
  { "",                      "<ctrl>x",           "<ctrl>x" },
  { "<ctrl>v",               "",                  "<ctrl>v" },
  { "",                      "",                  "" },

  { NULL, NULL, NULL }
};


static void
test_extract_modifiers_and_key (void)
{
  for (int i = 0; shortcut_cases[i].input; i++) {
    g_autofree char *mods = extract_modifiers (shortcut_cases[i].input);
    g_autofree char *key  = extract_key (shortcut_cases[i].input);

    g_assert_cmpstr (mods, ==, shortcut_cases[i].expected_mod);
    g_assert_cmpstr (key, ==, shortcut_cases[i].expected_key);
  }
}


static void
test_compute_new_shortcut (void)
{
  for (int i = 0; compute_cases[i].base; i++) {
    g_autofree char *result = compute_new_shortcut (compute_cases[i].base,
                                                    compute_cases[i].input);
    g_assert_cmpstr (result, ==, compute_cases[i].expected);
  }
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh-mobile-settings/add-shortcut-dialog/extract_modifiers_and_key",
                   test_extract_modifiers_and_key);
  g_test_add_func ("/phosh-mobile-settings/add-shortcut-dialog/compute_new_shortcut",
                   test_compute_new_shortcut);

  return g_test_run ();
}
