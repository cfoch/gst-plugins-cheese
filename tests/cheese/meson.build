gdk_dep = dependency('gdk-3.0', version : ['>= 3.22'], required : true)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version : ['>= 2.36.0'],
  required : true)

exe = executable('facespriteframe',
  'facespriteframe.c',
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep, gstcheese_dep]
)
test('facespriteframe', exe)

exe = executable('facespritekeypoint',
  'facespritekeypoint.c',
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep, gstcheese_dep]
)
test('facespritekeypoint', exe)

exe = executable('facesprite',
  'facesprite.c',
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep, gstcheese_dep]
)
test('facesprite', exe)

exe = executable('multifacesprite',
  'multifacesprite.c',
  install : false,
  dependencies : [glib_dep, gobject_dep, gdk_dep, gdk_pixbuf_dep, json_glib_dep,
                  gstcheese_dep]
)
test('multifacesprite', exe)
