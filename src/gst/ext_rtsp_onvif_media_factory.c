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
    gint * width;
    gint * height;
    gint * fps;
    gchar * video_encoder;

    //Audio capture parameters
    gchar * audio_device;

};

G_DEFINE_TYPE_WITH_PRIVATE (ExtRTSPOnvifMediaFactory, ext_rtsp_onvif_media_factory, GST_TYPE_RTSP_ONVIF_MEDIA_FACTORY);

void 
ext_rtsp_onvif_media_factory_set_v4l2_params(ExtRTSPOnvifMediaFactory * factory, v4l2ParameterResults * params){
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    if(params != NULL){
        GST_LOG("width : '%f'",params->device_width);
        GST_LOG("height : '%f'",params->device_height);
        GST_LOG("fps : '%f'",params->device_denominator/params->device_numerator);
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
    factory, const gint * width)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",width);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->width = (gint *) width;
    g_mutex_unlock (&factory->priv->lock);
}

void
ext_rtsp_onvif_media_factory_set_height (ExtRTSPOnvifMediaFactory *
    factory, const gint * height)
{
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",height);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->height = (gint *) height;
    g_mutex_unlock (&factory->priv->lock);
}

void 
ext_rtsp_onvif_media_factory_set_fps (ExtRTSPOnvifMediaFactory * factory, const gint * fps){
    g_return_if_fail (IS_EXT_RTSP_ONVIF_MEDIA_FACTORY (factory));
    GST_LOG("'%i'",fps);

    g_mutex_lock (&factory->priv->lock);
    factory->priv->fps = (gint *) fps;
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
    ext_rtsp_onvif_media_factory_set_width(factory,(gint *)640);
    ext_rtsp_onvif_media_factory_set_height(factory,(gint *)480);
    ext_rtsp_onvif_media_factory_set_fps(factory,(gint *)10);

    g_mutex_init (&factory->priv->lock);
}

static GstElement * 
priv_ext_rtsp_onvif_media_factory_add_video_elements (ExtRTSPOnvifMediaFactory * factory, GstElement * ret){
    //Create source
    GstElement * last_element;

    v4l2ParameterResults * input = factory->priv->v4l2params;
    //TODO handle alternative sources like picamsrc
    if(!strcmp(factory->priv->video_device,"test")){
        GstElement * src = gst_element_factory_make ("videotestsrc", "src");
        //Validate source elements
        if (!src) {
            g_printerr ("the source  wasn't created... Exiting\n");
            return NULL;
        }
        g_object_set(G_OBJECT(src), "is-live", TRUE,NULL);
        last_element = src;
    } else {
        GstElement * src = gst_element_factory_make ("v4l2src", "src");
        //Validate source elements
        if (!src) {
            g_printerr ("the source  wasn't created... Exiting\n");
            return NULL;
        }
        g_object_set(G_OBJECT(src), "device", factory->priv->video_device,NULL);
        last_element = src;
    }

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
        gst_bin_add_many (GST_BIN (ret), last_element, videorate, NULL);

        //Linking source elements
        if (!gst_element_link_many (last_element, videorate, NULL)){
            GST_ERROR ("Linking source part (A)-2 Fail...");
            return NULL;
        }
        last_element = videorate;
    }

    if(input != NULL && (input->device_width != factory->priv->width || input->device_height != factory->priv->height)){
        //Scale down required
        //TODO instead crop from center
        GST_WARNING("Scale down from '%i/%i' to '%i/%i'",input->device_width,input->device_height,factory->priv->width,factory->priv->height);
        GstElement * videoscale = gst_element_factory_make ("videoscale", "vscale");

        //Validate videorate elements
        if (!videoscale) {
            g_printerr ("videorate wasn't created... Exiting\n");
            return NULL;
        }
        //Adding source elements to bin
        gst_bin_add_many (GST_BIN (ret), last_element, videoscale, NULL);

        //Linking source elements
        if (!gst_element_link_many (last_element, videoscale, NULL)){
            GST_ERROR ("Linking source part (A)-2 Fail...");
            return NULL;
        }
        last_element = videoscale;
    }
    GstElement * src_capsfilter = gst_element_factory_make ("capsfilter", "src_caps");
    GstElement * vq = gst_element_factory_make ("queue", "vqueue");

    //Validate source elements
    if (!src_capsfilter || !vq) {
        g_printerr ("One of the source elements wasn't created... Exiting\n");
        return NULL;
    }

    //Adding source elements to bin
    gst_bin_add_many (GST_BIN (ret), last_element, src_capsfilter, vq, NULL);

    //Linking source elements
    if (!gst_element_link_many (last_element, src_capsfilter, vq, NULL)){
        GST_ERROR ("Linking source part (A)-2 Fail...");
        return NULL;
    }

    //Set as last element for additional linking
    last_element = vq;

    if(factory->priv->video_encoder){
        GstElement * videoconvert = gst_element_factory_make ("videoconvert", "videoconvert");
        GstElement * venc = gst_element_factory_make (factory->priv->video_encoder, "venc");
        GstElement * venc_capsfilter = gst_element_factory_make ("capsfilter", "venc_caps");
        if (!videoconvert || !venc || !venc_capsfilter) {
            g_printerr ("One of the v4l2h264enc elements wasn't created... Exiting\n");
            return NULL;
        }
        
        //Setting caps on source and encoder
        GstCaps *venc_filtercaps = gst_caps_new_simple ("video/x-h264",
            "profile", G_TYPE_STRING, "main",
            "level", G_TYPE_STRING, "4",
            NULL);
        g_object_set(G_OBJECT(venc_capsfilter), "caps", venc_filtercaps,NULL);
        GstCaps *src_filtercaps = gst_caps_new_simple ("video/x-raw",
            "framerate", GST_TYPE_FRACTION, factory->priv->fps, 1,
            "format", G_TYPE_STRING, "YUY2",
            "width", G_TYPE_INT, factory->priv->width,
            "height", G_TYPE_INT, factory->priv->height,
            NULL);
        g_object_set(G_OBJECT(src_capsfilter), "caps", src_filtercaps,NULL);

        //Adding encoder elements to bin
        gst_bin_add_many (GST_BIN (ret), videoconvert, venc, venc_capsfilter, NULL);

        //Linking encoder elements
        if (!gst_element_link_many (last_element, videoconvert, venc, venc_capsfilter, NULL)){
            GST_ERROR ("Linking encoder part (A)-2 Fail...");
            return NULL;
        }
        
        //Set as last element for additional linking
        last_element = venc_capsfilter;
    } else {
        // //TODO handle h264 support native cam live below and skip encoder creation
        // //Im currently unable to test this as my current installation is unable to set this pixelformat type "VIDIOC_S_FMT: failed: Invalid argument"
        GstCaps *src_filtercaps = gst_caps_new_simple ("video/x-h264",
            "profile", G_TYPE_STRING, "main",
            "level", G_TYPE_STRING, "4",
            "framerate", GST_TYPE_FRACTION, factory->priv->fps, 1,
            "format", G_TYPE_STRING, "YUY2",
            "width", G_TYPE_INT, factory->priv->width,
            "height", G_TYPE_INT, factory->priv->height,
            NULL);
        g_object_set(G_OBJECT(src_capsfilter), "caps", src_filtercaps,NULL);
    }

    //Create rtp payload sink
    GstElement * hparse = gst_element_factory_make ("h264parse", "h264parse");
    GstElement * hpay = gst_element_factory_make ("rtph264pay", "pay0");

    g_object_set(G_OBJECT(hpay), "pt", 96,NULL);

    if (!hparse || !hpay) {
        g_printerr ("One of the v4l2h264enc elements wasn't created... Exiting\n");
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
    desires.desired_pixelformat = V4L2_FMT_YUYV;
    if(strcmp(factory->priv->video_device,"test")){
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

  /* ERRORS */
no_launch:
    {
        GST_ERROR ("no launch line specified");
        g_free (launch);
        return NULL;
    }
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