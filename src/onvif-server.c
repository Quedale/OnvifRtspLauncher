/* GStreamer
 * Copyright (C) 2017 Sebastian Dröge <sebastian@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-onvif-server.h>

#include <string.h>

#include <argp.h>
#include <stdbool.h>
#include <stdio.h>

#include "onvifinitstaticplugins.h"
#include "sink-retriever.h"
#include "vencoder-retriever.h"
#include "gst/ext_rtsp_onvif_media_factory.h"

GST_DEBUG_CATEGORY_STATIC (ext_onvif_server_debug);
#define GST_CAT_DEFAULT (ext_onvif_server_debug)


const char *argp_program_version = "0.0";
const char *argp_program_bug_address = "<your@email.address>";
static char doc[] = "Your program description.";
static struct argp_option options[] = { 
    { "video", 'v', "VIDEO", 0, "Input video device. (Default: /dev/video0)"},
    { "audio", 'a', "AUDIO", 0, "Input audio device. (Default: none)"},
    { "encoder", 'e', "ENCODER", 0, "Gstreamer encoder. (Default: x264enc)"},
    { "width", 'w', "WIDTH", 0, "Video input width. (Default: 640)"},
    { "height", 'h', "HEIGHT", 0, "Video input height. (Default: 480)"},
    { "fps", 'f', "FPS", 0, "Video input height. (Default: 30)"},
    { "format", 't', "FORMAT", 0, "Video input format. (Default: YUY2)"},
    { "mount", 'm', "MOUNT", 0, "URL mount point. (Default: h264)"},
    { "port", 'p', "PORT", 0, "Network port to listen. (Default: 8554)"},
    { 0 } 
};

//Define argument struct
struct arguments {
    char *vdev;
    char *adev;
    int width;
    int height;
    int fps;
    char *format;
    char *encoder;
    char *mount;
    int port;
};

//main arguments processing function
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'v': arguments->vdev = arg; break;
    case 'a': arguments->adev = arg; break;
    case 'w': arguments->width = atoi(arg); break;
    case 'h': arguments->height = atoi(arg); break;
    case 'f': arguments->fps = atoi(arg); break;
    case 'e': arguments->encoder = arg; break;
    case 'm': arguments->mount = arg; break;
    case 'p': arguments->port = atoi(arg); break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

//Initialize argp context
static struct argp argp = { options, parse_opt, NULL, doc, 0, 0, 0 };

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}


int
main (int argc, char *argv[])
{
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    struct arguments arguments;
    char *strbin;

    gst_init (&argc, &argv);

    GST_DEBUG_CATEGORY_INIT (ext_onvif_server_debug, "ext-onvif-server", 0, "Extended ONVIF server");
    
    onvif_init_static_plugins();

    arguments.vdev = "/dev/video0";
    arguments.adev = NULL;
    arguments.width = 640;
    arguments.height = 480;
    arguments.fps = 10;
    arguments.format = "YUY2";
    arguments.encoder = NULL;
    arguments.mount = "h264";
    arguments.port = 8554;

    /* Default values. */

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(!arguments.encoder){
        arguments.encoder = retrieve_videoencoder();
        if(!arguments.encoder){
            GST_ERROR("No functional video encoder found.");
            return 1;
        }
    } else {
        GST_WARNING("Video Encoder override '%s'",arguments.encoder);
    }

    GST_DEBUG ("vdev : %s\n", arguments.vdev);
    GST_DEBUG ("adev : %s\n", arguments.adev);
    GST_DEBUG ("width : %i\n", arguments.width);
    GST_DEBUG ("height : %i\n", arguments.height);
    GST_DEBUG ("format : %s\n", arguments.format);
    GST_DEBUG ("encoder : %s\n", arguments.encoder);
    GST_DEBUG ("mount : %s\n", arguments.mount);
    GST_DEBUG ("port : %i\n", arguments.port);
    GST_DEBUG ("fps : %i\n", arguments.fps);
    
    SupportedAudioSinkTypes audio_sink_type;
    audio_sink_type = retrieve_audiosink();

    loop = g_main_loop_new (NULL, FALSE);
    /* create a server instance */
    server = gst_rtsp_onvif_server_new ();

    /* get the mount points for this server, every server has a default object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server);
    
    // TODO handle sink override
    char * audiosink;
    switch(audio_sink_type){
        case ONVIF_PULSE:
            audiosink = "pulsesink async=false";
            break;
        case ONVIF_ASLA:
            audiosink = "alsasink async=false";
            break;
        case ONVIF_OMX:
            audiosink = "omxhdmiaudiosink";
            break;
        case ONVIF_NA:
        default:
            audiosink = "fakesink";
            GST_WARNING("No valid audio sink found for backchannel!");
    }

    //TODO use switchbin to handle LAW and AAC
    char * backchannel_lauch;
    if(!asprintf(&backchannel_lauch, "( capsfilter caps=\"application/x-rtp, media=audio, payload=0, clock-rate=8000, encoding-name=PCMU\" name=depay_backchannel ! rtppcmudepay ! mulawdec ! %s )",
        audiosink)){
        GST_ERROR("Unable to construct backchannel");
        return -2;
    } 

    // Test backpipe
    // "( capsfilter caps=\"application/x-rtp, media=audio, payload=0, clock-rate=8000, encoding-name=PCMU\" name=depay_backchannel ! fakesink async=false )");

    // async-handling will make it pull buffer from appsrc.
    // message-forward will prevent a broken pipe if the stream is used more than once
    // For some reason, the decodebin will randomly stop pulling from appsrc after multiple re-use
    // "( capsfilter caps=\"application/x-rtp, media=audio, payload=0, clock-rate=8000, encoding-name=PCMU\" name=depay_backchannel ! decodebin async-handling=false message-forward=true !  pulsesink async=false )"); //Works but randomly break after a few stream switch
    
    // Works flawlessly
    // "( capsfilter caps=\"application/x-rtp, media=audio, payload=0, clock-rate=8000, encoding-name=PCMU\" name=depay_backchannel ! rtppcmudepay ! mulawdec ! pulsesink async=false )");

    // autoaudiosink sync property doesnt seem to work. Sample queues up in appsrc.
    // "( capsfilter caps=\"application/x-rtp, media=audio, payload=0, clock-rate=8000, encoding-name=PCMU\" name=depay_backchannel ! rtppcmudepay ! mulawdec ! autoaudiosink sync=true )"); 
    
    GST_DEBUG("Backchannel : %s\n",backchannel_lauch);
    
    factory = ext_rtsp_onvif_media_factory_new ();
    
    //TODO handle format
    ext_rtsp_onvif_media_factory_set_video_device(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.vdev);
    ext_rtsp_onvif_media_factory_set_audio_device(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.adev);
    ext_rtsp_onvif_media_factory_set_video_encoder(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.encoder);
    ext_rtsp_onvif_media_factory_set_width(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),(gint *)arguments.width);
    ext_rtsp_onvif_media_factory_set_height(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),(gint *)arguments.height);
    ext_rtsp_onvif_media_factory_set_fps(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),(gint *)arguments.fps);

    gst_rtsp_onvif_media_factory_set_backchannel_launch(GST_RTSP_ONVIF_MEDIA_FACTORY (factory),backchannel_lauch);

    gst_rtsp_media_factory_set_shared (factory, FALSE);
    gst_rtsp_media_factory_set_media_gtype (factory, GST_TYPE_RTSP_ONVIF_MEDIA);


    char * mount = malloc(strlen(arguments.mount)+2);
    strcpy(mount,"/");
    strcat(mount,arguments.mount);
    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory (mounts, mount, factory);

    char *serviceaddr = "0.0.0.0";
    char serviceport[5]; //Max port number is 5 character long (65,535)
    itoa(arguments.port, serviceport, 10);

    /* don't need the ref to the mapper anymore */
    g_object_unref (mounts);
    gst_rtsp_server_set_address(server,serviceaddr);
    gst_rtsp_server_set_service(server,serviceport);
    /* attach the server to the default maincontext */
    gst_rtsp_server_attach (server, NULL);
    
    /* start serving */
    GST_DEBUG ("stream ready at rtsp://%s:%s/%s\n",serviceaddr,serviceport,arguments.mount);
    g_main_loop_run (loop);

    return 0;
}
