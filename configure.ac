AC_INIT([onvifrtspserver], [0.1], [maintainer@example.com])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
# AM_SILENT_RULES([yes])

AC_SUBST(PKG_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/x86_64-linux-gnu/pkgconfig')
AC_SUBST(PKG_GST_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/x86_64-linux-gnu/gstreamer-1.0/pkgconfig')
AC_SUBST(PKG_FULL_PATH, '$(PKG_PATH):$(PKG_GST_PATH)')

AC_CONFIG_FILES([
    Makefile
])

# Example of feature input for customized build
# E.g. nvcodec that fails as primary encoder on my laptop
# AC_ARG_ENABLE(audiotestsrc-target,
#         [AS_HELP_STRING([--enable-audiotestsrc],[Enable Gstreamer audiotestsrc element. @<:@default=no@:>@.])],
#         [with_audiotestsrc="no";],[with_audiotestsrc="yes"])

LT_INIT([shared])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT