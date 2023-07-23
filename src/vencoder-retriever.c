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
message_handler (GstBus * bus, GstMessage * message, gpointer p){
    VideoRetValue *ret = (VideoRetValue *) p;
    if(message->type == GST_MESSAGE_EOS || message->type == GST_MESSAGE_ERROR){
        ret->error = 1;
    } else if(message->type == GST_MESSAGE_WARNING) {
        GError *gerror;
        gchar *debug;

        if (message->type == GST_MESSAGE_ERROR)
            gst_message_parse_error (message, &gerror, &debug);
        else
            gst_message_parse_warning (message, &gerror, &debug);

        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        g_error_free (gerror);
        g_free (debug);
    } else if(message->type == GST_MESSAGE_STATE_CHANGED){
        state_changed_cb(bus,message,p);
    }
    return TRUE;
}

int  
test_videoencoder(char * encodername){
    GstElement * last_element;

    GST_DEBUG("Testing video encoder '%s'",encodername);
    //Create temporary pipeline to use AutoDetect element
    GstElement * pipeline = gst_pipeline_new ("audiotest-pipeline");
    GstElement * src = gst_element_factory_make ("videotestsrc", "src");
    GstElement * encoder = gst_element_factory_make (encodername, "encoder");
    GstElement * sink = gst_element_factory_make ("fakesink", "detectaudiosink");

    //Initializing return value
    VideoRetValue * ret_val = malloc(sizeof(VideoRetValue));
    ret_val->error = 0;

    //Validate elements
    if (!pipeline || \
        !src || \
        !encoder) {
        GST_WARNING ("One of the elements wasn't created... Exiting");
        return FALSE;
    }

    //Add elements to pipeline
    gst_bin_add_many (GST_BIN (pipeline), \
        src, \
        encoder, NULL);

    //Link elements
    if (!gst_element_link_many (src, \
        encoder, NULL)){
        GST_WARNING ("Linking part (A)-2 Fail...");
        return FALSE;
    }
    last_element = encoder;

    //Add elements to pipeline
    gst_bin_add_many (GST_BIN (pipeline), sink, NULL);

    if (!gst_element_link_many (last_element, sink, NULL)){
        GST_WARNING ("Failed to link muxer...");
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
retrieve_videoencoder_h264(void){
    int ret;   
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

char *  
retrieve_videoencoder_h265(void){
    int ret;   
#ifdef ENABLERPI
    ret = test_videoencoder("omxh265enc");
    if(ret){
        return "omxh265enc";
    }
#endif
    ret = test_videoencoder("v4l2h265enc");
    if(ret){
        // return "v4l2h264enc extra-controls=\"controls, video_bitrate=300000, h264_level=4, h264_profile=4\" ! video/x-h264, profile=main, level=(string)5";
         return "v4l2h265enc";
    }

    ret = test_videoencoder("nvh265enc");
    if(ret){
        return "nvh265enc";
    }

    ret = test_videoencoder("x265enc");
    if(ret){
        return "x265enc";
    }

    return NULL;
}

char *  
retrieve_videoencoder_mjpeg(void){
    int ret;   

    ret = test_videoencoder("jpegenc");
    if(ret){
        return "jpegenc";
    }

    return NULL;
}

char *  
retrieve_videoencoder(char * codec){
    GST_DEBUG_CATEGORY_INIT (ext_vencoder_debug, "ext-venc", 0, "Extension to support v4l capabilities");
    gst_debug_set_threshold_for_name ("ext-venc", GST_LEVEL_LOG);

    if(strcmp(codec,"h264") == 0){
        return retrieve_videoencoder_h264();
    } else if(strcmp(codec,"h265") == 0){
        return retrieve_videoencoder_h265();
    } else if(strcmp(codec,"mjpeg") == 0){
        return retrieve_videoencoder_mjpeg();
    }

    return NULL;
}