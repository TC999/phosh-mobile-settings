/*
 * Copyright (C) 2025 Stefan Hansson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Stefan Hansson <newbyte@postmarketos.org>
 */

#define G_LOG_DOMAIN "ms-tweaks-parser"

#include "mobile-settings-config.h"

#undef GETTEXT_PACKAGE
#define GETTEXT_PACKAGE "conf-tweaks"

#include "ms-tweaks-parser.h"

#include "ms-tweaks-datasources.h"
#include "ms-tweaks-utils.h"

#include <glib/gi18n-lib.h>
#include <gmobile.h>

#include <math.h>
#include <yaml.h>

#define MS_TWEAKS_NO_LOCALE_FORMATTING
#include "ms-tweaks-files-poison.h"

/* Undocumented default from settingstree.py in postmarketos-tweaks. */
#define CONF_TWEAKS_DEFAULT_WEIGHT 50

#define CONF_TWEAKS_DEFAULT_NAME NULL
#define CONF_TWEAKS_DEFAULT_TYPE MS_TWEAKS_TYPE_UNKNOWN
#define CONF_TWEAKS_DEFAULT_GTYPE MS_TWEAKS_GTYPE_UNKNOWN
#define CONF_TWEAKS_DEFAULT_STYPE MS_TWEAKS_STYPE_UNKNOWN
#define CONF_TWEAKS_DEFAULT_MAP NULL
/* Undocumented default from settingstree.py in postmarketos-tweaks- */
#define CONF_TWEAKS_DEFAULT_BACKEND MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS
#define CONF_TWEAKS_DEFAULT_HELP NULL
#define CONF_TWEAKS_DEFAULT_DEFAULT_VALUE NULL
/* No CONF_TWEAKS_DEFAULT_KEY since it uses a constructor. */
#define CONF_TWEAKS_DEFAULT_READONLY FALSE
#define CONF_TWEAKS_DEFAULT_SELECTOR NULL
#define CONF_TWEAKS_DEFAULT_GUARD NULL
#define CONF_TWEAKS_DEFAULT_MULTIPLIER 1
/* No CONF_TWEAKS_DEFAULT_MIN, MAX, and STEP since they default to NaN, which is not equal to
 * itself. This makes them counterproductive to use when merging settings. */
#define CONF_TWEAKS_DEFAULT_CSS NULL


typedef enum {
  MS_TWEAKS_MAPPING_STATE_KEY,
  MS_TWEAKS_MAPPING_STATE_VALUE,
} MsTweaksMappingState;


typedef enum {
  MS_TWEAKS_STATE_START,              /* Start state. */
  MS_TWEAKS_STATE_STREAM,             /* Start/end stream. */
  MS_TWEAKS_STATE_DOCUMENT,           /* Start/end document. */

  MS_TWEAKS_STATE_PAGE,               /* Page entry. */
  MS_TWEAKS_STATE_PAGE_NAME,          /* Page name. */
  MS_TWEAKS_STATE_PAGE_WEIGHT,        /* Page weight. */

  MS_TWEAKS_STATE_SECTION,            /* Section entry. */
  MS_TWEAKS_STATE_SECTION_NAME,       /* Section name. */
  MS_TWEAKS_STATE_SECTION_WEIGHT,     /* Section weight. */

  MS_TWEAKS_STATE_SETTING,            /* Setting information. */
  MS_TWEAKS_STATE_SETTING_NAME,       /* Setting name. */
  MS_TWEAKS_STATE_SETTING_WEIGHT,     /* Setting weight. */
  MS_TWEAKS_STATE_SETTING_TYPE,       /* Setting type. */
  MS_TWEAKS_STATE_SETTING_GTYPE,      /* Setting GSettings type. */
  MS_TWEAKS_STATE_SETTING_STYPE,      /* Setting sysfs type. */
  MS_TWEAKS_STATE_SETTING_DATA,       /* Setting data source. */
  MS_TWEAKS_STATE_SETTING_MAP,        /* Mapping of multiple values. */
  MS_TWEAKS_STATE_SETTING_BACKEND,    /* Setting backend. */
  MS_TWEAKS_STATE_SETTING_HELP,       /* Setting help message. */
  MS_TWEAKS_STATE_SETTING_DEFAULT,    /* Setting default value. */
  MS_TWEAKS_STATE_SETTING_KEY,        /* Setting key. */
  MS_TWEAKS_STATE_SETTING_READONLY,   /* Setting readonly property. */
  MS_TWEAKS_STATE_SETTING_SOURCE_EXT, /* Setting source_ext property. */
  MS_TWEAKS_STATE_SETTING_SELECTOR,   /* Setting selector (CSS backend). */
  MS_TWEAKS_STATE_SETTING_GUARD,      /* Setting guard (CSS backend). */
  MS_TWEAKS_STATE_SETTING_MULTIPLIER, /* Setting multiplier. */
  MS_TWEAKS_STATE_SETTING_MIN,        /* Setting minimum value. */
  MS_TWEAKS_STATE_SETTING_MAX,        /* Setting maximum value. */
  MS_TWEAKS_STATE_SETTING_STEP,       /* Setting step value. */
  MS_TWEAKS_STATE_SETTING_CSS,        /* Setting CSS definition (CSS backend). */

  MS_TWEAKS_STATE_STOP,               /* End state. */
} MsTweaksState;


static const char *ms_tweaks_backend_identifier[] = {
  [MS_TWEAKS_BACKEND_IDENTIFIER_UNKNOWN] = "UNKNOWN",

  [MS_TWEAKS_BACKEND_IDENTIFIER_CSS] = "CSS",
  [MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS] = "GSETTINGS",
  [MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS] = "GTK3SETTINGS",
  [MS_TWEAKS_BACKEND_IDENTIFIER_HARDWAREINFO] = "HARDWAREINFO",
  [MS_TWEAKS_BACKEND_IDENTIFIER_OSKSDL] = "OSKSDL",
  [MS_TWEAKS_BACKEND_IDENTIFIER_SOUNDTHEME] = "SOUNDTHEME",
  [MS_TWEAKS_BACKEND_IDENTIFIER_SYSFS] = "SYSFS",
  [MS_TWEAKS_BACKEND_IDENTIFIER_SYMLINK] = "SYMLINk",
  [MS_TWEAKS_BACKEND_IDENTIFIER_XRESOURCES] = "XRESOURCES",
};
static const guint ms_tweaks_backend_identifier_count = G_N_ELEMENTS (ms_tweaks_backend_identifier);


static const char *ms_tweaks_state[] = {
  [MS_TWEAKS_STATE_START] = "START",
  [MS_TWEAKS_STATE_STREAM] = "STREAM",
  [MS_TWEAKS_STATE_DOCUMENT] = "DOCUMENT",

  [MS_TWEAKS_STATE_PAGE] = "PAGE",
  [MS_TWEAKS_STATE_PAGE_NAME] = "PAGE_NAME",
  [MS_TWEAKS_STATE_PAGE_WEIGHT] = "PAGE_WEIGHT",

  [MS_TWEAKS_STATE_SETTING] = "SETTING",
  [MS_TWEAKS_STATE_SETTING_NAME] = "SETTING_NAME",
  [MS_TWEAKS_STATE_SETTING_WEIGHT] = "SETTING_WEIGHT",
  [MS_TWEAKS_STATE_SETTING_TYPE] = "SETTING_TYPE",
  [MS_TWEAKS_STATE_SETTING_GTYPE] = "SETTING_GTYPE",
  [MS_TWEAKS_STATE_SETTING_STYPE] = "SETTING_STYPE",
  [MS_TWEAKS_STATE_SETTING_DATA] = "SETTING_DATA",
  [MS_TWEAKS_STATE_SETTING_MAP] = "SETTING_MAP",
  [MS_TWEAKS_STATE_SETTING_BACKEND] = "SETTING_BACKEND",
  [MS_TWEAKS_STATE_SETTING_HELP] = "SETTING_HELP",
  [MS_TWEAKS_STATE_SETTING_DEFAULT] = "SETTING_DEFAULT",
  [MS_TWEAKS_STATE_SETTING_KEY] = "SETTING_KEY",
  [MS_TWEAKS_STATE_SETTING_READONLY] = "SETTING_READONLY",
  [MS_TWEAKS_STATE_SETTING_SOURCE_EXT] = "SETTING_SOURCE_EXT",
  [MS_TWEAKS_STATE_SETTING_SELECTOR] = "SETTING_SELECTOR",
  [MS_TWEAKS_STATE_SETTING_GUARD] = "SETTING_GUARD",
  [MS_TWEAKS_STATE_SETTING_MULTIPLIER] = "SETTING_MULTIPLIER",
  [MS_TWEAKS_STATE_SETTING_MIN] = "SETTING_MIN",
  [MS_TWEAKS_STATE_SETTING_MAX] = "SETTING_MAX",
  [MS_TWEAKS_STATE_SETTING_STEP] = "SETTING_STEP",
  [MS_TWEAKS_STATE_SETTING_CSS] = "SETTING_CSS",

  [MS_TWEAKS_STATE_STOP] = "STOP",
};
static const guint ms_tweaks_state_count = G_N_ELEMENTS (ms_tweaks_state);


static const char *yaml_event[] = {
  [YAML_NO_EVENT] = "NO",
  [YAML_STREAM_START_EVENT] = "STREAM_START",
  [YAML_STREAM_END_EVENT] = "STREAM_END",
  [YAML_DOCUMENT_START_EVENT] = "DOCUMENT_START",
  [YAML_DOCUMENT_END_EVENT] = "DOCUMENT_END",
  [YAML_ALIAS_EVENT] = "ALIAS",
  [YAML_SCALAR_EVENT] = "SCALAR",
  [YAML_SEQUENCE_START_EVENT] = "SEQUENCE_START",
  [YAML_SEQUENCE_END_EVENT] = "SEQUENCE_END",
  [YAML_MAPPING_START_EVENT] = "MAPPING_START",
  [YAML_MAPPING_END_EVENT] = "MAPPING_END",
};
static const guint yaml_event_count = G_N_ELEMENTS (yaml_event);


struct _MsTweaksParser {
  GObject parent_instance;

  MsTweaksState state;
  MsTweaksPage *current_page;
  MsTweaksSection *current_section;
  MsTweaksSetting *current_setting;
  /* _inserted: Tracks whether the element already was inserted into the relevant hash table. */
  gboolean current_page_inserted;
  gboolean current_section_inserted;
  gboolean current_setting_inserted;
  GHashTable *page_table; /* key: char *, value: MsTweaksPage * */
  /* Used when parsing the "key" property of a setting. */
  gboolean in_setting_key_list;
  /* Used when parsing the "map" and "css" properties of a setting. */
  MsTweaksMappingState setting_mapping_state;
  /* Also used when parsing the "map" and "css" properties of a setting. */
  char *last_key_name;
};


G_DEFINE_TYPE (MsTweaksParser, ms_tweaks_parser, G_TYPE_OBJECT)
G_DEFINE_QUARK (ms-tweaks-parser-error-quark, ms_tweaks_parser_error)


MsTweaksSetting *
ms_tweaks_setting_copy (const MsTweaksSetting *setting)
{
  MsTweaksSetting *new_setting = g_new0 (MsTweaksSetting, 1);

  new_setting->weight = setting->weight;
  new_setting->name = g_strdup (setting->name);
  new_setting->name_i18n = g_strdup (setting->name_i18n);
  new_setting->type = setting->type;
  new_setting->gtype = setting->gtype;
  new_setting->stype = setting->stype;
  if (setting->map)
    new_setting->map = g_hash_table_ref (setting->map);
  new_setting->backend = setting->backend;
  new_setting->help = g_strdup (setting->help);
  new_setting->help_i18n = g_strdup (setting->help_i18n);
  new_setting->default_ = g_strdup (setting->default_);
  new_setting->key = g_ptr_array_ref (setting->key);
  new_setting->readonly = setting->readonly;
  new_setting->source_ext = setting->source_ext;
  new_setting->selector = g_strdup (setting->selector);
  new_setting->guard = g_strdup (setting->guard);
  new_setting->multiplier = setting->multiplier;
  new_setting->min = setting->min;
  new_setting->max = setting->max;
  new_setting->step = setting->step;
  if (setting->css)
    new_setting->css = g_hash_table_ref (setting->css);

  return new_setting;
}


MsTweaksSection *
ms_tweaks_section_copy (const MsTweaksSection *section)
{
  MsTweaksSection *new_section = g_new0 (MsTweaksSection, 1);

  new_section->weight = section->weight;
  new_section->name = g_strdup (section->name);
  new_section->name_i18n = g_strdup (section->name_i18n);
  if (section->setting_table)
    new_section->setting_table = g_hash_table_ref (section->setting_table);

  return new_section;
}


MsTweaksPage *
ms_tweaks_page_copy (const MsTweaksPage *page)
{
  MsTweaksPage *new_page = g_new0 (MsTweaksPage, 1);

  new_page->weight = page->weight;
  new_page->name = g_strdup (page->name);
  new_page->name_i18n = g_strdup (page->name_i18n);
  if (page->section_table)
    new_page->section_table = g_hash_table_ref (page->section_table);

  return new_page;
}


void
ms_tweaks_setting_free (MsTweaksSetting *setting)
{
  /* Free string properties. */
  g_free (setting->name);
  g_free (setting->help);
  g_free (setting->default_);
  g_free (setting->selector);
  g_free (setting->guard);

  g_free (setting->name_i18n);
  g_free (setting->help_i18n);

  /* Free hash table properties. */
  g_clear_pointer (&setting->map, g_hash_table_unref);
  g_clear_pointer (&setting->css, g_hash_table_unref);

  /* Free pointer array properties. */
  g_clear_pointer (&setting->key, g_ptr_array_unref);

  g_free (setting);
}


void
ms_tweaks_section_free (MsTweaksSection *section)
{
  g_free (section->name);
  g_free (section->name_i18n);

  g_clear_pointer (&section->setting_table, g_hash_table_unref);

  g_free (section);
}


void
ms_tweaks_page_free (MsTweaksPage *page)
{
  g_free (page->name);
  g_free (page->name_i18n);

  g_clear_pointer (&page->section_table, g_hash_table_unref);

  g_free (page);
}


G_DEFINE_BOXED_TYPE (MsTweaksSetting, ms_tweaks_setting, ms_tweaks_setting_copy, ms_tweaks_setting_free)
G_DEFINE_BOXED_TYPE (MsTweaksSection, ms_tweaks_section, ms_tweaks_section_copy, ms_tweaks_section_free)
G_DEFINE_BOXED_TYPE (MsTweaksPage, ms_tweaks_page, ms_tweaks_page_copy, ms_tweaks_page_free)


GHashTable *
ms_tweaks_parser_get_page_table (MsTweaksParser *self)
{
  return self->page_table;
}


static void
ms_tweaks_parser_init (MsTweaksParser *self)
{
  self->state = MS_TWEAKS_STATE_START;
  self->page_table = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            (GDestroyNotify) ms_tweaks_page_free);
  /* Initialise it to key as we will always start with a key in mappings. */
  self->setting_mapping_state = MS_TWEAKS_MAPPING_STATE_KEY;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
}


static void
ms_tweaks_parser_dispose (GObject *object)
{
  MsTweaksParser *self = MS_TWEAKS_PARSER (object);

  g_clear_pointer (&self->last_key_name, g_free);
  g_clear_pointer (&self->page_table, g_hash_table_unref);

  if (!self->current_setting_inserted)
    g_clear_pointer (&self->current_setting, ms_tweaks_setting_free);
  if (!self->current_section_inserted)
    g_clear_pointer (&self->current_section, ms_tweaks_section_free);
  if (!self->current_page_inserted)
    g_clear_pointer (&self->current_page, ms_tweaks_page_free);
}


static const char *
pretty_format_state (const MsTweaksState state)
{
  if (state < ms_tweaks_state_count)
    return ms_tweaks_state[state];
  else
    return "FIXME: unknown state!";
}


static const char *
pretty_format_event (const yaml_event_t *event)
{
  g_assert (event);

  if (event->type < yaml_event_count)
    return yaml_event[event->type];
  else
    return "FIXME: unknown event!";
}


const char *
pretty_format_backend_identifier (const MsTweaksSettingBackend backend_identifier)
{
  if (backend_identifier < ms_tweaks_backend_identifier_count)
    return ms_tweaks_backend_identifier[backend_identifier];
  else
    return "FIXME: unknown backend identifier!";
}


static void
remove_current_page (MsTweaksParser *self)
{
  if (self->current_page->name && self->current_page_inserted) {
    if (!g_hash_table_remove (self->page_table, self->current_page->name)) {
      g_error ("Failed to remove '%s' from page table despite that it previously was inserted!",
               self->current_page->name);
    }
  }
}


static void
remove_current_section (MsTweaksParser *self)
{
  if (self->current_section->name && self->current_section_inserted) {
    if (!g_hash_table_remove (self->current_page->section_table, self->current_section->name)) {
      g_error ("Failed to remove '%s' from section table despite that it previously was inserted!",
               self->current_section->name);
    }
  }
}


static void
remove_current_setting (MsTweaksParser *self)
{
  if (self->current_setting->name && self->current_setting_inserted) {
    if (!g_hash_table_remove (self->current_section->setting_table, self->current_setting->name)) {
      g_error ("Failed to remove '%s' from setting table despite that it previously was inserted!",
               self->current_setting->name);
    }
  }
}


static void
report_error (const MsTweaksState state, const yaml_event_t *event, GError **error)
{
  g_set_error (error,
               MS_TWEAKS_PARSER_ERROR,
               MS_TWEAKS_PARSER_ERROR_UNEXPECTED_EVENT_IN_STATE,
               "Unexpected event %s in state MS_TWEAKS_STATE_%s.",
               pretty_format_event (event),
               pretty_format_state (state));
}


static void
merge_weights (int *into, const int from)
{
  g_assert (into);

  /* Only overwrite the weight if it wasn't specified. We assume that even if the default was
   * specified, the original definition didn't care about the weight. */
  if (*into == CONF_TWEAKS_DEFAULT_WEIGHT)
    *into = from;
}


static void
merge_settings (MsTweaksSetting *into, const MsTweaksSetting *from)
{
  g_assert (into);
  g_assert (from);

  merge_weights (&into->weight, from->weight);

  /* Only overwrite properties if they weren't specified. We assume that even if the default was
   * specified, the original definition didn't care about the property. */
  if (into->type == CONF_TWEAKS_DEFAULT_TYPE)
    into->type = from->type;
  if (into->gtype == CONF_TWEAKS_DEFAULT_GTYPE)
    into->gtype = from->gtype;
  if (into->stype == CONF_TWEAKS_DEFAULT_STYPE)
    into->stype = from->stype;
  if (into->map == CONF_TWEAKS_DEFAULT_MAP)
    into->map = from->map;
  else if (from->map)
    g_hash_table_unref (from->map);
  if (into->backend == CONF_TWEAKS_DEFAULT_BACKEND)
    into->backend = from->backend;
  if (into->help == CONF_TWEAKS_DEFAULT_HELP) {
    into->help = from->help;
    into->help_i18n = from->help_i18n;
  } else if (from->help)
    g_free (from->help);
  if (into->default_ == CONF_TWEAKS_DEFAULT_DEFAULT_VALUE)
    into->default_ = from->default_;
  else if (from->default_)
    g_free (from->default_);
  if (into->key == NULL)
    into->key = from->key;
  else if (from->key)
    g_ptr_array_unref (from->key);
  if (into->readonly == CONF_TWEAKS_DEFAULT_READONLY)
    into->readonly = from->readonly;
  if (into->selector == CONF_TWEAKS_DEFAULT_SELECTOR)
    into->selector = from->selector;
  else if (from->selector)
    g_free (from->selector);
  if (into->guard == CONF_TWEAKS_DEFAULT_GUARD)
    into->guard = from->guard;
  else if (from->guard)
    g_free (from->guard);
  if (into->multiplier == CONF_TWEAKS_DEFAULT_MULTIPLIER)
    into->multiplier = from->multiplier;
  if (isnan (into->min))
    into->min = from->min;
  if (isnan (into->max))
    into->max = from->max;
  if (isnan (into->step))
    into->step = from->step;
  if (into->css == CONF_TWEAKS_DEFAULT_CSS)
    into->css = from->css;
  else if (from->css)
    g_hash_table_unref (from->css);
}

/**
 * merge_sections:
 * @into: Pointer to the MsTweaksPage data should be copied into.
 * @from: Pointer to the MsTweaksPage data should be copied from.
 */
static void
merge_sections (MsTweaksSection *into, const MsTweaksSection *from)
{
  gpointer setting_to_insert_name = NULL, setting_to_insert = NULL;
  GHashTableIter iter;

  merge_weights (&into->weight, from->weight);

  /* Merge settings. */
  g_hash_table_iter_init (&iter, from->setting_table);
  while (g_hash_table_iter_next (&iter, &setting_to_insert_name, &setting_to_insert)) {
    MsTweaksSetting *setting_to_merge_into = g_hash_table_lookup (into->setting_table,
                                                                  setting_to_insert_name);
    MsTweaksSetting *setting_to_insert_copy = ms_tweaks_setting_copy (setting_to_insert);

    if (setting_to_merge_into) {
      /* Both tables have a setting with the same name, merge them. */
      merge_settings (setting_to_merge_into, setting_to_insert_copy);
    } else {
      /* Section is new, just insert it. */
      g_hash_table_insert (into->setting_table,
                           g_strdup (setting_to_insert_name),
                           setting_to_insert_copy);
    }
  }
}

/**
 * merge_pages:
 * @into: Pointer to the MsTweaksPage data should be copied into.
 * @from: Pointer to the MsTweaksPage data should be copied from.
 *
 * Merges data from one MsTweaksPage to another. Data in `into` is only overwritten by data in
 * `from` if it is set to a default value or NULL. Containers in `into` will be extended using the
 * values in `from`.
 */
static void
merge_pages (MsTweaksPage *into, const MsTweaksPage *from)
{
  gpointer section_to_insert_name = NULL, section_to_insert = NULL;
  GHashTableIter iter;

  merge_weights (&into->weight, from->weight);

  /* Merge sections. */
  g_hash_table_iter_init (&iter, from->section_table);
  while (g_hash_table_iter_next (&iter, &section_to_insert_name, &section_to_insert)) {
    MsTweaksSection *section_to_merge_into = g_hash_table_lookup (into->section_table,
                                                                  section_to_insert_name);
    MsTweaksSection *section_to_insert_copy = ms_tweaks_section_copy (section_to_insert);

    if (section_to_merge_into) {
      /* Both tables have a section with the same name, merge them. */
      merge_sections (section_to_merge_into, section_to_insert_copy);
    } else {
      /* Section is new, just insert it. */
      g_hash_table_insert (into->section_table,
                           g_strdup (section_to_insert_name),
                           section_to_insert_copy);
    }
  }
}

/**
 * str_to_backend:
 * @setting_backend_str: String representation of a backend identifier.
 *
 * Converts a string representation of a backend identifier to an enum representation.
 *
 * This needs to be updated whenever a new backend is added.
 *
 * Returns: Enum representation of the string backend identifier.
 */
static MsTweaksSettingBackend
str_to_backend (const char *setting_backend_str)
{
  if (strcmp (setting_backend_str, "css") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_CSS;
  else if (strcmp (setting_backend_str, "gsettings") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_GSETTINGS;
  else if (strcmp (setting_backend_str, "gtk3settings") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_GTK3SETTINGS;
  else if (strcmp (setting_backend_str, "hardwareinfo") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_HARDWAREINFO;
  else if (strcmp (setting_backend_str, "osksdl") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_OSKSDL;
  else if (strcmp (setting_backend_str, "soundtheme") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_SOUNDTHEME;
  else if (strcmp (setting_backend_str, "sysfs") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_SYSFS;
  else if (strcmp (setting_backend_str, "symlink") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_SYMLINK;
  else if (strcmp (setting_backend_str, "xresources") == 0)
    return MS_TWEAKS_BACKEND_IDENTIFIER_XRESOURCES;
  else {
    g_warning ("Unknown backend '%s'", setting_backend_str);
    return MS_TWEAKS_BACKEND_IDENTIFIER_UNKNOWN;
  }
}


static MsTweaksWidgetType
str_to_setting_type (const char *setting_type_str)
{
  if (strcmp (setting_type_str, "boolean") == 0)
    return MS_TWEAKS_TYPE_BOOLEAN;
  else if (strcmp (setting_type_str, "choice") == 0)
    return MS_TWEAKS_TYPE_CHOICE;
  else if (strcmp (setting_type_str, "color") == 0)
    return MS_TWEAKS_TYPE_COLOR;
  else if (strcmp (setting_type_str, "file") == 0)
    return MS_TWEAKS_TYPE_FILE;
  else if (strcmp (setting_type_str, "font") == 0)
    return MS_TWEAKS_TYPE_FONT;
  else if (strcmp (setting_type_str, "info") == 0)
    return MS_TWEAKS_TYPE_INFO;
  else if (strcmp (setting_type_str, "number") == 0)
    return MS_TWEAKS_TYPE_NUMBER;
  else {
    g_warning ("Unknown setting type '%s'", setting_type_str);
    return MS_TWEAKS_TYPE_UNKNOWN;
  }
}


static MsTweaksSettingGsettingType
str_to_gsettings_type (const char *setting_type_str)
{
  if (strcmp (setting_type_str, "boolean") == 0)
    return MS_TWEAKS_GTYPE_BOOLEAN;
  else if (strcmp (setting_type_str, "string") == 0)
    return MS_TWEAKS_GTYPE_STRING;
  else if (strcmp (setting_type_str, "number") == 0)
    return MS_TWEAKS_GTYPE_NUMBER;
  else if (strcmp (setting_type_str, "double") == 0)
    return MS_TWEAKS_GTYPE_DOUBLE;
  else if (strcmp (setting_type_str, "flags") == 0)
    return MS_TWEAKS_GTYPE_FLAGS;
  else {
    g_warning ("Unknown GSettings type '%s'", setting_type_str);
    return MS_TWEAKS_GTYPE_UNKNOWN;
  }
}


static MsTweaksSettingSysfsType
str_to_sysfs_type (const char *setting_type_str)
{
  if (strcmp (setting_type_str, "int") == 0)
    return MS_TWEAKS_STYPE_INT;
  else if (strcmp (setting_type_str, "string") == 0)
    return MS_TWEAKS_STYPE_STRING;
  else {
    g_warning ("Unknown sysfs type '%s'", setting_type_str);
    return MS_TWEAKS_STYPE_UNKNOWN;
  }
}

/*
 * Convert a yaml boolean string to a boolean value (true|false).
 *
 * Copied from parse.c in libyaml-examples with minor modifications.
 */
static int
str_to_bool (const char *string, gboolean *value)
{
  const char *t[] = { "y", "Y", "yes", "Yes", "YES", "true", "True", "TRUE", "on", "On", "ON", NULL };
  const char *f[] = { "n", "N", "no", "No", "NO", "false", "False", "FALSE", "off", "Off", "OFF", NULL };
  const char **p;

  for (p = t; *p; p++) {
    if (strcmp (string, *p) == 0) {
      *value = TRUE;
      return 0;
    }
  }
  for (p = f; *p; p++) {
    if (strcmp (string, *p) == 0) {
      *value = FALSE;
      return 0;
    }
  }
  return EINVAL;
}


static gboolean
consume_event (MsTweaksParser *self, const yaml_event_t *event, GError **error)
{
  GHashTable **ctx_hash_table_ptr = NULL;

  g_assert (self);
  g_assert (event);
  /* Check for pileup. */
  g_assert (!*error);

#pragma GCC diagnostic push
/* This part of the function has a lot of switches where each has a lot of possible values, avoid
 * having to repeat those when they're not relevant. */
#pragma GCC diagnostic ignored "-Wswitch-enum"

  switch (self->state) {
  case MS_TWEAKS_STATE_START:
    switch (event->type) {
    case YAML_STREAM_START_EVENT:
      self->state = MS_TWEAKS_STATE_STREAM;
      break;
    default:
      report_error (self->state, event, error);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_STREAM:
    switch (event->type) {
    case YAML_DOCUMENT_START_EVENT:
      self->state = MS_TWEAKS_STATE_DOCUMENT;
      break;
    case YAML_STREAM_END_EVENT:
      self->state = MS_TWEAKS_STATE_STOP; /* All done! */
      break;
    default:
      report_error (self->state, event, error);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_DOCUMENT:
    switch (event->type) {
    case YAML_SEQUENCE_START_EVENT:
      self->state = MS_TWEAKS_STATE_PAGE; /* Start of a page. */
      break;
    default:
      report_error (self->state, event, error);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_PAGE:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      const char *prop_name = (char *) event->data.scalar.value;

      if (strcmp (prop_name, "name") == 0)
        self->state = MS_TWEAKS_STATE_PAGE_NAME;
      else if (strcmp (prop_name, "weight") == 0)
        self->state = MS_TWEAKS_STATE_PAGE_WEIGHT;
      else if (strcmp (prop_name, "sections") == 0)
        self->state = MS_TWEAKS_STATE_SECTION;
      else {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_UNEXPECTED_SCALAR_IN_PAGE,
                     "Unexpected scalar in page: %s",
                     prop_name);
        remove_current_page (self);
        return FALSE;
      }
      break;
    case YAML_SEQUENCE_START_EVENT:
      break;
    case YAML_SEQUENCE_END_EVENT:
      self->state = MS_TWEAKS_STATE_STOP;
      break;
    case YAML_MAPPING_START_EVENT:
      /* Start of new page. */
      g_assert (!self->current_page);

      self->current_page = g_new0 (MsTweaksPage, 1);
      self->current_page->weight = CONF_TWEAKS_DEFAULT_WEIGHT;
      self->current_page->section_table = g_hash_table_new_full (g_str_hash,
                                                                 g_str_equal,
                                                                 g_free,
                                                                 (GDestroyNotify) ms_tweaks_section_free);
      self->current_page_inserted = FALSE;
      /* We don't know the name of the page yet, so we can't add it to the table. It gets added
       * later in STATE_PAGE_NAME. */
      break;
    case YAML_MAPPING_END_EVENT:
      g_assert (self->current_page);

      if (!self->current_page_inserted && self->current_page)
        ms_tweaks_page_free (self->current_page);
      self->current_page = NULL;
      /* There may be another page mapping after this,
       * so don't switch state to STATE_STOP. */
      break;
    case YAML_STREAM_END_EVENT:
      /* End of stream. Nothing to do. */
      break;
    case YAML_DOCUMENT_END_EVENT:
      /* End of document. Nothing to do. */
      break;
    default:
      report_error (self->state, event, error);
      remove_current_page (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_PAGE_NAME:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      MsTweaksPage *existing_page = NULL;

      g_assert (self->current_page);

      if (self->current_page->name) {
        g_warning ("Page '%s' has 'name' defined more than once, using first definition",
                   self->current_page->name);
        self->state = MS_TWEAKS_STATE_PAGE;
        break;
      }

      if (g_utf8_strlen ((char *) event->data.scalar.value, -1) == 0) {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_EMPTY_PAGE_DECLARATION,
                     "Empty page name declarations are not allowed");
        remove_current_page (self);
        self->state = MS_TWEAKS_STATE_PAGE;
        return FALSE;
      }

      self->current_page->name_i18n = g_strdup (_((char *) event->data.scalar.value));
      self->current_page->name = g_strdup ((char *) event->data.scalar.value);

      existing_page = g_hash_table_lookup (self->page_table, self->current_page->name);

      g_assert (self->current_page != existing_page);

      if (existing_page)
        merge_pages (self->current_page, existing_page);

      g_hash_table_replace (self->page_table,
                            g_strdup (self->current_page->name),
                            self->current_page);
      self->current_page_inserted = TRUE;

      self->state = MS_TWEAKS_STATE_PAGE;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_page (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SECTION:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      const char *prop_name = (char *) event->data.scalar.value;

      if (strcmp (prop_name, "name") == 0)
        self->state = MS_TWEAKS_STATE_SECTION_NAME;
      else if (strcmp (prop_name, "weight") == 0)
        self->state = MS_TWEAKS_STATE_SECTION_WEIGHT;
      else if (strcmp (prop_name, "settings") == 0)
        self->state = MS_TWEAKS_STATE_SETTING;
      else {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_UNEXPECTED_SCALAR_IN_SECTION,
                     "Unexpected scalar in section: %s",
                     prop_name);
        remove_current_section (self);
        return FALSE;
      }
      break;
    case YAML_SEQUENCE_START_EVENT:
      /* Nothing to do? */
      break;
    case YAML_SEQUENCE_END_EVENT:
      /* End of the section mapping. */
      self->state = MS_TWEAKS_STATE_PAGE;
      break;
    case YAML_MAPPING_START_EVENT:
      /* Start of new section. */
      g_assert (!self->current_section);

      self->current_section = g_new0 (MsTweaksSection, 1);
      self->current_section->weight = CONF_TWEAKS_DEFAULT_WEIGHT;
      self->current_section->setting_table = g_hash_table_new_full (g_str_hash,
                                                                    g_str_equal,
                                                                    g_free,
                                                                    (GDestroyNotify) ms_tweaks_setting_free);
      self->current_section_inserted = FALSE;
      /* We don't know the name of the section yet, so we can't add it to the table. It gets added
       * later in STATE_SECTION_NAME. */
      break;
    case YAML_MAPPING_END_EVENT:
      g_assert (self->current_section);

      if (!self->current_section_inserted && self->current_section)
        ms_tweaks_section_free (self->current_section);
      self->current_section = NULL;
      /* There may be another section mapping after this,
       * so don't switch state to STATE_PAGE. */
      break;
    default:
      report_error (self->state, event, error);
      remove_current_section (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_PAGE_WEIGHT:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_page);

      self->current_page->weight = atoi ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_PAGE;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_section (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SECTION_NAME:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      MsTweaksSection *existing_section = NULL;

      g_assert (self->current_section);

      if (self->current_section->name) {
        g_warning ("Section '%s' has 'name' defined more than once, using first definition",
                   self->current_section->name);
        break;
      }

      if (g_utf8_strlen ((char *) event->data.scalar.value, -1) == 0) {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_EMPTY_SECTION_DECLARATION,
                     "Empty section name declarations are not allowed");
        remove_current_section (self);
        self->state = MS_TWEAKS_STATE_SECTION;
        return FALSE;
      }

      self->current_section->name_i18n = g_strdup (_((char *) event->data.scalar.value));
      self->current_section->name = g_strdup ((char *) event->data.scalar.value);

      existing_section = g_hash_table_lookup (self->current_page->section_table,
                                              self->current_section->name);

      g_assert (self->current_section != existing_section);

      if (existing_section)
        merge_sections (self->current_section, existing_section);

      g_hash_table_replace (self->current_page->section_table,
                            g_strdup (self->current_section->name),
                            self->current_section);
      self->current_section_inserted = TRUE;
      self->state = MS_TWEAKS_STATE_SECTION;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_section (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SECTION_WEIGHT:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_section);

      self->current_section->weight = atoi ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SECTION;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_section (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      const char *prop_name = (char *) event->data.scalar.value;

      if (strcmp (prop_name, "name") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_NAME;
      else if (strcmp (prop_name, "weight") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_WEIGHT;
      else if (strcmp (prop_name, "type") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_TYPE;
      else if (strcmp (prop_name, "gtype") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_GTYPE;
      else if (strcmp (prop_name, "stype") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_STYPE;
      else if (strcmp (prop_name, "map") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_MAP;
      else if (strcmp (prop_name, "backend") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_BACKEND;
      else if (strcmp (prop_name, "help") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_HELP;
      else if (strcmp (prop_name, "default") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_DEFAULT;
      else if (strcmp (prop_name, "key") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_KEY;
      else if (strcmp (prop_name, "readonly") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_READONLY;
      else if (strcmp (prop_name, "source_ext") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_SOURCE_EXT;
      else if (strcmp (prop_name, "selector") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_SELECTOR;
      else if (strcmp (prop_name, "guard") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_GUARD;
      else if (strcmp (prop_name, "data") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_DATA;
      else if (strcmp (prop_name, "multiplier") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_MULTIPLIER;
      else if (strcmp (prop_name, "min") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_MIN;
      else if (strcmp (prop_name, "max") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_MAX;
      else if (strcmp (prop_name, "step") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_STEP;
      else if (strcmp (prop_name, "css") == 0)
        self->state = MS_TWEAKS_STATE_SETTING_CSS;
      else {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_UNEXPECTED_SCALAR_IN_SETTING,
                     "Unexpected scalar in setting: %s",
                     prop_name);
        remove_current_setting (self);
        return FALSE;
      }
      break;
    case YAML_SEQUENCE_START_EVENT:
      break;
    case YAML_SEQUENCE_END_EVENT:
      self->state = MS_TWEAKS_STATE_SECTION;
      break;
    case YAML_MAPPING_START_EVENT:
      /* Start of new setting. */
      g_assert (!self->current_setting);

      self->current_setting = g_new (MsTweaksSetting, 1);
      self->current_setting->weight = CONF_TWEAKS_DEFAULT_WEIGHT;
      self->current_setting->name = CONF_TWEAKS_DEFAULT_NAME;
      self->current_setting->type = CONF_TWEAKS_DEFAULT_TYPE;
      self->current_setting->gtype = CONF_TWEAKS_DEFAULT_GTYPE;
      self->current_setting->stype = CONF_TWEAKS_DEFAULT_STYPE;
      self->current_setting->map = CONF_TWEAKS_DEFAULT_MAP;
      self->current_setting->backend = CONF_TWEAKS_DEFAULT_BACKEND;
      self->current_setting->help = CONF_TWEAKS_DEFAULT_HELP;
      self->current_setting->default_ = CONF_TWEAKS_DEFAULT_DEFAULT_VALUE;
      /* Might as well preallocate 1 element as any valid setting will have a key. */
      self->current_setting->key = g_ptr_array_new_full (1, g_free);
      self->current_setting->readonly = CONF_TWEAKS_DEFAULT_READONLY;
      self->current_setting->selector = CONF_TWEAKS_DEFAULT_SELECTOR;
      self->current_setting->guard = CONF_TWEAKS_DEFAULT_GUARD;
      self->current_setting->multiplier = CONF_TWEAKS_DEFAULT_MULTIPLIER;
      /* min, max, and step have no default values in the original Python
       * implementation. Set them to the obviously wrong NaN by default. */
      self->current_setting->min = NAN;
      self->current_setting->max = NAN;
      self->current_setting->step = NAN;
      self->current_setting->css = CONF_TWEAKS_DEFAULT_CSS;

      self->current_setting->name_i18n = CONF_TWEAKS_DEFAULT_NAME;
      self->current_setting->help_i18n = CONF_TWEAKS_DEFAULT_HELP;
      /* We don't know the name of the setting yet, so we can't add it to the table. It gets added
       * later in STATE_SETTING_NAME. */
      self->current_setting_inserted = FALSE;
      break;
    case YAML_MAPPING_END_EVENT:
      g_assert (self->current_setting);

      if (self->current_setting->gtype == MS_TWEAKS_GTYPE_UNKNOWN) {
        MsTweaksSettingGsettingType ctx_gtype = MS_TWEAKS_GTYPE_UNKNOWN;

        /* In some parts of the original code, type is used as a fallback for gtype. While there
         * it's handled in the backend implementations, we're handling it in the parser here to
         * simplify our backend implementations. */
        switch (self->current_setting->type) {
        case MS_TWEAKS_TYPE_BOOLEAN:
          ctx_gtype = MS_TWEAKS_GTYPE_BOOLEAN;
          break;
        case MS_TWEAKS_TYPE_NUMBER:
          ctx_gtype = MS_TWEAKS_GTYPE_NUMBER;
          break;
        case MS_TWEAKS_TYPE_CHOICE:
        case MS_TWEAKS_TYPE_COLOR:
        case MS_TWEAKS_TYPE_FILE:
        case MS_TWEAKS_TYPE_FONT:
        case MS_TWEAKS_TYPE_INFO:
        case MS_TWEAKS_TYPE_UNKNOWN:
        default:
          break;
        }

        self->current_setting->gtype = ctx_gtype;
      }

      if (!self->current_setting_inserted && self->current_setting)
        ms_tweaks_setting_free (self->current_setting);
      self->current_setting = NULL;
      /* There may be another setting mapping after this,
       * so don't switch state to STATE_SECTION. */
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_NAME:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      MsTweaksSetting *existing_setting = NULL;

      g_assert (self->current_setting);

      if (self->current_setting->name) {
        g_warning ("Setting '%s' has 'name' defined more than once, using first definition",
                   self->current_setting->name);
        break;
      }

      if (g_utf8_strlen ((char *) event->data.scalar.value, -1) == 0) {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_EMPTY_SETTING_DECLARATION,
                     "Empty setting name declarations are not allowed");
        remove_current_setting (self);
        self->state = MS_TWEAKS_STATE_SETTING;
        return FALSE;
      }

      self->current_setting->name_i18n = g_strdup (_((char *) event->data.scalar.value));
      self->current_setting->name = g_strdup ((char *) event->data.scalar.value);

      existing_setting = g_hash_table_lookup (self->current_section->setting_table,
                                              self->current_setting->name);

      g_assert (self->current_setting != existing_setting);

      if (existing_setting)
        merge_settings (self->current_setting, existing_setting);

      g_hash_table_replace (self->current_section->setting_table,
                            g_strdup (self->current_setting->name),
                            self->current_setting);
      self->current_setting_inserted = TRUE;
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_HELP:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->help_i18n = g_strdup (_((char *) event->data.scalar.value));
      self->current_setting->help = g_strdup ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_WEIGHT:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->weight = atoi ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_TYPE:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->type = str_to_setting_type ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_GTYPE:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->gtype = str_to_gsettings_type ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_STYPE:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->stype = str_to_sysfs_type ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_MAP: /* Intentional fallthrough. */
  case MS_TWEAKS_STATE_SETTING_CSS:
    switch (self->state) {
    case MS_TWEAKS_STATE_SETTING_MAP:
      ctx_hash_table_ptr = &self->current_setting->map;
      break;
    case MS_TWEAKS_STATE_SETTING_CSS:
      ctx_hash_table_ptr = &self->current_setting->css;
      break;
    default:
      g_warn_if_reached ();
      break;
    }

    switch (event->type) {
    case YAML_SCALAR_EVENT:
      switch (self->setting_mapping_state) {
      case MS_TWEAKS_MAPPING_STATE_KEY:
        self->last_key_name = g_strdup ((char *) event->data.scalar.value);

        self->setting_mapping_state = MS_TWEAKS_MAPPING_STATE_VALUE;
        break;
      case MS_TWEAKS_MAPPING_STATE_VALUE:
        g_assert (self->last_key_name);

        g_hash_table_insert (*ctx_hash_table_ptr,
                             g_steal_pointer (&self->last_key_name),
                             g_strdup ((char *) event->data.scalar.value));

        self->setting_mapping_state = MS_TWEAKS_MAPPING_STATE_KEY;
        break;
      default:
        g_assert_not_reached ();
      }
      break;
    case YAML_MAPPING_START_EVENT:
      g_assert (self->setting_mapping_state == MS_TWEAKS_MAPPING_STATE_KEY);

      if (*ctx_hash_table_ptr)
        g_hash_table_unref (*ctx_hash_table_ptr);

      *ctx_hash_table_ptr = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

      break;
    case YAML_MAPPING_END_EVENT:
      g_assert (self->setting_mapping_state == MS_TWEAKS_MAPPING_STATE_KEY);

      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_BACKEND:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->backend = str_to_backend ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_DEFAULT:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->default_ = g_strdup ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_KEY:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      g_ptr_array_add (self->current_setting->key, g_strdup ((char *) event->data.scalar.value));

      if (!self->in_setting_key_list)
        self->state = MS_TWEAKS_STATE_SETTING;
      break;
    case YAML_SEQUENCE_START_EVENT:
      self->in_setting_key_list = TRUE;
      break;
    case YAML_SEQUENCE_END_EVENT:
      self->in_setting_key_list = FALSE;
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_READONLY:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      gboolean boolean_representation = FALSE;

      g_assert (self->current_setting);

      if (str_to_bool ((char *) event->data.scalar.value, &boolean_representation) == EINVAL) {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_INVALID_BOOLEAN_VALUE_IN_READONLY_PROPERTY,
                     "Invalid YAML bool value '%s' in readonly property",
                     (char *) event->data.scalar.value);
        remove_current_setting (self);
        return FALSE;
      }

      self->current_setting->readonly = boolean_representation;

      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_SOURCE_EXT:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      gboolean boolean_representation = FALSE;

      g_assert (self->current_setting);

      if (str_to_bool ((char *) event->data.scalar.value, &boolean_representation) == EINVAL) {
        g_set_error (error,
                     MS_TWEAKS_PARSER_ERROR,
                     MS_TWEAKS_PARSER_ERROR_INVALID_BOOLEAN_VALUE_IN_SOURCE_EXT_PROPERTY,
                     "Invalid YAML bool value '%s' in source_ext property",
                     (char *) event->data.scalar.value);
        remove_current_setting (self);
        return FALSE;
      }

      self->current_setting->source_ext = boolean_representation;

      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_SELECTOR:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->selector = g_strdup ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_GUARD:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->guard = g_strdup ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_DATA:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      if (self->current_setting->map)
        g_hash_table_unref (self->current_setting->map);

      self->current_setting->map = ms_tweaks_datasources_get_map ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_MULTIPLIER:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->multiplier = atoi ((char *) event->data.scalar.value);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_MIN:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->min = g_ascii_strtod ((char *) event->data.scalar.value, NULL);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_MAX:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->max = g_ascii_strtod ((char *) event->data.scalar.value, NULL);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_SETTING_STEP:
    switch (event->type) {
    case YAML_SCALAR_EVENT:
      g_assert (self->current_setting);

      self->current_setting->step = g_ascii_strtod ((char *) event->data.scalar.value, NULL);
      self->state = MS_TWEAKS_STATE_SETTING;
      break;
    default:
      report_error (self->state, event, error);
      remove_current_setting (self);
      return FALSE;
    }
    break;
  case MS_TWEAKS_STATE_STOP:
    break;
  default:
    g_error ("Unimplemented case %i", self->state);
    return FALSE;
  }

#pragma GCC diagnostic pop

  return TRUE;
}

/**
 * ms_tweaks_parser_parse_fragment:
 * @self: Instance of MsTweaksParser.
 * @to_parse: String of YAML markup to parse.
 * @size: Length of `to_parse`.
 * @error: Will be filled in the case of an error.
 *
 * Parses `to_parse` and populates `self` accordingly.
 */
static gboolean
ms_tweaks_parser_parse_fragment (MsTweaksParser  *self,
                                 const guchar    *to_parse,
                                 const size_t     size,
                                 GError         **error)
{
  gboolean done = FALSE;
  yaml_parser_t parser;
  yaml_event_t event;

  yaml_parser_initialize (&parser);
  yaml_parser_set_input_string (&parser, to_parse, size);
  self->state = MS_TWEAKS_STATE_START;

  while (!done) {
    if (!yaml_parser_parse (&parser, &event)) {
      g_set_error (error,
                   MS_TWEAKS_PARSER_ERROR,
                   MS_TWEAKS_PARSER_ERROR_FAILURE,
                   "Failure while parsing");
      yaml_parser_delete (&parser);
      return FALSE;
    }

    if (!consume_event (self, &event, error)) {
      yaml_event_delete (&event);
      yaml_parser_delete (&parser);
      return FALSE;
    }

    done = (event.type == YAML_STREAM_END_EVENT);

    yaml_event_delete (&event);
  }

  yaml_parser_delete (&parser);

  return TRUE;
}

/**
 * ms_tweaks_parser_parse_definition_files:
 * @self: Instance of MsTweaksParser.
 * @tweaks_yaml_path: Path to a directory containing postmarketOS Tweaks-compatible YAML definitions
 *                    files. Must be a directory, cannot point directly to such a file.
 *
 * Parses all files with the .yaml or .yml extensions in the directory specified by
 * `tweaks_yaml_path` and populates `self` accordingly. Files with other extensions or no extension
 * at all are ignored.
 */
void
ms_tweaks_parser_parse_definition_files (MsTweaksParser *self, const char *tweaks_yaml_path)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GDir) yaml_directory = g_dir_open (tweaks_yaml_path, 0, &error);
  const char *yaml_filename_current;

  g_assert (MS_IS_TWEAKS_PARSER (self));

  if (gm_str_is_null_or_empty (tweaks_yaml_path)) {
    g_debug ("No path configured for conf-tweaks");
    return;
  }

  if (!yaml_directory) {
    g_warning ("Couldn't open conf-tweaks YAML directory at '%s': %s.\nNo tweaks definitions will be read.",
               tweaks_yaml_path,
               error->message);
    return;
  }

  while ((yaml_filename_current = g_dir_read_name (yaml_directory)) != NULL) {
    g_autofree char *yaml_filepath_current = NULL;
    const char *file_extension = NULL;
    g_autofree char *contents = NULL;
    gsize contents_length = 0;

    if (yaml_filename_current[0] == '.')
      continue; /* Skip the current directory, the parent directory, and any dotfiles. */

    file_extension = ms_tweaks_get_filename_extension (yaml_filename_current);

    if (!(g_str_equal (file_extension, "yml") || g_str_equal (file_extension, "yaml")))
      continue; /* Skip files that don't have the right file extension. */

    yaml_filepath_current = g_build_path ("/", tweaks_yaml_path, yaml_filename_current, NULL);

    if (!g_file_get_contents (yaml_filepath_current, &contents, &contents_length, &error)) {
      g_warning ("Failed to open '%s': %s", yaml_filepath_current, error->message);
      g_clear_error (&error);
      continue;
    }

    if (!ms_tweaks_parser_parse_fragment (self, (guchar *) contents, contents_length, &error)) {
      g_warning ("Failure while parsing '%s': %s", yaml_filepath_current, error->message);
      g_clear_error (&error);
      continue;
    }
  }

  if (g_hash_table_size (self->page_table) == 0) {
    g_warning ("The conf-tweaks YAML directory '%s' doesn't contain any valid tweak definition files",
               tweaks_yaml_path);
  }
}


static void
ms_tweaks_parser_class_init (MsTweaksParserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ms_tweaks_parser_dispose;
}


static gint
compare_by_weight (gconstpointer structure_a, gconstpointer structure_b)
{
  const int *weight_a;
  const int *weight_b;

  /* The cast below only works if these assertions hold true, if they fail the sorting will be
   * completely broken. */
  G_STATIC_ASSERT (offsetof (MsTweaksSetting, weight) == 0);
  G_STATIC_ASSERT (offsetof (MsTweaksSection, weight) == 0);
  G_STATIC_ASSERT (offsetof (MsTweaksPage, weight) == 0);

  weight_a = structure_a;
  weight_b = structure_b;

  return *weight_a - *weight_b;
}

/**
 * ms_tweaks_parser_sort_by_weight:
 * @hash_table: Hash table containing either MsTweaksSetting, MsTweaksSection, or MsTweaksPage
 *              values.
 *
 * Returns: The entries of the hash table sorted by their weight property.
 */
GList *
ms_tweaks_parser_sort_by_weight (GHashTable *hash_table)
{
  GList *hash_table_values = g_hash_table_get_values (hash_table);

  return g_list_sort (hash_table_values, compare_by_weight);
}


MsTweaksParser *
ms_tweaks_parser_new (void)
{
  return g_object_new (MS_TYPE_TWEAKS_PARSER, NULL);
}
