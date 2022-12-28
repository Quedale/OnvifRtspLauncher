#include <gst/gst.h>
#include <stdio.h>
#include "vencoder-retriever.h"

GST_DEBUG_CATEGORY_STATIC (ext_vencoder_debug);
#define GST_CAT_DEFAULT (ext_vencoder_debug)

typedef struct {
    int error;
} VideoRetValue;

void
cb_message (GstBus     *bus,
            GstMessage *message,
            gpointer    user_data)
{
  VideoRetValue *ret = (VideoRetValue *) user_data;
    GST_ERROR("message..");
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      GST_DEBUG ("we received an error!");
        ret->error=1;
      break;
    case GST_MESSAGE_EOS:
      GST_DEBUG ("we reached EOS");
       ret->error=1;
      break;
    default:
      break;
  }
}

void
state_changed_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{   
    // VideoRetValue *ret = (VideoRetValue *) user_data;
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    if(new_state == GST_STATE_PLAYING){
        GST_DEBUG("SUCCESS!!!");
    }
    GST_DEBUG ("State set to %s", gst_element_state_get_name (new_state));
}

int
message_handler (GstBus * bus, GstMessage * message, gpointer p)
{ 
    VideoRetValue *ret = (VideoRetValue *) p;
    switch(message->type){
        case GST_MESSAGE_ANY:
        case GST_MESSAGE_INSTANT_RATE_REQUEST:
        case GST_MESSAGE_DEVICE_CHANGED:
        case GST_MESSAGE_REDIRECT:
        case GST_MESSAGE_STREAMS_SELECTED:
        case GST_MESSAGE_STREAM_COLLECTION:
        case GST_MESSAGE_PROPERTY_NOTIFY:
        case GST_MESSAGE_DEVICE_REMOVED:
        case GST_MESSAGE_DEVICE_ADDED:
        case GST_MESSAGE_EXTENDED:
        case GST_MESSAGE_HAVE_CONTEXT:
        case GST_MESSAGE_NEED_CONTEXT:
        case GST_MESSAGE_STREAM_START:
        case GST_MESSAGE_RESET_TIME:
        case GST_MESSAGE_TOC:
        case GST_MESSAGE_PROGRESS:
        case GST_MESSAGE_QOS:
        case GST_MESSAGE_STEP_START:
        case GST_MESSAGE_REQUEST_STATE:
        case GST_MESSAGE_ASYNC_DONE:
        case GST_MESSAGE_ASYNC_START:
        case GST_MESSAGE_LATENCY:
        case GST_MESSAGE_DURATION_CHANGED:
        case GST_MESSAGE_SEGMENT_DONE:
        case GST_MESSAGE_SEGMENT_START:
        case GST_MESSAGE_ELEMENT:
        case GST_MESSAGE_APPLICATION:
        case GST_MESSAGE_STREAM_STATUS:
        case GST_MESSAGE_STRUCTURE_CHANGE:
        case GST_MESSAGE_NEW_CLOCK:
        case GST_MESSAGE_CLOCK_LOST:
        case GST_MESSAGE_CLOCK_PROVIDE:
        case GST_MESSAGE_STEP_DONE:
        case GST_MESSAGE_STATE_DIRTY:
        case GST_MESSAGE_BUFFERING:
        case GST_MESSAGE_TAG:
        case GST_MESSAGE_INFO:
        case GST_MESSAGE_UNKNOWN:
            break;
        case GST_MESSAGE_EOS:
            GST_DEBUG("msg : GST_MESSAGE_EOS");
            ret->error = 1;
            break;
        case GST_MESSAGE_ERROR:
            GST_DEBUG("msg : GST_MESSAGE_ERROR");
            ret->error = 1;
            break;
        case GST_MESSAGE_WARNING:
            GST_DEBUG("msg : GST_MESSAGE_WARNING");
            GError *gerror;
            gchar *debug;

            if (message->type == GST_MESSAGE_ERROR)
                gst_message_parse_error (message, &gerror, &debug);
            else
                gst_message_parse_warning (message, &gerror, &debug);

            gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
            g_error_free (gerror);
            g_free (debug);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            state_changed_cb(bus,message,p);
            break;
    }
    return TRUE;
}

int  
test_videoencoder(char * encodername){

    GST_DEBUG("Testing video encoder '%s'",encodername);
    //Create temporary pipeline to use AutoDetect element
    GstElement * pipeline = gst_pipeline_new ("audiotest-pipeline");
    GstElement * src = gst_element_factory_make ("videotestsrc", "src");
    GstElement * encoder = gst_element_factory_make (encodername, "encoder");
    GstElement * capsfilter = gst_element_factory_make ("capsfilter", "enc_caps");
    GstElement * sink = gst_element_factory_make ("fakesink", "detectaudiosink");

    GstCaps* filtercaps = gst_caps_from_string("video/x-h264, profile=baseline, level=(string)4");
    g_object_set(G_OBJECT(capsfilter), "caps", filtercaps,NULL);

    //Initializing return value
    VideoRetValue * ret_val = malloc(sizeof(VideoRetValue));
    ret_val->error = 0;

    // GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    // guint watch_id = gst_bus_add_watch (bus, message_handler, ret_val);
    // gst_object_unref (bus);

    // GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    // gst_bus_add_signal_watch (bus);
    // g_signal_connect (bus, "message", (GCallback) cb_message, ret_val);
    // gst_object_unref (bus);


    //Validate elements
    if (!pipeline || \
        !src || \
        !encoder || \
        !capsfilter || \
        !sink) {
        GST_WARNING ("One of the elements wasn't created... Exiting");
        return FALSE;
    }

    //Add elements to pipeline
    gst_bin_add_many (GST_BIN (pipeline), \
        src, \
        encoder, \
        capsfilter, \
        sink, NULL);

    //Link elements
    if (!gst_element_link_many (src, \
        encoder, \
        capsfilter, \
        sink, NULL)){
        GST_WARNING ("Linking part (A)-2 Fail...");
        return FALSE;
    }


    //Play pipeline to construct it.
    GstStateChangeReturn ret;
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR ("Unable to set the pipeline to the playing state.");
        gst_object_unref (pipeline);
        return FALSE;
    }

    // GST_DEBUG("poping message");
    //TODO pop message and consume them until error or state changed to playing.
    // GstMessage * msg = gst_bus_poll (bus,GST_MESSAGE_WARNING, -1);
    // message_handler(bus,msg,ret_val);
    // GST_DEBUG("poping message done");
    // return FALSE;
    //Caps are validated in a sperate thread.
    //Message signals are dispatched using main loop

    //Stop pipeline
    ret = gst_element_set_state (pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR ("Unable to set the pipeline to the ready state.");
        gst_object_unref (pipeline);
        return FALSE;
    }

    //Clean up
    ret = gst_element_set_state (pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR ("Unable to set the pipeline to the null state.");
        gst_object_unref (pipeline);
        return FALSE;
    }
    gst_object_unref (pipeline);
    
    return TRUE;
}

char *  
retrieve_videoencoder(void){
    int ret;
    GST_DEBUG_CATEGORY_INIT (ext_vencoder_debug, "ext-venc", 0, "Extension to support v4l capabilities");
    gst_debug_set_threshold_for_name ("ext-venc", GST_LEVEL_LOG);
    
#ifdef ENABLERPI
    ret = test_videoencoder("omxh264enc");
    if(ret){
        return "omxh264enc";
    }
#endif
    ret = test_videoencoder("v4l2h264enc");
    if(ret){
        // return "v4l2h264enc extra-controls=\"controls, video_bitrate=300000, h264_level=4, h264_profile=4\" ! video/x-h264, profile=main, level=(string)5";
         return "v4l2h264enc";
    }

    ret = test_videoencoder("nvh264enc");
    if(ret){
        return "nvh264enc";
    }

    ret = test_videoencoder("openh264enc");
    if(ret){
        return "openh264enc";
    }

    ret = test_videoencoder("x264enc");
    if(ret){
        return "x264enc";
    }

    return NULL;
}
