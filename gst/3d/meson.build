# These have to stay in -bad until we can move GstVideoAggregator to -base
#  'gstglbasemixer.c',
#  'gstglmixerbin.c',
#  'gstglmixer.c',
#  'gstglvideomixer.c',
#  'gstglstereomix.c',
cheese_3d_sources = [
  'gstcheese3d.c',
  'gstcheese3dfilter.c'
]

if gstgl_dep.found()
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

  gstcheese3d = library('gstcheese3d',
    cheese_3d_sources,
    c_args : gst_plugins_bad_args + [ '-DGST_USE_UNSTABLE_API' ],
    link_args : [],
    include_directories : [configinc],
    dependencies : [gstgl_dep, gstvideo_dep,
                    gstbase_dep, gst_controller_dep] + optional_deps,
    install : true,
    install_dir : plugins_install_dir,
  )
endif
