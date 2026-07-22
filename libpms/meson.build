subdir('systemd-sysupdate')

api_version = '1.0'
libname = f'pms-@api_version@'

libpms_enum_sources = gnome.mkenums_simple(
  'libpms-enum-types',
  sources: files('libpms-enums.h'),
  install_header: true,
  install_dir: prefix / get_option('includedir') / libname,
)

libpms_osk_private_sources = [
  'ms-osk-add-layout-dialog.c',
  'ms-osk-layout-row.c',
]

libpms_lang_private_sources = [
  'lang/ms-util.c',
  'lang/ms-language-row.c',
  'lang/ms-common-language.c',
]

libpms_private_sources = [
  libpms_osk_private_sources,
  libpms_lang_private_sources,
  libpms_systemd_sysupdate_private_sources,
]

libpms_public_sources = [
  libpms_enum_sources,
  'ms-main.c',
  'ms-lang.c',
  'ms-os-update.c',
  'ms-os-updater.c',
  'ms-osk-layout-prefs.c',
  'ms-osk-layout.c',
  'lang/ms-language-chooser.c',
]

libpms_public_headers = [
  'ms-main.h',
  'ms-lang.h',
  'ms-os-update.h',
  'ms-os-updater.h',
  'ms-osk-layout-prefs.h',
  'libpms.h',
  'libpms-enums.h',
  'lang/ms-language-chooser.h',
]

libpms_deps = [
  adwaita_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gnome_desktop_dep,
  gtk_dep,
  json_glib_dep,
  dependency('fontconfig'),
  cc.find_library('m', required: false),
]

gnome = import('gnome')

libpms_resources = gnome.compile_resources(
  'libpms-resources',
  'libpms.gresource.xml',
)

libpms_generated_sources = [
  libpms_enum_sources,
  libpms_resources,
]

libpms_lib = shared_library(
  libname,
  libpms_public_sources + libpms_public_headers + libpms_private_sources,
  libpms_generated_sources,
  dependencies: libpms_deps,
  soversion: '0',
  install: true,
  c_args: '-D_LIBPMS_INSIDE',
)

install_headers(
  libpms_public_headers,
  preserve_path: true,
  subdir: libname,
)

libpms_dep = declare_dependency(
  sources: libpms_generated_sources,
  link_with: libpms_lib,
  include_directories: include_directories('.'),
  dependencies: libpms_deps,
)

if get_option('introspection')
  libpms_gir_extra_args = [
    '--quiet',
    '-DLIBPMS_COMPILATION',
    '-DLIBPMS_USE_UNSTABLE_API',
    '--doc-format=gi-docgen',
  ]

  libpms_gir = gnome.generate_gir(
    libpms_lib,
    header: 'libpms.h',
    sources: libpms_public_sources + libpms_public_headers,
    nsversion: api_version,
    namespace: 'Pms',
    export_packages: libname,
    symbol_prefix: 'ms',
    identifier_prefix: 'Ms',
    includes: ['Gio-2.0', 'Gtk-4.0', 'Adw-1'],
    install: true,
    extra_args: libpms_gir_extra_args,
    fatal_warnings: true,
  )
endif

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  name: 'libpms',
  description: 'Reusable widgets from Phosh Mobile Settings',
  filebase: libname,
  subdirs: libname,
  libraries: [libpms_lib],
  version: api_version,
  requires: 'gio-2.0',
  requires_private: 'libadwaita-1',
  install_dir: join_paths(libdir, 'pkgconfig'),
)
