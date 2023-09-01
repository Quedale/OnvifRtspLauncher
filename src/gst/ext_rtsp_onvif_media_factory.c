#include "ext_rtsp_onvif_media_factory.h"
#include "../v4l/v4l2-device.h"

GST_DEBUG_CATEGORY_STATIC (ext_onvif_server_factory_debug);
#define GST_CAT_DEFAULT (ext_onvif_server_factory_debug)


struct ExtRTSPOnvifMediaFactoryPrivate
{
    GstRTSPOnvifMediaFactory parent;
    GMutex lock;

    //Video capture parameters
    v4l2ParameterResults * v4l2params;

    //Video output parameters
    gchar * video_device;
    gint width;
    gint height;
    gint fps;
    gchar * video_encoder;

    //Audio capture parameters
    gchar * microphone_device;
    gchar * microphone_element;

    ExtRTSPOnvifMediaCodec codec;
};

G_DEFINE_TYPE_WITH_PRIVATE (ExtRTSPOnvifMediaFactory, ext_rtsp_onvif_media_factory, GST_TYPE_RTSP_ONVIF_MEDIA_FACTORY);

int is_suported_output_format(SupportedPixelFormats format){
    switch(format){
        case V4L2_FMT_HEVC:
        case V4L2_FMT_H264:
        case V4L2_FMT_MJPEG:
            return TRUE;
        default: return FALSE;
    }
}

int is_suported_capture_format(int format){
    switch(format){
        case V4L2_FMT_H264:
        case V4L2_FMT_HEVC:
        case V4L2_FMT_YUYV:
        case V4L2_FMT_MJPEG:
            return TRUE;
        default:
            GST_ERROR("Unsupported pixel format : %d",format);
            return FALSE;
    }
}

GstElement * priv_add_link (char * element, char * name, GstElement * link_to, GstElement * bin){
    GstElement * elem = gst_element_factory_make (element, name);

    //Validate elements
    if (!elem) {
        GST_ERROR ("Element %s [%s] wasn't created... Exiting\n", element, name);
        return NULL;
    }
    
    //Adding elements to bin
    gst_bin_add_many (GST_BIN (bin), elem, NULL);

    //Linking elements
    if (link_to && !gst_element_link_many (link_to, elem, NULL)){
        GST_ERROR ("Linking %s [%s] part (A)-2 Fail...", element, name);
        return NULL;
    }
    
    return elem;
}

GstElement * priv_add_caps (GstCaps * caps, GstElement * link_to, GstElement * bin){
    link_to = priv_add_link("capsfilter", NULL,link_to,bin);
    if(!link_to) return NULL;

    g_object_set(G_OBJECT(link_to), "caps", caps,NULL);
    return link_to;
}

void 
ext_rtsp_onvif_media_factory_set_v4l2_params(ExtRTSPOnvifMediaFactory * factory, v4l2ParameterResults * params){
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    
    if(!is_suported_capture_format(params->device_pixelformat)){
        GST_ERROR("Unsupported pixel format : %d",params->device_pixelformat);
        g_mutex_lock (&factory->priv->lock);
        g_free (factory->priv->v4l2params);
        factory->priv->v4l2params = NULL;
        g_mutex_unlock (&factory->priv->lock);
        return;
    }

    if(params != NULL){
        GST_LOG("width : '%d'",params->device_width);
        GST_LOG("height : '%d'",params->device_height);
        GST_LOG("fps : '%d'",params->device_denominator/params->device_numerator);
    } else {
        GST_LOG("(NULL)");
    }
    g_mutex_lock (&factory->priv->lock);
    g_free (factory->priv->v4l2params);
    factory->priv->v4l2params = params;
    g_mutex_unlock (&factory->priv->lock);
}

void
ext_rtsp_onvif_media_factory_set_video_encoder (ExtRTSPOnvifMediaFactory *
    factory, const gchar * video_encoder)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%s'",video_encoder);

    g_mutex_lock (&factory->priv->lock);
    g_free (factory->priv->video_encoder);
    factory->priv->video_encoder = g_strdup (video_encoder);
    g_mutex_unlock (&factory->priv->lock);
}
void
ext_rtsp_onvif_media_factory_set_video_device (ExtRTSPOnvifMediaFactory *
    factory, const gchar * dev)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%s'",dev);

    g_mutex_lock (&factory->priv->lock);
    g_free (factory->priv->video_device);
    factory->priv->video_device = g_strdup (dev);
    g_mutex_unlock (&factory->priv->lock);
}

void
ext_rtsp_onvif_media_factory_set_microphone_device (ExtRTSPOnvifMediaFactory *
    factory, const gchar * dev)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%s'",dev);

    g_mutex_lock (&factory->priv->lock);
    g_free (factory->priv->microphone_device);
    factory->priv->microphone_device = g_strdup (dev);
    g_mutex_unlock (&factory->priv->lock);
}

void
ext_rtsp_onvif_media_factory_set_codec (ExtRTSPOnvifMediaFactory *
    factory, ExtRTSPOnvifMediaCodec codec)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",codec);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->codec = codec;
    g_mutex_unlock (&factory->priv->lock);
}

int
ext_rtsp_onvif_media_factory_get_codec (ExtRTSPOnvifMediaFactory *
    factory)
{
    g_return_val_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory),-1);
    int ret;
    g_mutex_lock (&factory->priv->lock);
    ret = factory->priv->codec;
    g_mutex_unlock (&factory->priv->lock);
    return ret;
}

void 
ext_rtsp_onvif_media_factory_set_microphone_element (ExtRTSPOnvifMediaFactory * factory, const gchar * element){
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%s'",element);

    g_mutex_lock (&factory->priv->lock);
    g_free (factory->priv->microphone_element);
    factory->priv->microphone_element = g_strdup (element);
    g_mutex_unlock (&factory->priv->lock);
}

void
ext_rtsp_onvif_media_factory_set_width (ExtRTSPOnvifMediaFactory *
    factory, const gint width)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",width);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->width = width;
    g_mutex_unlock (&factory->priv->lock);
}

void
ext_rtsp_onvif_media_factory_set_height (ExtRTSPOnvifMediaFactory *
    factory, const gint height)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",height);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->height = height;
    g_mutex_unlock (&factory->priv->lock);
}

void 
ext_rtsp_onvif_media_factory_set_fps (ExtRTSPOnvifMediaFactory * factory, const gint fps){
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",fps);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->fps = fps;
    g_mutex_unlock (&factory->priv->lock);
}

static void
ext_rtsp_onvif_media_factory_init (ExtRTSPOnvifMediaFactory * factory)
{
    GST_LOG("factory_init");
    factory->priv = ext_rtsp_onvif_media_factory_get_instance_private (factory);

    //Defaults
    ext_rtsp_onvif_media_factory_set_video_device(factory,"test");
    ext_rtsp_onvif_media_factory_set_microphone_device(factory,NULL);
    ext_rtsp_onvif_media_factory_set_microphone_element(factory,"autoaudiosrc");
    ext_rtsp_onvif_media_factory_set_width(factory,640);
    ext_rtsp_onvif_media_factory_set_height(factory,480);
    ext_rtsp_onvif_media_factory_set_fps(factory,10);
    ext_rtsp_onvif_media_factory_set_codec(factory,EXT_RTSP_CODEC_H264);
    g_mutex_init (&factory->priv->lock);
}

/*
    Workaround element to support v4l2h264enc hardware encoding
    https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/1056
*/
static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_capssetter_element (ExtRTSPOnvifMediaFactory * factory, 
        GstElement * ret, GstElement * last_element,
        unsigned int dev_denominator,
        unsigned int dev_numerator,
        unsigned int dev_width,
        unsigned int dev_height){

    if(strncmp("v4l2", factory->priv->video_encoder,4)){
        return last_element;
    }

    GST_DEBUG("Creating capssetter with fps[%d/%d] resolution[%d/%d]",dev_denominator,dev_numerator,dev_width,dev_height);
    last_element = priv_add_link("capssetter", NULL,last_element,ret);
    if(!last_element) return NULL;

    GstCaps *setter_filtercaps = gst_caps_new_simple ("video/x-raw",
        "framerate", GST_TYPE_FRACTION, dev_denominator, dev_numerator,
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, dev_width,
        "height", G_TYPE_INT, dev_height,
        "colorimetry", G_TYPE_STRING, "bt709", //TODO Find compatible colorimetry before fallback to default
        "interlace-mode", G_TYPE_STRING, "progressive",
        NULL);
    g_object_set(G_OBJECT(last_element), "caps", setter_filtercaps,NULL);
    g_object_set(G_OBJECT(last_element), "replace", TRUE,NULL);
    
    return last_element;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_video_encoder_elements (ExtRTSPOnvifMediaFactory * factory, 
        GstElement * ret, 
        GstElement * last_element){

    if(factory->priv->video_encoder){
        last_element = priv_add_link("videoconvert", NULL,last_element,ret);
        if(!last_element) return NULL;

        last_element = priv_ext_rtsp_onvif_media_factory_add_capssetter_element (factory, ret, last_element,factory->priv->fps,1,factory->priv->width,factory->priv->height); 
        if(last_element == NULL) return NULL;

        last_element = priv_add_link(factory->priv->video_encoder, "venc",last_element,ret);
        if(!last_element) return NULL;

        //v4l2h264enc specific
        if(!strncmp(factory->priv->video_encoder,"v4l2",4)){
            //245760
            //25000000 - 5.1
            //The bitrate has to be supported by the encoder profile/level
            GstStructure * structure = gst_structure_new_from_string ("controls,video_bitrate=2500000,video_bitrate_mode=1,repeat_sequence_header=1,video_gop_size=1");
            g_object_set(G_OBJECT(last_element), "extra-controls", structure,NULL);
        } else if(!strcmp(factory->priv->video_encoder,"openh264enc")){
            g_object_set(G_OBJECT(last_element), "gop-size", 1,NULL);
        } else if(!strcmp(factory->priv->video_encoder,"x264enc")){
            g_object_set(G_OBJECT(last_element), "speed-preset", 1,NULL);
            g_object_set(G_OBJECT(last_element), "b-adapt", FALSE,NULL);
            g_object_set(G_OBJECT(last_element), "tune", 4,NULL);
        }

        // Setting caps on source and encoder
        // TODO Set profile and level based on probed compatible settings
        GstCaps *venc_filtercaps = NULL;
        switch(factory->priv->codec){
            case EXT_RTSP_CODEC_H265:
                venc_filtercaps = gst_caps_from_string("video/x-h265");
                break;
            case EXT_RTSP_CODEC_H264:
                venc_filtercaps = gst_caps_new_simple ("video/x-h264",
                    // "profile", G_TYPE_STRING, "high",
                    "level", G_TYPE_STRING, "4",
                    NULL);
                break;
            case EXT_RTSP_CODEC_MJPEG:
                venc_filtercaps = gst_caps_new_simple ("image/jpeg",
                    "width", G_TYPE_INT, 640,
                    "height", G_TYPE_INT, 480,
                    NULL);
            default:
                break;
        }

        last_element = priv_add_caps(venc_filtercaps,last_element,ret);
        if(!last_element) return NULL;
        
        //Set as last element for additional linking
        return last_element;
    } else {
        GST_ERROR("Video encoder required, but none defined.");
        return NULL;
    }
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_videoscale_element (ExtRTSPOnvifMediaFactory * factory, GstElement * ret, GstElement * last_element, unsigned int dev_width, unsigned int dev_height){
    if(dev_width != factory->priv->width || dev_height != factory->priv->height){
        //Scale down required
        //TODO instead crop from center
        GST_WARNING("Scale down from '%i/%i' to '%i/%i'",dev_width,dev_height,factory->priv->width,factory->priv->height);
        last_element = priv_add_link("videoscale","vscale",last_element, ret);
    }
    return last_element;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_videorate_element (ExtRTSPOnvifMediaFactory * factory, GstElement * ret, GstElement * last_element, v4l2ParameterResults * input){
    if(input != NULL && factory->priv->fps != (gint)1.0*(input->device_denominator/input->device_numerator)){
        GST_WARNING("Droping framerate from '%i' to '%i'",(int)1.0*(input->device_denominator/input->device_numerator), factory->priv->fps);
        last_element = priv_add_link("videorate","vrate", last_element, ret);
        g_object_set(G_OBJECT(last_element), "max-rate", factory->priv->fps,NULL);
        g_object_set(G_OBJECT(last_element), "drop-only", TRUE, NULL);
    }
    return last_element;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_source_elements (ExtRTSPOnvifMediaFactory * factory, GstElement * ret, GstElement * last_element, unsigned int dev_pixelformat, unsigned int dev_denominator, unsigned int dev_numerator, unsigned int dev_width, unsigned int dev_height){
    //TODO handle alternative sources like picamsrc
    if(!strcmp(factory->priv->video_device,"test")){
        last_element = priv_add_link("videotestsrc","vidsrc",NULL,ret);
        if(!last_element) return NULL;
        g_object_set(G_OBJECT(last_element), "is-live", TRUE,NULL);
    } else {
        last_element = priv_add_link("v4l2src","vidsrc",NULL,ret);
        if(!last_element) return NULL;
        g_object_set(G_OBJECT(last_element), "device", factory->priv->video_device,NULL);
    }
    
    if(dev_pixelformat == V4L2_FMT_H264){
        GstStructure * structure = gst_structure_new_from_string ("controls,video_bitrate=25000000");
        g_object_set(G_OBJECT(last_element), "extra-controls", structure,NULL);
    }

    GST_DEBUG("Setting Capture FPS [%d/%d]",dev_denominator,dev_numerator);
    //Setting Capture Caps filters
    GstCaps *src_filtercaps;
    char fourcc[5] = {0};
    switch(dev_pixelformat){
        case V4L2_FMT_H264:
            GST_DEBUG("Setting H264 Capture Caps");
            src_filtercaps = gst_caps_new_simple("video/x-h264",
                "profile", G_TYPE_STRING, "high",
                "level", G_TYPE_STRING, "4",
                NULL);
            break;
        case V4L2_FMT_YUYV:
            GST_DEBUG("Setting Raw YUYV Capture Caps");
            src_filtercaps = gst_caps_new_simple ("video/x-raw",
                "format", G_TYPE_STRING, "YUY2",
                NULL);
            break;
        case V4L2_FMT_MJPEG: 
            GST_DEBUG("Setting MJPEG Capture Caps");
            src_filtercaps = gst_caps_new_simple ("image/jpeg",
                "mpegversion", G_TYPE_INT, 4, //Can be 2...
                "profile", G_TYPE_STRING, "high",
                "level", G_TYPE_STRING, "4",
                NULL);
            break;
        default:
            strncpy(fourcc, (char *)&dev_pixelformat, 4);
            GST_ERROR("Unsupported pixel format : [%s] %d",fourcc, dev_pixelformat);
            return NULL;

    }

    //Common caps properties
    gst_caps_set_simple(src_filtercaps,
        "width", G_TYPE_INT, dev_width,
        "height", G_TYPE_INT, dev_height,
        "framerate", GST_TYPE_FRACTION, dev_denominator, dev_numerator,
    NULL);
    last_element = priv_add_caps(src_filtercaps,last_element,ret);

    return last_element;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_video_elements (ExtRTSPOnvifMediaFactory * factory, GstElement * ret){
    //Create source
    GstElement * last_element= {0};

    v4l2ParameterResults * input = factory->priv->v4l2params;
    int dev_width;
    int dev_height;
    int dev_denominator;
    int dev_numerator;
    int pixel_format;
    if(input == NULL){
        dev_width = factory->priv->width;
        dev_height = factory->priv->height;
        dev_denominator = factory->priv->fps;
        dev_numerator = 1;
        pixel_format = V4L2_PIX_FMT_YUYV;
    } else {
        dev_width = input->device_width;
        dev_height = input->device_height;
        dev_denominator = input->device_denominator;
        dev_numerator = input->device_numerator;
        pixel_format = input->device_pixelformat;
    }

    //Create Capture elements
    last_element = priv_ext_rtsp_onvif_media_factory_add_source_elements(factory,ret, last_element, pixel_format,dev_denominator,dev_numerator,dev_width,dev_height);
    if(last_element == NULL) return NULL;

    //Adjust device framerate to desired stream framerate
    last_element = priv_ext_rtsp_onvif_media_factory_add_videorate_element(factory,ret,last_element, input);
    if(last_element == NULL) return NULL;

    //Adjust device resolution to desired stream resolution
    last_element = priv_ext_rtsp_onvif_media_factory_add_videoscale_element(factory,ret,last_element, dev_width, dev_height);
    if(last_element == NULL) return NULL;

    //Good measure video queue
    last_element = priv_add_link("queue","vqueue",last_element,ret);
    if(!last_element) return NULL;

    //Check if native feed requires encoding
    if(!is_suported_output_format(pixel_format)){
        GST_WARNING("Using video encoder!!!");
        last_element = priv_ext_rtsp_onvif_media_factory_add_video_encoder_elements(factory,ret, last_element);
        if(last_element == NULL) return NULL;
    } else {
        GST_WARNING("Using native video stream!!!");
    }

    //Create rtp payload sink
    char * parser = NULL;
    char * payloader = NULL;
    switch(factory->priv->codec){
        case EXT_RTSP_CODEC_H265:
            parser = "h265parse";
            payloader = "rtph265pay";
            break;
        case EXT_RTSP_CODEC_MJPEG:
            parser = "jpegparse";
            payloader = "rtpjpegpay";
            break;
        case EXT_RTSP_CODEC_H264:
        default:
            parser = "h264parse";
            payloader = "rtph264pay";
            break;
    }

    last_element = priv_add_link(parser, parser,last_element,ret);
    if(!last_element) return NULL;
    last_element = priv_add_link(payloader,"pay0",last_element,ret);
    if(!last_element) return NULL;

    return ret;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_audio_elements (ExtRTSPOnvifMediaFactory * factory, GstElement * ret){
    GstElement * last_element;
    if(!strcmp(factory->priv->microphone_device,"test")){
        last_element = priv_add_link("audiotestsrc","audio_src",NULL,ret);
        if(!last_element) return NULL;
    } else {
        if(strlen(factory->priv->microphone_element) < 1){
            GST_WARNING("No microphone source detected.");
            return ret;
        }
        last_element = priv_add_link(factory->priv->microphone_element,"audio_src",NULL,ret);
        if(!last_element) return NULL;

        if(factory->priv->microphone_device 
            && !strcmp(factory->priv->microphone_element,"alsasrc") 
            && strlen(factory->priv->microphone_device) > 1){
            GST_DEBUG("Setting alsa mic to '%s'",factory->priv->microphone_device);
            g_object_set(G_OBJECT(last_element), "device", factory->priv->microphone_device,NULL);
        }

        last_element = priv_add_link("audioresample","audio_resampler",last_element,ret);
        if(!last_element) return NULL;

        last_element = priv_add_link("audioconvert","audioconvert",last_element,ret);
        if(!last_element) return NULL;
    }

    last_element = priv_add_link("queue","aqueue",last_element,ret);
    if(!last_element) return NULL;

    //TODO support ONVIF input config for audio format/channels/rate
    GstCaps * mic_out_filtercaps = gst_caps_new_simple ("audio/x-mulaw",
                "channels", G_TYPE_INT, 1,
                "rate", G_TYPE_INT, 8000,
                NULL);
    last_element = priv_add_link("mulawenc","audio_enc",last_element,ret);
    if(!last_element) return NULL;

    last_element = priv_add_caps(mic_out_filtercaps,last_element,ret);
    if(!last_element) return NULL;

    last_element = priv_add_link("rtppcmupay","pay1",last_element,ret);
    if(!last_element) return NULL;

    return ret;
}

//WIP doesnt work
static GstElement *
priv_ext_rtsp_onvif_media_factory_create_element (ExtRTSPOnvifMediaFactory * factory,
    const GstRTSPUrl * url)
{

    //TODO Increase match scope support
    v4l2ParameterInput desires;
    desires.desired_fps = factory->priv->fps;
    desires.desired_width = factory->priv->width;
    desires.desired_height = factory->priv->height;
    switch(factory->priv->codec){
        case EXT_RTSP_CODEC_H265:
            desires.desired_pixelformat = V4L2_PIX_FMT_HEVC;
            break;
        case EXT_RTSP_CODEC_MJPEG:
            desires.desired_pixelformat = V4L2_PIX_FMT_MJPEG;
            break;
        case EXT_RTSP_CODEC_H264:
            desires.desired_pixelformat = V4L2_PIX_FMT_H264;
            break;
        default:
            break;
    }

    //Force configure v4l2 if it wasn't done already
    if(strcmp(factory->priv->video_device,"test") && factory->priv->v4l2params == NULL){
        v4l2ParameterResults * ret_val = configure_v4l2_device(factory->priv->video_device, desires, BAD_MATCH);
        if(ret_val == NULL){
            g_printerr ("Unable to configure v4l2 source device...\n");
            return NULL;
        }
        ext_rtsp_onvif_media_factory_set_v4l2_params(factory,ret_val);
    }
    GST_DEBUG("encoder : %s",factory->priv->video_encoder);
    GST_DEBUG("video_device : %s",factory->priv->video_device);
    GST_DEBUG("width : %i",factory->priv->width);
    GST_DEBUG("height : %i",factory->priv->height);
    GST_DEBUG("fps : %i",factory->priv->fps);
    GST_DEBUG("microphone_device : %s",factory->priv->microphone_device);
    GstElement *ret = gst_bin_new (NULL);

    if(!ret){
        g_printerr ("One of the root bins elements wasn't created... Exiting\n");
        return NULL;
    }

    if(!priv_ext_rtsp_onvif_media_factory_add_video_elements(factory,ret)){
        g_printerr ("Failed to add video elements to bin.. Exiting\n");
        gst_object_unref (ret);
        return NULL;
    }

    if(!priv_ext_rtsp_onvif_media_factory_add_audio_elements(factory,ret)){
        g_printerr ("Failed to add audio elements to bin.. Exiting\n");
        gst_object_unref (ret);
        return NULL;
    }

    return ret;

}

static GstElement *
ext_rtsp_onvif_media_factory_create_element (GstRTSPMediaFactory * factory, const GstRTSPUrl * url)
{
    GST_INFO("Creating GstBin");
    GstElement *element;
    GError *error = NULL;
    gchar *launch;
    GstRTSPContext *ctx = gst_rtsp_context_get_current ();

  /* Behavior taken out GstRtspOnvifMediaFactory
   * Except we handle v4l2src via an appsink/appsrc proxy
   * This is to support v4l2h264enc single instance use
   */

    launch = gst_rtsp_media_factory_get_launch (factory);

  /* we need a parse syntax */
    if (launch == NULL)
        element = priv_ext_rtsp_onvif_media_factory_create_element(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),url);
    else
        element = gst_parse_launch_full (launch, NULL, GST_PARSE_FLAG_PLACE_IN_BIN, &error);

    if (element == NULL)
        goto parse_error;

    g_free (launch);

    if (error != NULL) {
        /* a recoverable error was encountered */
        GST_ERROR ("recoverable parsing error: %s", error->message);
        g_error_free (error);
    }

  /* add backchannel pipeline part, if requested */
    if (gst_rtsp_onvif_media_factory_requires_backchannel (factory, ctx)) {
        GstRTSPOnvifMediaFactory *onvif_factory = GST_RTSP_ONVIF_MEDIA_FACTORY (factory);
        GstElement *backchannel_bin;
        GstElement *backchannel_depay;
        GstPad *depay_pad, *depay_ghostpad;

        launch = gst_rtsp_onvif_media_factory_get_backchannel_launch (onvif_factory);
        if (launch == NULL)
            goto no_launch_backchannel;

        backchannel_bin = gst_parse_bin_from_description_full (launch, FALSE, NULL,
            GST_PARSE_FLAG_PLACE_IN_BIN, &error);
        if (backchannel_bin == NULL)
            goto parse_error_backchannel;

        g_free (launch);

        if (error != NULL) {
            /* a recoverable error was encountered */
            GST_ERROR ("recoverable parsing error: %s", error->message);
            g_error_free (error);
        }

        gst_object_set_name (GST_OBJECT (backchannel_bin), "onvif-backchannel");

        backchannel_depay = gst_bin_get_by_name (GST_BIN (backchannel_bin), "depay_backchannel");
        if (!backchannel_depay) {
            gst_object_unref (backchannel_bin);
            goto wrongly_formatted_backchannel_bin;
        }

        depay_pad = gst_element_get_static_pad (backchannel_depay, "sink");
        if (!depay_pad) {
            gst_object_unref (backchannel_depay);
            gst_object_unref (backchannel_bin);
            goto wrongly_formatted_backchannel_bin;
        }

        depay_ghostpad = gst_ghost_pad_new ("sink", depay_pad);
        gst_element_add_pad (backchannel_bin, depay_ghostpad);

        gst_bin_add (GST_BIN (element), backchannel_bin);
    }

    return element;

parse_error:
    {
        GST_ERROR ("could not parse launch syntax (%s): %s", launch,
            (error ? error->message : "unknown reason"));
        if (error)
            g_error_free (error);
        g_free (launch);
        return NULL;
    }
no_launch_backchannel:
    {
        GST_ERROR ("no backchannel launch line specified");
        gst_object_unref (element);
        return NULL;
    }
parse_error_backchannel:
    {
        GST_ERROR ("could not parse backchannel launch syntax (%s): %s", launch,
            (error ? error->message : "unknown reason"));
        if (error)
            g_error_free (error);
        g_free (launch);
        gst_object_unref (element);
        return NULL;
    }

wrongly_formatted_backchannel_bin:
    {
        GST_ERROR ("invalidly formatted backchannel bin");

        gst_object_unref (element);
        return NULL;
    }
}


static void
ext_rtsp_onvif_media_factory_class_init (ExtRTSPOnvifMediaFactoryClass * klass)
{
    GST_LOG("factory_class_init");
    GstRTSPMediaFactoryClass *mf_class = GST_RTSP_MEDIA_FACTORY_CLASS (klass);
    mf_class->create_element = ext_rtsp_onvif_media_factory_create_element;
}

GstRTSPMediaFactory *
ext_rtsp_onvif_media_factory_new (void)
{
    GST_DEBUG_CATEGORY_INIT (ext_onvif_server_factory_debug, "ext-onvif-factory", 0, "Extended ONVIF Factory");
    gst_debug_set_threshold_for_name ("ext-onvif-factory", GST_LEVEL_LOG);
    GST_INFO ("Creating new ExtRTSPOnvifMediaFactory");
    GstRTSPMediaFactory *result;

    result = g_object_new (ext_rtsp_onvif_media_factory_get_type (), NULL);

    return result;
}