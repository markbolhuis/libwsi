dep_xcb = dependency('xcb', version : '>=1.15')

src_wsi_x11 = files(
  'platform.c',
  'input.c',
  'window.c',
  'egl.c',
  'vulkan.c',
  'utils.c',
)

dep_egl    = dependency('egl', version : '>=1.4.0')

lib_wsi_x11 = library('wsi-x11',
  src_wsi_x11,
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_egl,
    dep_vulkan,
    dep_xcb,
  ],
)

dep_wsi_x11 = declare_dependency(
  link_with : lib_wsi_x11,
  include_directories : inc_wsi,
)
