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
    gchar * audio_device;

};

G_DEFINE_TYPE_WITH_PRIVATE (ExtRTSPOnvifMediaFactory, ext_rtsp_onvif_media_factory, GST_TYPE_RTSP_ONVIF_MEDIA_FACTORY);

int is_suported_output_format(SupportedPixelFormats format){
    switch(format){
        case V4L2_FMT_H264: return TRUE;
        case V4L2_FMT_MJPEG: //TODO Support mjpeg
        default: return FALSE;
    }
}

int is_suported_capture_format(int format){
    switch(format){
        case V4L2_FMT_H264:
        case V4L2_FMT_YUYV:
            return TRUE;
        case V4L2_FMT_MJPEG: //TODO support mjpeg stream
        default:
            GST_ERROR("Unsupported pixel format : %d",format);
            return FALSE;
    }
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
ext_rtsp_onvif_media_factory_set_audio_device (ExtRTSPOnvifMediaFactory *
    factory, const gchar * dev)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%s'",dev);

    g_mutex_lock (&factory->priv->lock);
    g_free (factory->priv->audio_device);
    factory->priv->audio_device = g_strdup (dev);
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
    ext_rtsp_onvif_media_factory_set_audio_device(factory,"test");
    ext_rtsp_onvif_media_factory_set_width(factory,640);
    ext_rtsp_onvif_media_factory_set_height(factory,480);
    ext_rtsp_onvif_media_factory_set_fps(factory,10);

    g_mutex_init (&factory->priv->lock);
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_videoconvert_element (ExtRTSPOnvifMediaFactory * factory, 
        GstElement * ret, GstElement * last_element){
    GstElement * videoconvert = gst_element_factory_make ("videoconvert", NULL);
    if (!videoconvert) {
        g_printerr ("Videoconvert elements wasn't created... Exiting\n");
        return NULL;
    }

    //Adding videoconvert elements to bin
    gst_bin_add_many (GST_BIN (ret), videoconvert, NULL);

    //Linking videoconvert elements
    if (!gst_element_link_many (last_element, videoconvert, NULL)){
        GST_ERROR ("Linking videoconvert part (A)-2 Fail...");
        return NULL;
    }
    return videoconvert;
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

    if(strcmp(factory->priv->video_encoder,"v4l2h264enc")){
        return last_element;
    }

    GST_DEBUG("Creating capssetter with fps[%d/%d] resolution[%d/%d]",dev_denominator,dev_numerator,dev_width,dev_height);
    GstElement * capssetter = gst_element_factory_make ("capssetter", NULL);
    if (!capssetter) {
        g_printerr ("capssetter elements wasn't created... Exiting\n");
        return NULL;
    }

    GstCaps *setter_filtercaps = gst_caps_new_simple ("video/x-raw",
        "framerate", GST_TYPE_FRACTION, dev_denominator, dev_numerator,
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, dev_width,
        "height", G_TYPE_INT, dev_height,
        "colorimetry", G_TYPE_STRING, "bt709", //TODO Find compatible colorimetry before fallback to default
        "interlace-mode", G_TYPE_STRING, "progressive",
        NULL);
    g_object_set(G_OBJECT(capssetter), "caps", setter_filtercaps,NULL);
    g_object_set(G_OBJECT(capssetter), "replace", TRUE,NULL);
    
    //Adding capssetter elements to bin
    gst_bin_add_many (GST_BIN (ret), capssetter, NULL);

    //Linking capssetter elements
    if (!gst_element_link_many (last_element, capssetter, NULL)){
        GST_ERROR ("Linking capssetter part (A)-2 Fail...");
        return NULL;
    }
    return capssetter;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_video_encoder_elements (ExtRTSPOnvifMediaFactory * factory, 
        GstElement * ret, 
        GstElement * last_element){

    if(factory->priv->video_encoder){
        last_element = priv_ext_rtsp_onvif_media_factory_add_videoconvert_element(factory,ret, last_element);
        if(last_element == NULL){
            return NULL;
        }

        last_element = priv_ext_rtsp_onvif_media_factory_add_capssetter_element (factory, ret, last_element,factory->priv->fps,1,factory->priv->width,factory->priv->height); 
        if(last_element == NULL){
            return NULL;
        }

        GstElement * venc = gst_element_factory_make (factory->priv->video_encoder, "venc");
        GstElement * venc_capsfilter = gst_element_factory_make ("capsfilter", "venc_caps");
        if (!venc || !venc_capsfilter) {
            g_printerr ("One of the v4l2h264enc elements wasn't created... Exiting\n");
            return NULL;
        }

        //Setting caps on source and encoder
        //TODO Set profile and level based on probed compatible settings
        GstCaps *venc_filtercaps = gst_caps_new_simple ("video/x-h264",
            "profile", G_TYPE_STRING, "high",
            "level", G_TYPE_STRING, "4",
            NULL);
        g_object_set(G_OBJECT(venc_capsfilter), "caps", venc_filtercaps,NULL);

        //v4l2h264enc specific
        if(!strcmp(factory->priv->video_encoder,"v4l2h264enc")){
            GstStructure * structure = gst_structure_new_from_string ("controls,h264_profile=4,h264_level=11,video_bitrate=25000000");
            g_object_set(G_OBJECT(venc), "extra-controls", structure,NULL);
        }

        //Adding encoder elements to bin
        gst_bin_add_many (GST_BIN (ret), venc, venc_capsfilter, NULL);

        //Linking encoder elements
        if (!gst_element_link_many (last_element, venc, venc_capsfilter, NULL)){
            GST_ERROR ("Linking encoder part (A)-2 Fail...");
            return NULL;
        }
        
        //Set as last element for additional linking
        return venc_capsfilter;
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
        GstElement * videoscale = gst_element_factory_make ("videoscale", "vscale");

        //Validate videorate elements
        if (!videoscale) {
            g_printerr ("videorate wasn't created... Exiting\n");
            return NULL;
        }
        //Adding source elements to bin
        gst_bin_add_many (GST_BIN (ret), videoscale, NULL);

        //Linking source elements
        if (!gst_element_link_many (last_element, videoscale, NULL)){
            GST_ERROR ("Linking source part (A)-2 Fail...");
            return NULL;
        }
        last_element = videoscale;
    }
    return last_element;
}


static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_videorate_element (ExtRTSPOnvifMediaFactory * factory, GstElement * ret, GstElement * last_element, v4l2ParameterResults * input){
    if(input != NULL && factory->priv->fps != (gint)1.0*(input->device_denominator/input->device_numerator)){
        //Drop frames
        GST_WARNING("Droping framerate from '%i' to '%i'",(int)1.0*(input->device_denominator/input->device_numerator), factory->priv->fps);
        GstElement * videorate = gst_element_factory_make ("videorate", "vrate");
        g_object_set(G_OBJECT(videorate), "max-rate", factory->priv->fps,NULL);
        //Validate videorate elements
        if (!videorate) {
            g_printerr ("videorate wasn't created... Exiting\n");
            return NULL;
        }
        //Adding source elements to bin
        gst_bin_add_many (GST_BIN (ret), videorate, NULL);

        //Linking source elements
        if (!gst_element_link_many (last_element, videorate, NULL)){
            GST_ERROR ("Linking source part (A)-2 Fail...");
            return NULL;
        }
        last_element = videorate;
    }
    return last_element;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_source_elements (ExtRTSPOnvifMediaFactory * factory, GstElement * ret, GstElement * last_element, unsigned int dev_pixelformat, unsigned int dev_denominator, unsigned int dev_numerator, unsigned int dev_width, unsigned int dev_height){
    GstElement * src;
    //TODO handle alternative sources like picamsrc
    if(!strcmp(factory->priv->video_device,"test")){
        src = gst_element_factory_make ("videotestsrc", "src");
        //Validate source elements
        if (!src) {
            g_printerr ("the source  wasn't created... Exiting\n");
            return NULL;
        }
        g_object_set(G_OBJECT(src), "is-live", TRUE,NULL);
    } else {
        src = gst_element_factory_make ("v4l2src", "src");
        //Validate source elements
        if (!src) {
            g_printerr ("the source  wasn't created... Exiting\n");
            return NULL;
        }
        g_object_set(G_OBJECT(src), "device", factory->priv->video_device,NULL);
    }

    //Adding source elements to bin
    gst_bin_add_many (GST_BIN (ret), src, NULL);

    GstElement * src_capsfilter = gst_element_factory_make ("capsfilter", "src_caps");
    //Validate source elements
    if (!src_capsfilter) {
        g_printerr ("Capture caps filter element wasn't created... Exiting\n");
        return NULL;
    }

    GST_DEBUG("Setting Capture FPS [%d/%d]",dev_denominator,dev_numerator);
    //Setting Capture Caps filters
    GstCaps *src_filtercaps;
    char fourcc[5] = {0};
    switch(dev_pixelformat){
        case V4L2_FMT_H264:
            GST_DEBUG("Setting H264 Capture Caps");
            src_filtercaps = gst_caps_new_simple("video/x-h264",
                "framerate", GST_TYPE_FRACTION, dev_denominator, dev_numerator,
                "profile", G_TYPE_STRING, "high",
                "level", G_TYPE_STRING, "4",
                "width", G_TYPE_INT, dev_width,
                "height", G_TYPE_INT, dev_height,
                NULL);
            GstStructure * structure = gst_structure_new_from_string ("controls,video_bitrate=25000000");
            g_object_set(G_OBJECT(src), "extra-controls", structure,NULL);
            break;
        case V4L2_FMT_YUYV:
            GST_DEBUG("Setting Raw YUYV Capture Caps");
            src_filtercaps = gst_caps_new_simple ("video/x-raw",
                "framerate", GST_TYPE_FRACTION, dev_denominator, dev_numerator,
                "format", G_TYPE_STRING, "YUY2",
                "width", G_TYPE_INT, dev_width,
                "height", G_TYPE_INT, dev_height,
                NULL);
            break;
        case V4L2_FMT_MJPEG: //TODO support mjpeg stream
        default:
            strncpy(fourcc, (char *)&dev_pixelformat, 4);
            GST_ERROR("Unsupported pixel format : [%s] %d",fourcc, dev_pixelformat);
            return NULL;

    }

    g_object_set(G_OBJECT(src_capsfilter), "caps", src_filtercaps,NULL);


    //Adding source elements to bin
    gst_bin_add_many (GST_BIN (ret), src_capsfilter, NULL);

    //Linking source elements
    if (!gst_element_link_many (src, src_capsfilter, NULL)){
        GST_ERROR ("Linking source caps part (A)-2 Fail...");
        return NULL;
    }

    return src_capsfilter;
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
    if(input == NULL){
        dev_width = factory->priv->width;
        dev_height = factory->priv->height;
        dev_denominator = factory->priv->fps;
        dev_numerator = 1;
    } else {
        dev_width = input->device_width;
        dev_height = input->device_height;
        dev_denominator = input->device_denominator;
        dev_numerator = input->device_numerator;
    }

    //Create Capture elements
    last_element = priv_ext_rtsp_onvif_media_factory_add_source_elements(factory,ret, last_element, input->device_pixelformat,dev_denominator,dev_numerator,dev_width,dev_height);
    if(last_element == NULL){
        return NULL;
    }

    //Adjust device framerate to desired stream framerate
    last_element = priv_ext_rtsp_onvif_media_factory_add_videorate_element(factory,ret,last_element, input);
    if(last_element == NULL){
        return NULL;
    }

    //Adjust device resolution to desired stream resolution
    last_element = priv_ext_rtsp_onvif_media_factory_add_videoscale_element(factory,ret,last_element, dev_width, dev_height);
    if(last_element == NULL){
        return NULL;
    }

    GstElement * vq = gst_element_factory_make ("queue", "vqueue");

    //Validate source elements
    if (!vq) {
        GST_ERROR ("A queue element wasn't created... Exiting\n");
        return NULL;
    }
    
    //Adding source elements to bin
    gst_bin_add_many (GST_BIN (ret), vq, NULL);

    //Linking source elements
    if (!gst_element_link_many (last_element, vq, NULL)){
        GST_ERROR ("Linking source part (A)-2 Fail...");
        return NULL;
    }
    last_element = vq;

    //Check if device format is compatible with stream output
    if(!is_suported_output_format(input->device_pixelformat)){
        GST_WARNING("Using video encoder!!!");
        last_element = priv_ext_rtsp_onvif_media_factory_add_video_encoder_elements(factory,ret, last_element);
        if(last_element == NULL){
            return NULL;
        }
    } else {
        GST_WARNING("Using native video stream!!!");
    }

    //Create rtp payload sink
    GstElement * hparse = gst_element_factory_make ("h264parse", "h264parse");
    GstElement * hpay = gst_element_factory_make ("rtph264pay", "pay0");

    g_object_set(G_OBJECT(hpay), "pt", 96,NULL);

    if (!hparse || !hpay) {
        g_printerr ("One of the rtppayload elements wasn't created... Exiting\n");
        return NULL;
    }

    gst_bin_add_many (GST_BIN (ret), hparse, hpay, NULL);

    //Linking encoder elements
    if (!gst_element_link_many (last_element, hparse, hpay, NULL)){
        GST_ERROR ("Linking pay part (A)-2 Fail...");
        return NULL;
    }

    return ret;
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_audio_elements (ExtRTSPOnvifMediaFactory * factory, GstElement * ret){
    GstElement * audio_src = gst_element_factory_make ("audiotestsrc", "audio_src");
    GstElement * audio_enc = gst_element_factory_make ("mulawenc", "audio_enc");
    GstElement * audio_pay = gst_element_factory_make ("rtppcmupay", "pay1");

    if (!audio_src || !audio_enc || !audio_pay) {
        g_printerr ("One of the audio elements wasn't created... Exiting\n");
        return NULL;
    }
    gst_bin_add_many (GST_BIN (ret), audio_src, audio_enc, audio_pay, NULL);

    //Linking audio elements
    if (!gst_element_link_many (audio_src, audio_enc, audio_pay, NULL)){
        GST_ERROR ("Linking audio part (A)-2 Fail...");
        return NULL;
    }

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
    desires.desired_pixelformat = V4L2_PIX_FMT_H264;
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
    GST_DEBUG("audio_device : %s",factory->priv->audio_device);
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