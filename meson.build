project('libwsi', 'c',
  version : '0.0.1',
  license : 'Apache 2.0',
  default_options : [
    'c_std=c11',
    'warning_level=2',
  ],
  meson_version : '>= 0.63.0',
)

cc = meson.get_compiler('c')
lib_m = cc.find_library('m')

add_project_arguments(
  '-Wno-unused-parameter',
  language : 'c',
)

dep_egl    = dependency('egl', version : '>=1.4.0')
dep_vulkan = dependency('vulkan', version : '>=1.0.0')

inc_wsi = include_directories('include')

subdir('include')
subdir('src')

if get_option('examples')
  subdir('examples')
endif
