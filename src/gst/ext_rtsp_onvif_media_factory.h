

#ifndef __EXT_ONVIF_MEDIA_FACTORY_H__
#define __EXT_ONVIF_MEDIA_FACTORY_H__

#include <gst/gst.h>
#include "../v4l/v4l2-device.h"

// #include <gst/rtsp-server/rtsp-server.h>
#include <gst/rtsp-server/rtsp-onvif-media-factory.h>

#define EXT_TYPE_RTSP_ONVIF_MEDIA_FACTORY              (ext_rtsp_onvif_media_factory_get_type ())
#define IS_EXT_RTSP_ONVIF_MEDIA_FACTORY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXT_TYPE_RTSP_ONVIF_MEDIA_FACTORY))
#define IS_EXT_RTSP_ONVIF_MEDIA_FACTORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EXT_TYPE_RTSP_ONVIF_MEDIA_FACTORY))
#define EXT_RTSP_ONVIF_MEDIA_FACTORY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EXT_TYPE_RTSP_ONVIF_MEDIA_FACTORY, ExtRTSPOnvifMediaFactoryClass))
#define EXT_RTSP_ONVIF_MEDIA_FACTORY(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXT_TYPE_RTSP_ONVIF_MEDIA_FACTORY, ExtRTSPOnvifMediaFactory))
#define EXT_RTSP_ONVIF_MEDIA_FACTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EXT_TYPE_RTSP_ONVIF_MEDIA_FACTORY, ExtRTSPOnvifMediaFactoryClass))
#define EXT_RTSP_ONVIF_MEDIA_FACTORY_CAST(obj)         ((ExtRTSPOnvifMediaFactory*)(obj))
#define EXT_RTSP_ONVIF_MEDIA_FACTORY_CLASS_CAST(klass) ((ExtRTSPOnvifMediaFactoryClass*)(klass))

typedef struct ExtRTSPOnvifMediaFactoryClass ExtRTSPOnvifMediaFactoryClass;
typedef struct ExtRTSPOnvifMediaFactory ExtRTSPOnvifMediaFactory;
typedef struct ExtRTSPOnvifMediaFactoryPrivate ExtRTSPOnvifMediaFactoryPrivate;
typedef struct ExtRTSPOnvifSourceInput ExtRTSPOnvifSourceInput;

struct ExtRTSPOnvifMediaFactoryClass
{
  GstRTSPOnvifMediaFactoryClass parent;
//   gboolean (*has_backchannel_support2) (GstRTSPOnvifMediaFactory * factory);

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING_LARGE];
};

struct ExtRTSPOnvifMediaFactory
{
  GstRTSPOnvifMediaFactory parent;
  ExtRTSPOnvifMediaFactoryPrivate *priv;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType ext_rtsp_onvif_media_factory_get_type (void);

GstRTSPMediaFactory *
ext_rtsp_onvif_media_factory_new (void);

void 
ext_rtsp_onvif_media_factory_set_video_device (ExtRTSPOnvifMediaFactory * factory, const gchar * dev);

void 
ext_rtsp_onvif_media_factory_set_microphone_device (ExtRTSPOnvifMediaFactory * factory, const gchar * dev);

void 
ext_rtsp_onvif_media_factory_set_microphone_element (ExtRTSPOnvifMediaFactory * factory, const gchar * element);

void 
ext_rtsp_onvif_media_factory_set_width (ExtRTSPOnvifMediaFactory * factory, const gint width);

void 
ext_rtsp_onvif_media_factory_set_height (ExtRTSPOnvifMediaFactory * factory, const gint height);

void 
ext_rtsp_onvif_media_factory_set_fps (ExtRTSPOnvifMediaFactory * factory, const gint fps);

void
ext_rtsp_onvif_media_factory_set_video_encoder (ExtRTSPOnvifMediaFactory * factory, const gchar * video_encoder);

void 
ext_rtsp_onvif_media_factory_set_v4l2_params(ExtRTSPOnvifMediaFactory * factory, v4l2ParameterResults * params);

#endif