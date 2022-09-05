project('libwsi', 'c',
  version : '0.0.1',
  license : 'Apache 2.0',
  default_options : [
    'c_std=c11',
    'warning_level=0',
  ],
  meson_version : '>= 0.63.0',
)

dep_vulkan = dependency('vulkan')

inc_wsi = include_directories('include')

subdir('include')
subdir('src')
