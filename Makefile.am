ACLOCAL_AMFLAGS = -I m4
MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))
TAB  := $(NULL)<tab>$(NULL)

# Extract list of compiled package
# GST_PLGS = $(shell ls $(PKG_GST_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')
# GST_LIBS = $(shell ls $(PKG_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = onvifserver $(CLIENT_PROG)
EXTRA_PROGRAMS = onvifclient

onvifclient_SOURCES = $(top_srcdir)/src/onvif-client.c $(top_srcdir)/src/onvifinitstaticplugins.c
onvifclient_CFLAGS = -Wall $(GST_STATIC_FLAG) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(LIBAV_CFLAGS) $(OMX_CFLAGS) -DENABLECLIENT
onvifclient_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(OMX_PKG) $(CLIENT_LIB) $(GST_PLGS) $(LIBAV_LIB)` $(OMX_LIBS) -Wl,-Bdynamic $(OMX_DYN_LIBS) -lstdc++

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c $(top_srcdir)/src/onvifinitstaticplugins.c $(top_srcdir)/src/sink-retriever.c $(top_srcdir)/src/vencoder-retriever.c $(top_srcdir)/src/gst/ext_rtsp_onvif_media_factory.c $(top_srcdir)/src/v4l/v4l2-device.c $(top_srcdir)/src/v4l/v4l2_match_result.c $(top_srcdir)/src/alsa/alsa_utils.c $(top_srcdir)/src/alsa/alsa_devices.c $(top_srcdir)/src/onvif-server-snapshot.c
onvifserver_CFLAGS = -Wall $(GST_STATIC_FLAG) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(OMX_PKG) $(GST_PLGS) $(LIBAV_LIB) $(LIBCAM_LIB) libv4l2` $(LIBAV_CFLAGS) $(LIBCAM_CFLAGS) $(OMX_CFLAGS)
onvifserver_LDFLAGS = $(GST_LINK_TYPE) $(LIBCAM_STC_LIB) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(OMX_PKG) $(GST_PLGS) $(LIBAV_LIB) $(LIBCAM_LIB) libv4l2` $(OMX_LIBS) -Wl,-Bdynamic $(OMX_DYN_LIBS) -lstdc++

paths$(EXEEXT):
	@echo "PKG_FULL_PATH : $(PKG_FULL_PATH)"
	@echo ""
	@echo "GST_PLGS : $(GST_PLGS)"
	@echo ""
	@echo "GST_LIBS : $(GST_LIBS)"
	@echo ""
	@echo PKG_OMX_PATH : $(PKG_OMX_PATH)
	@echo GST_STATIC_FLAG : $(GST_STATIC_FLAG)
	@echo GST_LINK_TYPE : $(GST_LINK_TYPE)
	@echo "libs : " `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gstreamer-1.0`
	@echo ""
	@echo "GST_PLGS : $(GST_PLGS)"
	@echo ""
	@echo "GST_LIBS : $(GST_LIBS)"