ACLOCAL_AMFLAGS = -I m4
MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))
TAB  := $(NULL)<tab>$(NULL)

# Extract list of compiled package
GST_PLGS = $(shell ls $(PKG_GST_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')
GST_LIBS = $(shell ls $(PKG_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = onvifserver onvifclient

onvifclient_SOURCES = $(top_srcdir)/src/onvif-client.c $(top_srcdir)/src/onvifinitstaticplugins.c
onvifclient_CFLAGS = -Wall `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS)
onvifclient_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` $(EXT_STC_LIBS) -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c $(top_srcdir)/src/onvifinitstaticplugins.c  $(top_srcdir)/src/sink-retriever.c $(top_srcdir)/src/vencoder-retriever.c $(top_srcdir)/src/gst/ext_rtsp_onvif_media_factory.c $(top_srcdir)/src/v4l/v4l2-device.c $(top_srcdir)/src/v4l/v4l2_match_result.c
onvifserver_CFLAGS = -Wall `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS) `PKG_CONFIG_PATH=$(PKG_V4L2_PATH) pkg-config --cflags libv4l2`
onvifserver_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` $(EXT_STC_LIBS) `PKG_CONFIG_PATH=$(PKG_V4L2_PATH) pkg-config --libs libv4l2` -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++

paths$(EXEEXT):
	@echo "PKG_FULL_PATH : $(PKG_FULL_PATH)"
	@echo ""
	@echo "GST_PLGS : $(GST_PLGS)"
	@echo ""
	@echo "GST_LIBS : $(GST_LIBS)"
	@echo ""
	@echo PKG_OMX_PATH : $(PKG_OMX_PATH)
