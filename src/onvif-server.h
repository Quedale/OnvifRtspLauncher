

#ifndef __CUSTOM_ONVIF_MEDIA_FACTORY_H__
#define __CUSTOM_ONVIF_MEDIA_FACTORY_H__

#include <gst/gst.h>

#include <gst/rtsp-server/rtsp-server.h>



typedef struct OnvifFactoryClass OnvifFactoryClass;
typedef struct OnvifFactory OnvifFactory;
typedef struct OnvifFactoryPrivate OnvifFactoryPrivate;

struct OnvifFactoryClass
{
  GstRTSPOnvifMediaFactoryClass parent;
//   gboolean (*has_backchannel_support2) (GstRTSPOnvifMediaFactory * factory);

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING_LARGE];
};

struct OnvifFactory
{
  GstRTSPOnvifMediaFactory parent;
  OnvifFactoryPrivate *priv;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType custom_onvif_factory_get_type (void);

#endif