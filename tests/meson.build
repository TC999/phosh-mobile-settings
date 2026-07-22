test_env_common = environment()
test_env_common.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env_common.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env_common.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env_common.set('GSETTINGS_BACKEND', 'memory')
test_env_common.set('GSETTINGS_SCHEMA_DIR', '@0@/data'.format(meson.project_build_root()))
test_env_common.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env_common.set('MALLOC_CHECK_', '2')
test_env_common.set('NO_AT_BRIDGE', '1')
test_env_common.set('GTK_A11Y', 'none')
test_env_common.set('ASAN_OPTIONS', 'fast_unwind_on_malloc=0')
test_env_common.set(
  'LSAN_OPTIONS',
  'suppressions=@0@/data/leak-suppress.txt'.format(meson.project_source_root()),
)

test_env_common.set('MS_FORCE_OSK', 'unknown')

subdir('utils')

phoc = find_program('phoc', required: false)

unit_tests = [
  'add-shortcut-dialog',
  'tweaks-backend-gsettings',
  'tweaks-backend-gtk3settings',
  'tweaks-backend-symlink',
  'tweaks-backend-sysfs',
  'tweaks-backend-xresources',
  'tweaks-datasources',
  'tweaks-gtk-utils',
  'tweaks-mappings',
  'tweaks-parser',
  'tweaks-preferences-page',
  'tweaks-utils',
]

foreach test : unit_tests
  t = executable(
    'test-@0@'.format(test),
    ['test-@0@.c'.format(test)] + test_utils_sources,
    dependencies: libms_dep,
    include_directories: [conf_tweaks_inc, ms_plugin_inc, dbus_inc],
  )
  test(test, t, env: test_env_common, suite: ['unit'])
endforeach

if phoc.found()
  test_env_phoc = test_env_common
  test_env_phoc.set('WLR_RENDERER', 'pixman')
  test_env_phoc.set('WLR_BACKENDS', 'headless')

  local_opts = ['debug-info', 'help', 'list', 'version']

  # Execute these tests under phoc
  foreach opt : local_opts
    test(
      'test-@0@'.format(opt),
      phoc,
      args: [
        '--no-xwayland',
        '-E',
        '@0@/src/phosh-mobile-settings --@1@'.format(meson.project_build_root(), opt),
      ],
      depends: [pms, compiled],
      env: test_env_phoc,
    )
  endforeach

endif
