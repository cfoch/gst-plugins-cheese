face_meta_sources = [
  'face/cheesefaceinfo.c',
  'face/cheesemultifaceinfo.c',
  'face/cheesemultifacemeta.c',
]

face_sprite_sources = [
  'face/cheesefacespriteframe.c',
  'face/cheesefacespritekeypoint.c',
  'face/cheesefacesprite.c',
  'face/cheesemultifacesprite.c'
]

face_meta_headers = [
  'face/cheesefaceinfo.h',
  'face/cheesemultifaceinfo.h',
  'face/cheesemultifacemeta.h'
]

face_sprite_headers = [
  'face/cheesefacespriteframe.h',
  'face/cheesefacespritekeypoint.h',
  'face/cheesefacesprite.h',
  'face/cheesemultifacesprite.h'
]

face_sources = face_meta_sources + face_sprite_sources
face_headers = face_meta_headers + face_sprite_headers

graphene_dep = dependency('graphene-1.0', version : ['>= 1.7.0'],
  required : true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version : ['>= 2.36.0'],
  required : true)
json_glib_dep = dependency('json-glib-1.0', version : ['>= 1.4.3'],
  required : true)

if gst_dep.found() and graphene_dep.found()

  install_headers('gstcheese.h', subdir : 'gstreamer-1.0/gst/cheese')
  install_headers(face_headers, subdir : 'gstreamer-1.0/gst/cheese/face')

  gstcheese = library('gstcheese-@0@'.format(api_version),
    face_sources + face_headers,
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

  if build_gir
    cheese_gen_sources = gnome.generate_gir(gstcheese,
      sources : face_sprite_sources + face_sprite_headers,
      namespace : 'Cheese',
      nsversion : api_version,
      identifier_prefix : 'Cheese',
      symbol_prefix : 'cheese',
      export_packages : 'gstreamer-cheese-1.0',
      includes : ['Gst-1.0', 'GdkPixbuf-2.0'],
      install : true,
      extra_args : gir_init_section,
      dependencies : [
        gst_dep,
        graphene_dep,
        gdk_pixbuf_dep,
        json_glib_dep
      ],
      extra_args: [ '--c-include=gstcheese.h', '--quiet' ]
    )
  endif

  cheese_vapi = gnome.generate_vapi(
    'gstcheese-@0@'.format(api_version),
    sources: cheese_gen_sources[0],
    packages: ['gstreamer-1.0', 'gdk-pixbuf-2.0'],
    install: true,
    install_dir: vapidir
  )

  gstcheese_dep = declare_dependency(link_with : gstcheese,
    include_directories : [libsinc],
    sources: [cheese_gen_sources],
    dependencies : [
      gst_dep,
      graphene_dep
    ]
  )


  pkgg = import('pkgconfig')
  pkgg.generate(
      libraries: [gstcheese],
        subdirs: 'gstreamer-1.0/gst/cheese',
        version: meson.project_version(),
           name: 'GstCheese',
       filebase: 'gstcheese-@0@'.format(api_version),
    description: 'Utils for gst-plugins-cheese',
       requires: ['gstreamer-1.0', 'gdk-pixbuf-2.0']
  )

endif
