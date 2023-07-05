dep_cocoa = dependency('Cocoa')
dep_quartz_core = dependency('QuartzCore')

src_wsi_cocoa = files(
  'platform.m',
  'input.c',
  'window.m',
  #'egl.c',
  'vulkan.m',
  'utils.c',
)

lib_wsi_cocoa = library('wsi-cocoa',
  src_wsi_cocoa,
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_vulkan,
    dep_cocoa,
    dep_quartz_core,
  ],
)

dep_wsi_cocoa = declare_dependency(
  link_with : lib_wsi_cocoa,
  include_directories : inc_wsi,
)
