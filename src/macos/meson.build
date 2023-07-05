dep_cocoa = dependency('Cocoa')
dep_quartz_core = dependency('QuartzCore')

src_wsi_cocoa = files(
  'platform.m',
  'input.c',
  'window.m',
  'vulkan.m',
)

lib_wsi = library('wsi-cocoa',
  src_wsi_cocoa,
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_vulkan,
    dep_cocoa,
    dep_quartz_core,
  ],
)

dep_wsi = declare_dependency(
  link_with : lib_wsi,
  include_directories : inc_wsi,
)
