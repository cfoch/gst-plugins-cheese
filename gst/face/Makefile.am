# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstcheesefaceeffects.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

## Plugin 1 (cheese plugins example)

# sources used to compile this plug-in
libgstcheesefaceeffects_la_SOURCES = gstcheesefaceeffects.cpp gstcheesefacedetect.cpp

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstcheesefaceeffects_la_CXXFLAGS = $(GST_CFLAGS) $(OPENCV_CFLAGS) $(DLIB_CLFAGS)
libgstcheesefaceeffects_la_LIBADD = $(GST_LIBS) $(DLIB_LIBS) $(OPENCV_LIBS) -lgstopencv-1.0
libgstcheesefaceeffects_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstcheesefaceeffects_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstcheesefacedetect.h
