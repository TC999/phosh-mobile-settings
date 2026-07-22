systemd_sysupdate_dbus_inc = include_directories('.')

systemd_sysupdated_dbus_sources = gnome.gdbus_codegen(
  'ms-dbus-systemd-sysupdated',
  sources: ['org.freedesktop.sysupdate1.xml'],
  interface_prefix: 'org.freedesktop.sysupdate1',
  namespace: 'MsDBusSysupdate',
  extra_args: ['--glib-min-required=' + glib_dep.version()],
)

libpms_systemd_sysupdate_private_sources = [
  systemd_sysupdated_dbus_sources,
  'systemd-sysupdate/ms-systemd-sysupdate.c',
  'systemd-sysupdate/ms-systemd-sysupdate.h',
]
