dep_xcb = dependency('xcb')

src_wsi_xcb = files(
  'platform.c',
  'event_queue.c',
  'window.c',
  'vulkan/vulkan.c',
)

lib_wsi_xcb = library('wsi-xcb',
  src_wsi_xcb,
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_vulkan,
    dep_xcb,
  ],
)

dep_wsi_xcb = declare_dependency(
  link_with : lib_wsi_xcb,
  include_directories : inc_wsi,
)
