#include <gst/gst.h>
#include <stdio.h>
#include "sink-retriever.h"

int  
test_audiosink(char * sinkname){

    //Create temporary pipeline to use AutoDetect element
    GstElement * pipeline = gst_pipeline_new ("audiotest-pipeline");
    GstElement * src = gst_element_factory_make ("audiotestsrc", "src");
    GstElement * volume = gst_element_factory_make ("volume", "vol");
    GstElement * sink = gst_element_factory_make (sinkname, "detectaudiosink");
    g_object_set (G_OBJECT (volume), "mute", TRUE, NULL); 

    if(!strcmp(sinkname,"alsasink")){
        g_object_set (G_OBJECT (sink), "device", "hw:0", NULL); 
    }

    //Validate elements
    if (!pipeline || \
        !src || \
        !volume || \
        !sink) {
        g_printerr ("One of the elements wasn't created... Exiting\n");
        return FALSE;
    }

    //Add elements to pipeline
    gst_bin_add_many (GST_BIN (pipeline), \
        src, \
        volume, \
        sink, NULL);

    //Link elements
    if (!gst_element_link_many (src, \
        volume, \
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
retrieve_audiosink(void){

    int ret;
    ret = test_audiosink("pulsesink");
    if(ret){
        return "pulsesink async=false";
    }

    ret = test_audiosink("alsasink");
    if(ret){
        return "alsasink device=hw:0 async=false";
    }

    return "fakesink";
}