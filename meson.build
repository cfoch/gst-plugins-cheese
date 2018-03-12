project('gst-plugins-cheese', 'c', 'cpp',
        version : '1.13.0.1',
        meson_version : '>= 0.40.1')

gst_version = meson.project_version()
version_arr = gst_version.split('.')
gst_version_major = version_arr[0]
gst_version_minor = version_arr[1]
gst_version_micro = version_arr[2]
if version_arr.length() == 4
  gst_version_nano = version_arr[3]
else
  gst_version_nano = 0
endif

plugins_install_dir = '@0@/gstreamer-1.0'.format(get_option('libdir'))

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

cxx = meson.get_compiler('cpp')
cdata = configuration_data()
cdata.set('GST_LICENSE', '"LGPL"')
cdata.set('GST_PACKAGE_ORIGIN', '"c"')
cdata.set('GST_PACKAGE_NAME', '"Gstreamer Plugins Cheese"')
cdata.set('PACKAGE', '"gst-plugins-cheese"')
cdata.set('VERSION', '"@0@"'.format(gst_version))
cdata.set_quoted('GST_PLUGINS_CHEESE_DATADIR', datadir)
configure_file(output : 'config.h', configuration : cdata)

gst_plugins_bad_args = ['-DHAVE_CONFIG_H']
configinc = include_directories('.')
libsinc = include_directories('gst-libs')
hungarianinc = include_directories('hungarian-algorithm-cpp')

gst_req = '>= @0@.@1@.0'.format(gst_version_major, gst_version_minor)
api_version = '1.0'
soversion = 0
libversion = '@0@.@1@.0'.format(soversion, gst_version_minor.to_int() * 100 + gst_version_micro.to_int())

# GStreamer dependencies
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req,
    fallback : ['gst-plugins-base', 'video_dep'])
gst_controller_dep = dependency('gstreamer-controller-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_controller_dep'])
gstgl_dep = dependency('gstreamer-gl-1.0', version : gst_req,
    fallback : ['gst-plugins-base', 'gstgl_dep'], required: false)


gobject_dep = dependency('gobject-2.0')
glib_dep = dependency('glib-2.0')
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])

subdir('gst-libs')
subdir('gst')
subdir('tests')
