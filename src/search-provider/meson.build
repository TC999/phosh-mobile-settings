service_conf = configuration_data()
service_conf.set('libexecdir', libexecdir)

service = 'mobi.phosh.MobileSettings.service'

configure_file(
          input : service + '.in',
         output : service,
        install : true,
    install_dir : join_paths(datadir, 'dbus-1', 'services'),
  configuration : service_conf
)

install_data(
  'mobi.phosh.MobileSettings.search-provider.ini',
  install_dir: join_paths(datadir, 'gnome-shell', 'search-providers')
)

sources = files(
  'ms-search-provider.c',
  'ms-search-provider-app.c'
)

sources += gnome.gdbus_codegen(
  'ms-shell-search-provider-generated',
  'mobi.phosh.ShellSearchProvider2.xml',
  interface_prefix : 'mobi.phosh.',
         namespace : 'Ms'
)

search_provider_deps = [
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gtk_dep,
  adwaita_dep,
]

executable(
  'phosh-mobile-settings-search-provider',
               sources,
  include_directories : top_inc,
         dependencies : search_provider_deps,
              install : true,
          install_dir : libexecdir
)

