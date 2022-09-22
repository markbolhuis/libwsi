src_wsi = files(
  'platform.c',
  'event_queue.c',
  'seat.c',
  'window.c',
  'egl/egl.c',
)

lib_wsi = library('wsi', src_wsi,
  include_directories: inc_wsi,
)

dep_wsi = declare_dependency(
  link_with: lib_wsi,
  include_directories: inc_wsi
)
