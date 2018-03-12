gdk_dep = dependency('gdk-3.0', version : ['>= 3.22'], required : true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version : ['>= 2.36.0'],
  required : true)

exe = executable('facespriteframe',
  'facespriteframe.c',
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacespriteframe.c'),
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep]
)
test('facespriteframe', exe)

exe = executable('facespritekeypoint',
  'facespritekeypoint.c',
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacespritekeypoint.c'),
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep]
)
test('facespritekeypoint', exe)


