project('musek', 'c',
        version: '0.1',
        meson_version: '>= 0.56.0')

proj = meson.project_name()
ver  = meson.project_version()

# Install locations
dir_prefix = get_option('prefix')
dir_data   = join_paths(dir_prefix, get_option('datadir'))

# Generate config header
cfg = configuration_data()
cfg.set_quoted('PACKAGE', proj)
cfg.set_quoted('PACKAGE_VERSION', ver)
cfg.set_quoted('PACKAGE_DATA_DIR', join_paths(dir_data, proj))

configure_file(
  output: 'musek_config.h',
  configuration: cfg
)

subdir('src')
subdir('data')
