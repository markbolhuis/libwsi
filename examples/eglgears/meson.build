
dep_egl = dependency('egl')
dep_opengl = dependency('opengl')

executable('eglgears', 'eglgears.c',
  dependencies : [
    dep_egl,
    dep_opengl,
    dep_wsi,
    lib_m,
  ],
)
