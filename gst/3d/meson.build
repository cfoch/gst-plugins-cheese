# These have to stay in -bad until we can move GstVideoAggregator to -base
#  'gstglbasemixer.c',
#  'gstglmixerbin.c',
#  'gstglmixer.c',
#  'gstglvideomixer.c',
#  'gstglstereomix.c',
cheese_3d_sources = [
  'gstcheese3d.c',
  'gstcheese3dfilter.c',
  'gstcheese3dtestsrc.c',
  'cheese3dtestsrc.c'
]


gst3d_dep = dependency('gst3d-1.0', version : ['>= 0.1.0.0', '< 0.2.0.0'],
  required : true)
graphene_dep = dependency('graphene-1.0', fallback : ['graphene', 'graphene_dep'])
graphene_gobject_dep = dependency('graphene-gobject-1.0',
  version : ['>= 1.7.0'], required : true)
gobject_dep = dependency('gobject-2.0')
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')

if (gstgl_dep.found() and opencv_dep.found() and graphene_dep.found()
    and graphene_gobject_dep.found() and gobject_dep.found()
    and glib_dep.found() and gio_dep.found())
  optional_deps = []
'''
  if gl_dep.found() # have desktop GL
    # These have to stay in -bad until we can move GstVideoAggregator to -base
    # 'gstglmosaic.c',
    opengl_sources += [
      'gstglfilterglass.c',
    ]
  endif

  graphene_dep = dependency('graphene-1.0', version : '>=1.4.0', required : false)
  if graphene_dep.found()
    optional_deps += graphene_dep
    core_conf.set('HAVE_GRAPHENE', true)
    opengl_sources += [
      'gstgltransformation.c',
      'gstglvideoflip.c',
    ]
  endif

  png_dep = dependency('libpng', version : '>=1.0', required : false)
  jpeg_dep = cc.find_library('jpeg-mmx', required : false)
  if not jpeg_dep.found()
    jpeg_dep = cc.find_library('jpeg', required : false)
  endif

  if png_dep.found()
    optional_deps += png_dep
    core_conf.set('HAVE_PNG', true)
    opengl_sources += [
      'gstgldifferencematte.c',
    ]
    if jpeg_dep.found()
      optional_deps += jpeg_dep
      core_conf.set('HAVE_JPEG', true)
      opengl_sources += [
        'gstgloverlay.c',
      ]
    endif
  endif

  if false # have cocoa
    opengl_sources += [
      'caopengllayersink.m',
    ]
  endif

  if x11_dep.found()
    # for XInitThreads()
    optional_deps += x11_dep
  endif

  if bcm_host_dep.found()
    optional_deps += bcm_host_dep
  endif

  if egl_dep.found() and cc.has_header('libdrm/drm_fourcc.h', required : false)
    optional_deps += allocators_dep
  endif
'''

  # glib-compile-resources
  r = run_command('glib-compile-resources', '--target=gpu/shaders.c', '--sourcedir=gpu/', '--generate-source', 'gpu/shaders.gresource.xml')
  if r.returncode() != 0
    message('Could not compile glib resources:')
    message(r.stderr().strip())
  else
    cheese_3d_sources += ['gpu/shaders.c']

    gst3d_prefix = gst3d_dep.get_pkgconfig_variable('prefix')
    graphene_prefix = dlib_dep.get_pkgconfig_variable('prefix')
    graphene_gobject_prefix = dlib_dep.get_pkgconfig_variable('prefix')
    cheese3d_args = []
    cheese3d_args += ['-DGST3D_PREFIX="' + gst3d_prefix + '"']
    face_args += ['-DGRAPHENE_PREFIX="' + graphene_prefix + '"']
    face_args += ['-DGRAPHENE_GOBJECT_PREFIX="' + graphene_gobject_prefix + '"']

    gstcheese3d = library('gstcheese3d',
      cheese_3d_sources,
      c_args : gst_plugins_bad_args + cheese3d_args,
      link_args : [],
      include_directories : [configinc],
      dependencies : [gstgl_dep, gst3d_dep, gstvideo_dep, glib_dep,
                      graphene_dep, graphene_gobject_dep, gobject_dep, gio_dep,
                      gstbase_dep, gst_controller_dep] + optional_deps,
      install : true,
      install_dir : plugins_install_dir,
    )
  endif
endif
