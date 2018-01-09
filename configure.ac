dnl required version of autoconf
AC_PREREQ([2.53])

dnl TODO: fill in your package name and package version here
AC_INIT([my-plugin-package],[1.0.0])

dnl required versions of gstreamer and plugins-base
GST_REQUIRED=1.0.0
GSTPB_REQUIRED=1.0.0

AC_CONFIG_SRCDIR([gst/])
AC_CONFIG_HEADERS([config.h])

dnl required version of automake
AM_INIT_AUTOMAKE([1.10])

dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl check for tools (compiler etc.)
AC_PROG_CC

dnl required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl give error and exit if we don't have pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, [ ], [
  AC_MSG_ERROR([You need to have pkg-config installed!])
])

dnl Check for the required version of GStreamer core (and gst-plugins-base)
dnl This will export GST_CFLAGS and GST_LIBS variables for use in Makefile.am
dnl
dnl If you need libraries from gst-plugins-base here, also add:
dnl for libgstaudio-1.0: gstreamer-audio-1.0 >= $GST_REQUIRED
dnl for libgstvideo-1.0: gstreamer-video-1.0 >= $GST_REQUIRED
dnl for libgsttag-1.0: gstreamer-tag-1.0 >= $GST_REQUIRED
dnl for libgstpbutils-1.0: gstreamer-pbutils-1.0 >= $GST_REQUIRED
dnl for libgstfft-1.0: gstreamer-fft-1.0 >= $GST_REQUIRED
dnl for libgstinterfaces-1.0: gstreamer-interfaces-1.0 >= $GST_REQUIRED
dnl for libgstrtp-1.0: gstreamer-rtp-1.0 >= $GST_REQUIRED
dnl for libgstrtsp-1.0: gstreamer-rtsp-1.0 >= $GST_REQUIRED
dnl etc.
PKG_CHECK_MODULES(GST, [
  gstreamer-1.0 >= $GST_REQUIRED
  gstreamer-base-1.0 >= $GST_REQUIRED
], [
  AC_SUBST(GST_CFLAGS)
  AC_SUBST(GST_LIBS)
], [
  AC_MSG_ERROR([
      You need to install or upgrade the GStreamer development
      packages on your system. On debian-based systems these are
      libgstreamer1.0-dev and libgstreamer-plugins-base1.0-dev.
      on RPM-based systems gstreamer1.0-devel, libgstreamer1.0-devel
      or similar. The minimum version required is $GST_REQUIRED.
  ])
])

HAVE_OPENCV = "no"
PKG_CHECK_MODULES(OPENCV, opencv >= 2.3.0 opencv <= 3.4.0 , [
  AC_PROG_CXX
  AC_LANG([C++])
  OLD_CPPFLAGS=$CPPFLAGS
  CPPFLAGS=$OPENCV_CFLAGS
  something_not_found=no

  AC_CHECK_HEADERS([opencv2/core/core_c.h \
                    opencv2/core/version.hpp \
                    opencv2/highgui/highgui_c.h \
                    opencv2/imgproc/imgproc.hpp \
                    opencv2/imgproc/imgproc_c.h \
                    opencv2/objdetect/objdetect.hpp \
                    opencv2/video/background_segm.hpp], [], [something_not_found=yes])


  dnl Only required for OpenCV >= 3, we have compat code for older versions
  PKG_CHECK_MODULES(OPENCV_3, opencv >= 3, [
    dnl check opencv_contrib headers (not always present in opencv distributions)
    AC_CHECK_HEADERS([opencv2/bgsegm.hpp], [], [something_not_found=yes])],
    [NOT_A_PROBLEM=yes])

  CPPFLAGS=$OLD_CPPFLAGS
  AC_LANG([C])

  if test $something_not_found = "yes"; then
    AC_MSG_RESULT([not all opencv2 headers were found])
    AC_MSG_ERROR([Here 2])
    HAVE_OPENCV="no"
  else
    HAVE_OPENCV="yes"
  fi
], [
  AC_MSG_ERROR([Here 3])
  HAVE_OPENCV="no"
])

if test $HAVE_OPENCV = "no"; then
  AC_MSG_ERROR([OpenCV not found])
fi

PKG_CHECK_MODULES(DLIB, [
  dlib-1 > 19.0.0
], [
  AC_PROG_CXX
  AC_LANG([C++])
  AX_APPEND_COMPILE_FLAGS([$DLIB_CFLAGS], [CPPFLAGS])
  something_not_found=no
  AC_CHECK_HEADERS([dlib/image_processing/frontal_face_detector.h], [], [something_not_found=yes])
], [
  AC_MSG_ERROR([
      dlib not found.
  ])
])

dnl check if compiler understands -Wall (if yes, add -Wall to GST_CFLAGS)
AC_MSG_CHECKING([to see if compiler understands -Wall])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([ ], [ ])], [
  GST_CFLAGS="$GST_CFLAGS -Wall"
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
])

CFLAGS = "-O2"
CXXFLAGS = "-O2"

dnl set the plugindir where plugins should be installed (for src/Makefile.am)
if test "x${prefix}" = "x$HOME"; then
  plugindir="$HOME/.gstreamer-1.0/plugins"
else
  plugindir="\$(libdir)/gstreamer-1.0"
fi
AC_SUBST(plugindir)


dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

AC_CONFIG_FILES([
Makefile
gst/Makefile
gst/face/Makefile
examples/Makefile
examples/detector/Makefile
])
AC_OUTPUT

