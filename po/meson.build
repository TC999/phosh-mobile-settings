po_targets = i18n.gettext('phosh-mobile-settings', preset: 'glib')

if get_option('gcc-panels') and get_option('gcc-po')
  meson.add_postconf_script(
    './update-gcc-po',
  )
endif
