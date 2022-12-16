#include <gst/gst.h>
#include <stdio.h>
#include "vencoder-retriever.h"

typedef struct {
    int error;
} VideoRetValue;

void
cb_message (GstBus     *bus,
            GstMessage *message,
            gpointer    user_data)
{
  VideoRetValue *ret = (VideoRetValue *) user_data;
    g_warning("message...\n");
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      g_print ("we received an error!\n");
        ret->error=1;
      break;
    case GST_MESSAGE_EOS:
      g_print ("we reached EOS\n");
       ret->error=1;
      break;
    default:
      break;
  }
}

void
state_changed_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{   
    VideoRetValue *ret = (VideoRetValue *) user_data;
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    if(new_state == GST_STATE_PLAYING){
        printf("SUCCESS!!!\n");
    }
    g_print ("State set to %s\n", gst_element_state_get_name (new_state));
}

int
message_handler (GstBus * bus, GstMessage * message, gpointer p)
{ 
    VideoRetValue *ret = (VideoRetValue *) p;
    switch(message->type){
        case GST_MESSAGE_EOS:
            printf("msg : GST_MESSAGE_EOS\n");
            ret->error = 1;
            break;
        case GST_MESSAGE_ERROR:
            printf("msg : GST_MESSAGE_ERROR\n");
            ret->error = 1;
            break;
        case GST_MESSAGE_WARNING:
            printf("msg : GST_MESSAGE_WARNING\n");
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

    printf("Testing video encoder '%s'\n",encodername);
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
        g_printerr ("One of the elements wasn't created... Exiting\n");
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
        g_warning ("Linking part (A)-2 Fail...");
        return FALSE;
    }


    //Play pipeline to construct it.
    GstStateChangeReturn ret;
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return FALSE;
    }

    printf("poping message\n");
    //TODO pop message and consume them until error or state changed to playing.
    // GstMessage * msg = gst_bus_poll (bus,GST_MESSAGE_WARNING, -1);
    // message_handler(bus,msg,ret_val);
    // printf("poping message done\n");
    // return FALSE;
    //Caps are validated in a sperate thread.
    //Message signals are dispatched using main loop

    //Stop pipeline
    ret = gst_element_set_state (pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the ready state.\n");
        gst_object_unref (pipeline);
        return FALSE;
    }

    //Clean up
    ret = gst_element_set_state (pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the null state.\n");
        gst_object_unref (pipeline);
        return FALSE;
    }
    gst_object_unref (pipeline);
    
    return TRUE;
}

char *  
retrieve_videoencoder(void){
    int ret;

#ifdef ENABLERPI
    ret = test_videoencoder("omxh264enc");
    if(ret){
        return "omxh264enc";
    }
#endif
    ret = test_videoencoder("v4l2h264enc");
    if(ret){
        return "v4l2h264enc extra-controls=\"controls, video_bitrate=300000, h264_level=4, h264_profile=4\" ! video/x-h264, profile=main, level=(string)5";
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
