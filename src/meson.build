deps = [
  dependency('elementary'),
  dependency('taglib_c'),
]

srcs = [
  'main.c',
  'library.c',
  'playback.c',
  'scanner.c',
  'ui_callbacks.c',
  'ui_populate.c',
  'ui_setup.c',
  'ui_settings.c',
  'ui_albumart.c',
  'artist_image_fetch.c',
]

executable(
  'musek',
  srcs,
  dependencies: deps,
  install: true,
  install_dir: get_option('bindir')
)
