wl_mod = import('unstable-wayland')

dep_wl_client = dependency('wayland-client', version: '>= 1.21.0')
dep_wl_cursor = dependency('wayland-cursor', version: '>= 1.21.0')
dep_wl_egl    = dependency('wayland-egl', version: '>= 1.21.0')
dep_xkbcommon = dependency('xkbcommon', version: '>= 1.4.0')

src_wsi_wl = files(
  'platform.c',
  'input.c',
  'window.c',
  'output.c',
  'egl.c',
  'vulkan.c',
)

wl_xmls = [
  wl_mod.find_protocol('viewporter'),
  wl_mod.find_protocol('fractional-scale', state : 'staging', version : 1),
  wl_mod.find_protocol('input-timestamps', state : 'unstable', version : 1),
  wl_mod.find_protocol('xdg-shell'),
  wl_mod.find_protocol('xdg-output', state : 'unstable', version : 1),
  wl_mod.find_protocol('xdg-decoration', state : 'unstable', version : 1),
]

lib_wsi_wl = library('wsi-wl',
  src_wsi_wl,
  wl_mod.scan_xml(wl_xmls),
  include_directories : inc_wsi,
  install : true,
  dependencies : [
    dep_vulkan,
    dep_egl,
    dep_wl_client,
    dep_wl_cursor,
    dep_wl_egl,
    dep_xkbcommon,
  ],
)

dep_wsi_wl = declare_dependency(
  link_with : lib_wsi_wl,
  include_directories : inc_wsi,
)
