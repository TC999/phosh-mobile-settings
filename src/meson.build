subdir('dbus')
subdir('conf-tweaks')

mobile_settings_enum_sources = gnome.mkenums_simple('ms-enum-types', sources: files('ms-enums.h'))

mobile_settings_plugin_sources = [
  'ms-plugin.h',
  'ms-plugin.c',
  'ms-plugin-panel.h',
  'ms-plugin-panel.c',
]

mobile_settings_sources = [
  'ms-about-panel.c',
  'ms-about-panel.h',
  'ms-alerts-panel.c',
  'ms-alerts-panel.h',
  'ms-application.c',
  'ms-application.h',
  'ms-audio-device-row.c',
  'ms-audio-device-row.h',
  'ms-audio-device.c',
  'ms-audio-device.h',
  'ms-audio-devices.c',
  'ms-audio-devices.h',
  'ms-cb-message-row.c',
  'ms-cb-message-row.h',
  'ms-completer-info.c',
  'ms-completer-info.h',
  'ms-compositor-panel.c',
  'ms-compositor-panel.h',
  'ms-convergence-panel.c',
  'ms-convergence-panel.h',
  'ms-debug-info.c',
  'ms-debug-info.h',
  'ms-features-panel.c',
  'ms-features-panel.h',
  'ms-feedback-panel.c',
  'ms-feedback-panel.h',
  'ms-feedback-row.c',
  'ms-feedback-row.h',
  'ms-head-tracker.c',
  'ms-head-tracker.h',
  'ms-hidden-app-row.c',
  'ms-hidden-app-row.h',
  'ms-hidden-apps-dialog.c',
  'ms-hidden-apps-dialog.h',
  'ms-lang-panel.c',
  'ms-lang-panel.h',
  'ms-lockscreen-panel.c',
  'ms-lockscreen-panel.h',
  'ms-main.c',
  'ms-main.h',
  'ms-osk-add-shortcut-dialog.c',
  'ms-osk-add-shortcut-dialog.h',
  'ms-osk-panel.c',
  'ms-osk-panel.h',
  'ms-overview-panel.c',
  'ms-overview-panel.h',
  'ms-panel-switcher.c',
  'ms-panel-switcher.h',
  'ms-panel.c',
  'ms-panel.h',
  'ms-plugin-list-box.c',
  'ms-plugin-list-box.h',
  'ms-plugin-loader.c',
  'ms-plugin-loader.h',
  'ms-plugin-row.c',
  'ms-plugin-row.h',
  'ms-scale-to-fit-row.c',
  'ms-scale-to-fit-row.h',
  'ms-sensor-panel.c',
  'ms-sensor-panel.h',
  'ms-sound-row.c',
  'ms-sound-row.h',
  'ms-topbar-panel.c',
  'ms-topbar-panel.h',
  'ms-toplevel-tracker.c',
  'ms-toplevel-tracker.h',
  'ms-updates-panel.c',
  'ms-updates-panel.h',
  'ms-util.c',
  'ms-util.h',
  'ms-welcome-panel.c',
  'ms-welcome-panel.h',
  'ms-window.c',
  'ms-window.h',
]

mobile_settings_deps = [
  accountsservice_dep,
  adwaita_dep,
  libfeedback_dep,
  libcbd_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gmobile_dep,
  gmodule_dep,
  gnome_desktop_dep,
  gsound_dep,
  gtk_dep,
  gtk_wayland_dep,
  json_glib_dep,
  libgvc_dep,
  libportal_dep,
  libportal_gtk4_dep,
  libpms_dep,
  phosh_plugins_dep,
  phosh_settings_dep,
  polkit_gobject_dep,
  wayland_client_dep,
  yaml_dep,
  cc.find_library('m', required: false),
]

gnome = import('gnome')

mobile_settings_resources = gnome.compile_resources(
  'mobile-settings-resources',
  'mobile-settings.gresource.xml',
  extra_args: '--manual-register',
  c_name: 'mobile_settings',
)

mobile_settings_generated_sources = [
  generated_dbus_sources,
  mobile_settings_enum_sources,
  mobile_settings_plugin_sources,
  mobile_settings_resources,
  wl_proto_headers,
  wl_proto_sources,
]

ms_lib = static_library(
  'ms',
  mobile_settings_sources,
  conf_tweaks_sources,
  mobile_settings_generated_sources,
  include_directories: [conf_tweaks_inc, proto_inc],
  dependencies: mobile_settings_deps,
  install: false,
)

libms_dep = declare_dependency(
  sources: mobile_settings_generated_sources,
  link_with: ms_lib,
  dependencies: mobile_settings_deps,
)

pms = executable('phosh-mobile-settings', 'main.c', install: true, dependencies: libms_dep)


# Helpers for plugins */
ms_plugin_inc = include_directories('.')
ms_plugin_deps = [glib_dep, adwaita_dep]
mobile_settings_plugin_lib = static_library(
  'mobile-settings-plugin',
  mobile_settings_plugin_sources,
  include_directories: [ms_plugin_inc, dbus_inc],
  dependencies: ms_plugin_deps,
)
mobile_settings_plugin_dep = declare_dependency(
  include_directories: [ms_plugin_inc, dbus_inc],
  link_with: mobile_settings_plugin_lib,
  dependencies: ms_plugin_deps,
)
