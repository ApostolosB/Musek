deps = [
  dependency('elementary'),
  dependency('taglib_c'),
]

srcs = [
  'bin/main.c',
  'bin/library.c',
  'bin/playback.c',
  'bin/scanner.c',
  'bin/ui_callbacks.c',
  'bin/ui_populate.c',
  'bin/ui_setup.c',
  'bin/ui_settings.c',
  'bin/ui_albumart.c',
  'bin/artist_image_fetch.c',
  'bin/search.c',
]

executable(
  'musek',
  srcs,
  dependencies: deps,
  install: true,
  install_dir: get_option('bindir')
)
