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
  'search.c',
  'settings.c',
]

# Include top-level directory so musek_config.h is visible
inc = include_directories('..')

executable(
  'musek',
  srcs,
  dependencies: deps,
  include_directories: inc,
  install: true,
  install_dir: get_option('bindir')
)
