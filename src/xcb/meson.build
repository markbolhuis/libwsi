dep_xcb = dependency('xcb', version : '>=1.15')

src_wsi_xcb = files(
  'platform.c',
  'event_queue.c',
  'window.c',
  'egl/egl.c',
  'vulkan/vulkan.c',
  'utils.c',
)

lib_wsi_xcb = library('wsi-xcb',
  src_wsi_xcb,
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_egl,
    dep_vulkan,
    dep_xcb,
  ],
)

dep_wsi_xcb = declare_dependency(
  link_with : lib_wsi_xcb,
  include_directories : inc_wsi,
)
