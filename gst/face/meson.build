hungariandir = join_paths(meson.source_root(), 'hungarian-algorithm-cpp')
face_sources = [
  'gstcheesefacedetect.cpp',
  'gstcheesefacetrack.cpp',
  'gstcheesefaceomelette.cpp',
  'gstcheesefaceoverlay.c',
  'gstcheesefaceeffects.cpp',
  join_paths(hungariandir, 'Hungarian.cpp')
]

opencv_dep = dependency('opencv', version : ['>= 2.3.0', '< 3.5.0'],
  required : true)
dlib_dep = dependency('dlib-1', version : ['>= 19.0.0', '< 19.9.0'],
  required : true)
graphene_dep = dependency('graphene-1.0', version : ['>= 1.7.0'],
  required : true)
graphene_gobject_dep = dependency('graphene-gobject-1.0',
  version : ['>= 1.7.0'], required : true)
gdk_dep = dependency('gdk-3.0', version : ['>= 3.22'], required : true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version : ['>= 2.36.0'],
  required : true)
cairo_dep = dependency('cairo', version : ['>= 1.15.0'], required : true)

if (opencv_dep.found() and dlib_dep.found() and graphene_dep.found()
    and graphene_gobject_dep.found() and gdk_pixbuf_dep.found()
    and cairo_dep.found())
  face_args = []
  if host_machine.cpu_family().startswith('x86')
    face_args += '-DUSE_SSE41'
  endif

  install_subdir('data/pixmaps/omelette', install_dir : join_paths(datadir, 'pixmaps/gst/cheese/face/'))
  gstface = library('gstcheesefaceeffects',
    face_sources,
    cpp_args : gst_plugins_bad_args + face_args,
    link_args : ['-lgstopencv-1.0', '-lgslcblas'],
    include_directories : [configinc, hungarianinc],
    dependencies : [gstbase_dep, gstvideo_dep, opencv_dep, dlib_dep,
                    graphene_dep, graphene_gobject_dep, gdk_dep, gdk_pixbuf_dep,
                    cairo_dep, gstcheese_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
endif
