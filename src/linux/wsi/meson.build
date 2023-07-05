src_wsi = files(
  'platform.c',
  'output.c',
  'input.c',
  'window.c',
  'egl.c',
  'vulkan.c',
)

lib_wsi = library('wsi', src_wsi,
  include_directories: inc_wsi,
)

dep_wsi = declare_dependency(
  link_with: lib_wsi,
  include_directories: inc_wsi
)
