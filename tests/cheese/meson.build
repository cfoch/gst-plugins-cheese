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
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacespriteframe.c'),
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep]
)
test('facespritekeypoint', exe)

exe = executable('facesprite',
  'facesprite.c',
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacesprite.c'),
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacespritekeypoint.c'),
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep]
)
test('facesprite', exe)

exe = executable('multifacesprite',
  'multifacesprite.c',
  join_paths(meson.source_root(), 'gst', 'face', 'cheesemultifacesprite.c'),
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacesprite.c'),
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacespriteframe.c'),
  join_paths(meson.source_root(), 'gst', 'face', 'cheesefacespritekeypoint.c'),
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep, json_glib_dep]
)
test('multifacesprite', exe)
