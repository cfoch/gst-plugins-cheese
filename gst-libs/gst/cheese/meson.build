face_sources = [
  'face/cheesefaceinfo.c',
  'face/cheesemultifaceinfo.c',
  'face/cheesemultifacemeta.c',
  'face/cheesefacespriteframe.c',
  'face/cheesefacespritekeypoint.c',
  'face/cheesefacesprite.c',
  'face/cheesemultifacesprite.c'
]

face_headers = [
  'face/cheesefaceinfo.h',
  'face/cheesemultifaceinfo.h',
  'face/cheesemultifacemeta.h',
  'face/cheesefacespriteframe.h',
  'face/cheesefacespritekeypoint.h',
  'face/cheesefacesprite.h',
  'face/cheesemultifacesprite.h'
]

graphene_dep = dependency('graphene-1.0', version : ['>= 1.7.0'],
  required : true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version : ['>= 2.36.0'],
  required : true)
json_glib_dep = dependency('json-glib-1.0', version : ['>= 1.4.3'],
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
      graphene_dep,
      gdk_pixbuf_dep,
      json_glib_dep
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
