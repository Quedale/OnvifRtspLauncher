
#include "onvif-server-snapshot.h"
#include <gst/gst.h>
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (ext_onvif_server_snapshot_debug);
#define GST_CAT_DEFAULT (ext_onvif_server_snapshot_debug)

typedef struct _SnapshotData {
    GMainLoop *loop;
    GstElement * pipeline;
    GstElement * sink;
    pthread_mutex_t * lock;
    pthread_cond_t * cond;
    int aborted;
} SnapshotData;

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, SnapshotData * snap) {
    GST_ERROR ("End-Of-Stream reached.\n");
    if(GST_IS_ELEMENT(snap->pipeline)){
        gst_element_set_state (snap->pipeline, GST_STATE_NULL);
    }

    snap->aborted = 1;
    pthread_cond_signal(snap->cond);
}

static gboolean
message_handler (GstBus     *bus, GstMessage *message, gpointer    data){
    SnapshotData * snap = (SnapshotData *)data;
    if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS || GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR){
        eos_cb(bus,message,snap);
    } else if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_WARNING){
        GError *gerror;
        gchar *debug;

        if (message->type == GST_MESSAGE_ERROR)
            gst_message_parse_error (message, &gerror, &debug);
        else
            gst_message_parse_warning (message, &gerror, &debug);

        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        g_error_free (gerror);
        g_free (debug);
    }

    return TRUE;
}

/* Dynamically link */
static void on_pad_added (GstElement *element, GstPad *new_pad, gpointer data){
    GstPad *sinkpad;
    GstPadLinkReturn ret;
    GstElement *decoder = (GstElement *) data;

    /* We can now link this pad with the rtsp-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    ret = gst_pad_link (new_pad, sinkpad);

    if (GST_PAD_LINK_FAILED (ret)) {
      GST_WARNING("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(decoder));
    }

    gst_object_unref (sinkpad);
}

static GstElement * create_video_bin(SnapshotData * snap){
    GstElement *vdecoder, *videoconvert, *pngenc, *video_bin;
    GstPad *pad, *ghostpad;

    video_bin = gst_bin_new("video_bin");
    vdecoder = gst_element_factory_make ("decodebin3", NULL);
    videoconvert = gst_element_factory_make ("videoconvert", NULL);
    pngenc = gst_element_factory_make ("pngenc", NULL);
    snap->sink = gst_element_factory_make ("appsink", NULL);

    if (!video_bin ||
        !vdecoder ||
        !videoconvert ||
        !pngenc ||
        !snap->sink) {
        GST_ERROR ("One of the video elements wasn't created... Exiting\n");
        return NULL;
    }

    g_object_set (G_OBJECT (snap->sink), "sync", FALSE, NULL);
    g_object_set (G_OBJECT (snap->sink), "max-buffers", 2, NULL);
    g_object_set (G_OBJECT (snap->sink), "drop", TRUE, NULL);
    g_object_set (G_OBJECT (snap->sink), "emit-signals", TRUE, NULL);

    //pngenc
    g_object_set (G_OBJECT (pngenc), "snapshot", TRUE, NULL);
    

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (video_bin),
        vdecoder,
        videoconvert,
        pngenc,
        snap->sink, NULL);

    // Link confirmation
    if (!gst_element_link_many (videoconvert,
        pngenc,
        snap->sink, NULL)){
        GST_ERROR ("Linking video part (A)-2 Fail...");
        return NULL;
    }

    // Dynamic Pad Creation
    if(! g_signal_connect (vdecoder, "pad-added", G_CALLBACK (on_pad_added),videoconvert)){
        GST_ERROR ("Linking (A)-1 part with part (A)-2 Fail...");
        return NULL;
    }

    pad = gst_element_get_static_pad (vdecoder, "sink");
    if (!pad) {
        // TODO gst_object_unref 
        GST_ERROR("unable to get decoder static sink pad");
        return NULL;
    }

    ghostpad = gst_ghost_pad_new ("bin_sink", pad);
    gst_element_add_pad (video_bin, ghostpad);
    gst_object_unref (pad);

    return video_bin;
}

/* Dynamically link */
static void on_rtsp_pad_added (GstElement *element, GstPad *new_pad, SnapshotData * snap){
    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (element));

    GstPadLinkReturn pad_ret;
    GstPad *sink_pad = NULL;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  
    gint payload_v;
    gst_structure_get_int(new_pad_struct,"payload", &payload_v);

    if (payload_v == 96) {
        gst_event_new_flush_stop(0);
        GstElement * video_bin = create_video_bin(snap);

        gst_bin_add_many (GST_BIN (snap->pipeline), video_bin, NULL);

        sink_pad = gst_element_get_static_pad (video_bin, "bin_sink");

        pad_ret = gst_pad_link (new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED (pad_ret)) {
            GST_ERROR ("failed to link dynamically '%s' to '%s'\n",GST_ELEMENT_NAME(element),GST_ELEMENT_NAME(video_bin));
            goto exit;
        }

        pthread_cond_signal(snap->cond);
        gst_element_sync_state_with_parent(video_bin);
    }//Ignore other payload. We only care for the picture

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);
  if (sink_pad != NULL)
    gst_object_unref (sink_pad);
}

void save_buffer_to_file(GstSample * sample, char * output){
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMemory* memory = gst_buffer_get_all_memory(buffer);
    GstMapInfo map_info;

    if(! gst_memory_map(memory, &map_info, GST_MAP_READ)) {
        gst_memory_unref(memory);
        gst_sample_unref(sample);
        GST_ERROR("Unable to read sample memory.");
        return;
    }

    FILE *fptr;
    fptr = fopen(output,"wb");
    if(fptr == NULL){
        GST_ERROR("Unable open output file.");
        return;          
    }
    fwrite(map_info.data, map_info.size, 1, fptr); 

    fclose(fptr);
}

void * start_gstreamer_loop(void * data){
    SnapshotData * snap = (SnapshotData*) data;
    g_main_loop_run (snap->loop);
    pthread_exit(0);
}

void take_snapshot(char * url, char * output){
    GST_DEBUG_CATEGORY_INIT (ext_onvif_server_snapshot_debug, "ext_onvif_server_snapshot", 0, "Extended ONVIF snapshot server");
    gst_debug_set_threshold_for_name ("ext_onvif_server_snapshot", GST_LEVEL_LOG);
        
    GstElement *src;
    SnapshotData * snap = malloc(sizeof(SnapshotData));
    snap->sink = NULL;
    snap->lock = malloc(sizeof(pthread_mutex_t));
    snap->cond = malloc(sizeof(pthread_cond_t));
    snap->aborted = 0;
    pthread_mutex_init(snap->lock, NULL);
    pthread_cond_init(snap->cond, NULL);

    snap->pipeline = gst_pipeline_new ("snapshot-pipeline");
    src = gst_element_factory_make ("rtspsrc", "rtspsrc");

    if (!snap->pipeline){
        GST_ERROR("Failed to created pipeline. Check your gstreamer installation...\n");
        return;
    }
    if (!src){
        GST_ERROR ("Failed to created rtspsrc. Check your gstreamer installation...\n");
        return;
    }

    // Add Elements to the Bin
    gst_bin_add_many (GST_BIN (snap->pipeline), src, NULL);

    // Dynamic Pad Creation
    if(! g_signal_connect (src, "pad-added", G_CALLBACK (on_rtsp_pad_added),snap))
    {
        GST_ERROR ("Linking part (1) with part (A)-1 Fail...");
        return;
    }


    g_object_set (G_OBJECT (src), "location", url, NULL);
    g_object_set (G_OBJECT (src), "buffer-mode", 3, NULL);
    g_object_set (G_OBJECT (src), "latency", 0, NULL);
    g_object_set (G_OBJECT (src), "teardown-timeout", 0, NULL); 
    g_object_set (G_OBJECT (src), "user-agent", "OnvifServerSnnapshot-Linux-0.0", NULL);
    g_object_set (G_OBJECT (src), "do-retransmission", FALSE, NULL);
    g_object_set (G_OBJECT (src), "onvif-mode", TRUE, NULL);
    g_object_set (G_OBJECT (src), "is-live", TRUE, NULL);

    snap->loop = g_main_loop_new (NULL, FALSE);

    /* set up bus */
    GstBus *bus = gst_element_get_bus (snap->pipeline);
    gst_bus_add_watch(bus,message_handler,snap);
    gst_object_unref (bus);

    GST_INFO("Playing pipeline...");
    gst_element_set_state (snap->pipeline, GST_STATE_PLAYING);


    pthread_t pthread;
    pthread_create(&pthread, NULL, start_gstreamer_loop, snap);

    GST_INFO("Waiting for pipeline to start...");
    pthread_mutex_lock(snap->lock);
    pthread_cond_wait(snap->cond, snap->lock);
    pthread_mutex_unlock(snap->lock);

    if(!snap->aborted){
        int i =0;
        GstSample *sample = NULL;
        while(i<3 && sample == NULL){
            GST_INFO("Taking Snapshop attempt #%i",++i);
            g_signal_emit_by_name (snap->sink, "pull-sample", &sample);
            if (!sample){
                continue;
            }
            GST_INFO("Saving snapshot...");
            save_buffer_to_file(sample,output);
            GST_INFO("Done capturing file");
        };
        if (!sample){
            GST_ERROR("Unable to pull sample from sink.");
        }

    } else {
        GST_ERROR("Unable to get snapshot...");
    }

    //Killing main loop before stopping stream prevents EOF from printing
    g_main_loop_quit (snap->loop);
    pthread_join(pthread, NULL);

    gst_element_set_state (snap->pipeline, GST_STATE_READY);
    gst_element_set_state (snap->pipeline, GST_STATE_NULL);
    gst_object_unref (snap->pipeline);

    pthread_mutex_destroy(snap->lock);
    pthread_cond_destroy(snap->cond);
    free(snap->lock);

    free(snap->cond);
    free(snap);
}