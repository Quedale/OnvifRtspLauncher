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
onvifclient_CFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS)
onvifclient_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` $(EXT_STC_LIBS) -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c $(top_srcdir)/src/onvifinitstaticplugins.c  $(top_srcdir)/src/sink-retriever.c
onvifserver_CFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS)
onvifserver_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` $(EXT_STC_LIBS) -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++

paths$(EXEEXT):
	@echo "PKG_FULL_PATH : $(PKG_FULL_PATH)"
	@echo ""
	@echo "GST_PLGS : $(GST_PLGS)"
	@echo ""
	@echo "GST_LIBS : $(GST_LIBS)"
	@echo ""
	@echo PKG_OMX_PATH : $(PKG_OMX_PATH)
