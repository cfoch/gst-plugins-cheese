face_sources = [
  'gstcheesefacedetect.cpp',
  'gstcheesefaceomelette.cpp',
  'gstcheesefaceoverlay.c',
  'gstcheesefaceeffects.cpp',
  'Hungarian.cpp'
]

face_headers = [
  'gstcheesefacedetect.h',
  'Hungarian.h'
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
gio_dep = dependency('gio-2.0', version : ['>= 2.55.0'], required : true)

if (opencv_dep.found() and dlib_dep.found() and graphene_dep.found()
    and graphene_gobject_dep.found() and gdk_pixbuf_dep.found()
    and cairo_dep.found() and gio_dep.found())
  face_args = []

  opencv_prefix = opencv_dep.get_pkgconfig_variable('prefix')
  dlib_prefix = dlib_dep.get_pkgconfig_variable('prefix')
  graphene_prefix = dlib_dep.get_pkgconfig_variable('prefix')
  graphene_gobject_prefix = dlib_dep.get_pkgconfig_variable('prefix')
  gdk_prefix = gdk_dep.get_pkgconfig_variable('prefix')
  gdk_pixbuf_prefix = gdk_pixbuf_dep.get_pkgconfig_variable('prefix')
  cairo_prefix = cairo_dep.get_pkgconfig_variable('prefix')
  gio_prefix = gio_dep.get_pkgconfig_variable('prefix')

  face_args += ['-DOPENCV_PREFIX="' + opencv_prefix + '"']
  face_args += ['-DDLIB_PREFIX="' + dlib_prefix + '"']
  face_args += ['-DGRAPHENE_PREFIX="' + graphene_prefix + '"']
  face_args += ['-DGRAPHENE_GOBJECT_PREFIX="' + graphene_gobject_prefix + '"']
  face_args += ['-DGDK_PIXBUF_PREFIX="' + gdk_prefix + '"']
  face_args += ['-DGDK_PIXBUF_PREFIX="' + gdk_pixbuf_prefix + '"']
  face_args += ['-DGIO_PREFIX="' + gio_prefix + '"']

  if host_machine.cpu_family().startswith('x86')
    face_args += '-DUSE_SSE41'
  endif

  install_subdir('data/pixmaps/omelette', install_dir : join_paths(datadir, 'pixmaps/gst/cheese/face/'))
  gstface = library('gstcheesefaceeffects',
    face_sources,
    cpp_args : gst_plugins_bad_args + face_args + [ '-DGST_USE_UNSTABLE_API' ],
    link_args : ['-lgstopencv-1.0', '-lgslcblas'],
    include_directories : [configinc],
    dependencies : [gstbase_dep, gstvideo_dep, opencv_dep, dlib_dep,
                    graphene_dep, graphene_gobject_dep, gdk_dep, gdk_pixbuf_dep,
                    cairo_dep, gio_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
endif
