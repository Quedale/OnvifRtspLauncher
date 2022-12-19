AC_INIT([onvifrtspserver], [0.1], [maintainer@example.com])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AM_SILENT_RULES([yes])


# Example of feature input for customized build
# E.g. nvcodec that fails as primary encoder on my laptop
AC_ARG_ENABLE(rpi,
    [AS_HELP_STRING([--enable-rpi],[Enable omx and rpicamsrc. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetrpi=yes ;;
        no)  targetrpi=no ;;
        true) targetrpi=yes ;;
        false)  targetrpi=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
        esac],
    [targetrpi="no"])
AM_CONDITIONAL([RPICOND], [test x$targetrpi = xyes])
AM_COND_IF([RPICOND],[
    AC_MSG_NOTICE(Enabled OMX and RPI Camera support)
    AC_SUBST(PKG_OMX_PATH, '$(abs_srcdir)/subprojects/gstreamer/build_omx/dist/lib/gstreamer-1.0/pkgconfig')
    AC_SUBST(EXT_PLGS, gstrpicamsrc)
    AC_SUBST(EXT_STC_LIBS, -lgstomx)
    AC_SUBST(EXT_DYN_LIBS, `PKG_CONFIG_PATH=/opt/vc/lib/pkgconfig pkg-config --libs mmal`)
    AC_SUBST(EXT_CFLAGS, -DENABLERPI)
])

AC_ARG_ENABLE(libav,
    [AS_HELP_STRING([--enable-rpi],[Enable libav gstreamer plugin. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetlibav=yes ;;
        no)  targetlibav=no ;;
        true) targetlibav=yes ;;
        false)  targetlibav=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
        esac],
    [targetlibav="no"])
AM_CONDITIONAL([LIBAVCOND], [test x$targetlibav = xyes])
AM_COND_IF([LIBAVCOND],[
    AC_SUBST(EXT_CFLAGS, -DENABLELIBAV)
])

AC_SUBST(PKG_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/pkgconfig')
AC_SUBST(PKG_GST_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig')

AC_SUBST(PKG_V4L2_PATH, '$(abs_srcdir)/subprojects/v4l-utils-1.22.1/dist/lib/pkgconfig')
AC_SUBST(PKG_FULL_PATH, '$(PKG_PATH):$(PKG_GST_PATH):$(PKG_OMX_PATH)')


AC_CONFIG_FILES([
    Makefile
])


LT_INIT([shared])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT