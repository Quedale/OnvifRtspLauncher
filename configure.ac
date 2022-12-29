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
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-rpi]) ;;
        esac],
    [targetrpi="no"])
AM_CONDITIONAL([RPICOND], [test x$targetrpi = xyes])
AM_COND_IF([RPICOND],[
    AC_MSG_NOTICE(Enabled OMX and RPI Camera support)
    AC_SUBST(PKG_OMX_PATH, '$(abs_srcdir)/subprojects/gstreamer/build_omx/dist/lib/gstreamer-1.0/pkgconfig')
    AC_SUBST(OMX_PKG, gstrpicamsrc)
    AC_SUBST(OMX_LIBS, -lgstomx)
    AC_SUBST(OMX_DYN_LIBS, `PKG_CONFIG_PATH=/opt/vc/lib/pkgconfig pkg-config --libs mmal`)
    AC_SUBST(OMX_CFLAGS, -DENABLERPI)
],[
    AC_MSG_NOTICE(Disabled OMX and RPI Camera support)
    AC_SUBST(PKG_OMX_PATH)
    AC_SUBST(OMX_PKG)
    AC_SUBST(OMX_LIBS)
    AC_SUBST(OMX_DYN_LIBS)
    AC_SUBST(OMX_CFLAGS)
])

AC_ARG_ENABLE(libav,
    [AS_HELP_STRING([--enable-libav],[Enable libav gstreamer plugin. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetlibav=yes ;;
        no)  targetlibav=no ;;
        true) targetlibav=yes ;;
        false)  targetlibav=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-libav]) ;;
        esac],
    [targetlibav="no"])
AM_CONDITIONAL([LIBAVCOND], [test x$targetlibav = xyes])
AM_COND_IF([LIBAVCOND],[
    AC_MSG_NOTICE(Enabled Libav FFmpeg support.)
    AC_SUBST(LIBAV_LIB, gstlibav)
    AC_SUBST(LIBAV_CFLAGS, -DENABLELIBAV)
],[
    AC_MSG_NOTICE(Disabled Libav FFmpeg support.)
    AC_SUBST(LIBAV_LIB)
    AC_SUBST(LIBAV_CFLAGS)
])

AC_ARG_ENABLE(client,
    [AS_HELP_STRING([--enable-client],[Enable ONVIF client tool. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetclient=yes ;;
        no)  targetclient=no ;;
        true) targetclient=yes ;;
        false)  targetclient=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-client]) ;;
        esac],
    [targetclient="no"])
AM_CONDITIONAL([CLIENTCOND], [test x$targetclient = xyes])
AM_COND_IF([CLIENTCOND],[
    AC_MSG_NOTICE(Enabled Client Tool.)
    AC_SUBST(CLIENT_PROG, onvifclient)
],[
    AC_MSG_NOTICE(Disabled Client Tool.)
    AC_SUBST(CLIENT_PROG)
])


AC_SUBST(PKG_PULSE_PATH, '$(abs_srcdir)/subprojects/pulseaudio/build/dist/lib/pkgconfig')
AC_SUBST(PKG_ALSA_PATH, '$(abs_srcdir)/subprojects/alsa-lib/build/dist/lib/pkgconfig')
AC_SUBST(PKG_GUDEV_PATH, '$(abs_srcdir)/subprojects/libgudev/build/dist/lib/pkgconfig')
AC_SUBST(PKG_FFMPEG_PATH, '$(abs_srcdir)/subprojects/FFmpeg/dist/lib/pkgconfig')

AC_SUBST(PKG_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/pkgconfig')
AC_SUBST(PKG_GST_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig')
AC_SUBST(PKG_GSTLIBAV_PATH, '$(abs_srcdir)/subprojects/gstreamer/libav_build/dist/lib/gstreamer-1.0/pkgconfig:$(abs_srcdir)/subprojects/gstreamer/libav_build/dist/lib/pkgconfig')
AC_SUBST(PKG_V4L2_PATH, '$(abs_srcdir)/subprojects/v4l-utils/dist/lib/pkgconfig')
AC_SUBST(PKG_FULL_PATH, '$(PKG_PATH):$(PKG_GST_PATH):$(PKG_OMX_PATH):$(PKG_FFMPEG_PATH):$(PKG_GSTLIBAV_PATH):$(PKG_PULSE_PATH):$(PKG_ALSA_PATH):$(PKG_GUDEV_PATH)')


AC_CONFIG_FILES([
    Makefile
])


LT_INIT([shared])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT