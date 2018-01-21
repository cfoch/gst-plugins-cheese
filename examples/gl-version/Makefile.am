# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
# plugin_LTLIBRARIES = detector.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

## Plugin 1 (cheese plugins example)

# sources used to compile this plug-in
bin_PROGRAMS = glversion

# compiler and linker flags used to compile this plugin, set in configure.ac
AM_CXXFLAGS = $(CPPFLAGS)
glversion_SOURCES = gl-version.cpp
glversion_LDADD = $(OPENGL_LIBS)
# detector_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
# detector_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
# noinst_HEADERS = gstcheesefacedetect.h
