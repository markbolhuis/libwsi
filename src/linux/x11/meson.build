dep_xcb = dependency('xcb', version : '>=1.14')
dep_xcb_xinput = dependency('xcb-xinput', version : '>=1.14')

src_wsi_x11 = files(
  'platform.c',
  'input.c',
  'window.c',
  'egl.c',
  'vulkan.c',
)

lib_wsi_x11 = library('wsi-x11',
  src_wsi_x11,
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_egl,
    dep_vulkan,
    dep_xcb,
    dep_xcb_xinput,
  ],
)

dep_wsi_x11 = declare_dependency(
  link_with : lib_wsi_x11,
  include_directories : inc_wsi,
)
