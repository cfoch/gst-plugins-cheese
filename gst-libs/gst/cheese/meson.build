face_sources = [
  'face/cheesefaceinfo.c',
  'face/cheesemultifaceinfo.c',
  'face/cheesemultifacemeta.c'
]

face_headers = [
  'face/cheesefaceinfo.h',
  'face/cheesemultifaceinfo.h',
  'face/cheesemultifacemeta.h'
]

graphene_dep = dependency('graphene-1.0', version : ['>= 1.7.0'],
  required : true)

if gst_dep.found() and graphene_dep.found()

  install_headers(face_headers, subdir : 'gstreamer-1.0/gst/cheese/face')

  gstcheese = library('gstcheese-@0@'.format(api_version),
    face_sources,
    c_args : gst_plugins_bad_args,
    link_args: ['-lm'],
    include_directories: [configinc, libsinc],
    version : libversion,
    soversion : soversion,
    install : true,
    dependencies : [
      gst_dep,
      graphene_dep
    ]
  )

  gstcheese_dep = declare_dependency(link_with : gstcheese,
    include_directories : [libsinc],
    sources: [],
    dependencies : [
      gst_dep,
      graphene_dep
    ]
  )
endif
