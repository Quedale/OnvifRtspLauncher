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
#include <gst/pbutils/gstpluginsbaseversion.h>

#include <string.h>

#include <argp.h>
#include <stdbool.h>
#include <stdio.h>

#include "onvifinitstaticplugins.h"
#include "sink-retriever.h"
#include "vencoder-retriever.h"
#include "gst/ext_rtsp_onvif_media_factory.h"
#include "v4l/v4l2-device.h"
#include "onvif-server-snapshot.h"

GST_DEBUG_CATEGORY_STATIC (ext_onvif_server_debug);
#define GST_CAT_DEFAULT (ext_onvif_server_debug)


const char *argp_program_version = "0.0";
const char *argp_program_bug_address = "<your@email.address>";
static char doc[] = "Your program description.";
static struct argp_option options[] = { 
    { "video", 'v', "VIDEO", 0, "Input video device. (Default: /dev/video0)"},
    { "audio", 'a', "AUDIO", 0, "ALSA Input audio device. (Default: auto)"},
    { "encoder", 'e', "ENCODER", 0, "Gstreamer encoder. (Default: auto)"},
    { "width", 'w', "WIDTH", 0, "Video output width. (Default: 640)"},
    { "height", 'h', "HEIGHT", 0, "Video output height. (Default: 480)"},
    { "fps", 'f', "FPS", 0, "Video output framerate. (Default: 10)"},
    // { "format", 't', "FORMAT", 0, "Video input format. (Default: YUY2)"},
    { "mount", 'm', "MOUNT", 0, "URL mount point. (Default: h264)"},
    { "port", 'p', "PORT", 0, "Network port to listen. (Default: 8554)"},
    { "snapshot", 's', "RTSPURL", 0, "Overrides all other input and takes a snapshot of the provided rtsp url."},
    { "output", 'o', "PNGFILE", 0, "Overrides all other input and set the snapshot output. (Default: output.png)"},
    { "codec", 'c', "CODEC", 0, "Rtsp Output Codec used.(h264 or h265. Default h264)"},
    { "user", 'u', "USER", 0, "User account allowed to access it.(Default: anonymous)"},
    { "password", 'z', "PASSWORD", 0, "Password user to authenticate user.(Default: anonymous)"},
    { 0 } 
};

//Define argument struct
struct arguments {
    char *vdev;
    char *adev;
    char *snapshot;
    char * output;
    int width;
    int height;
    int fps;
    char *format;
    char *encoder;
    char *mount;
    int port;
    char * codec;
    char * user;
    char * pass;
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
    case 's': arguments->snapshot = arg; break;
    case 'o': arguments->output = arg; break;
    case 'c': arguments->codec = arg; break;
    case 'u': arguments->user = arg; break;
    case 'z': arguments->pass = arg; break;
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

    gst_init (&argc, &argv);

#ifdef STATIC_BUILD
    onvif_init_static_plugins();
#endif

    GST_DEBUG_CATEGORY_INIT (ext_onvif_server_debug, "ext-onvif-server", 0, "Extended ONVIF server");
    gst_debug_set_threshold_for_name ("ext-onvif-server", GST_LEVEL_LOG);
    GST_DEBUG("Using Gstreamer Version : %i.%i.%i.%i\n",GST_PLUGINS_BASE_VERSION_MAJOR,GST_PLUGINS_BASE_VERSION_MINOR,GST_PLUGINS_BASE_VERSION_MICRO,GST_PLUGINS_BASE_VERSION_NANO);
  
    arguments.vdev = "/dev/video0";
    arguments.adev = NULL;
    arguments.width = 640;
    arguments.height = 480;
    arguments.fps = 10;
    arguments.format = "YUY2";
    arguments.encoder = NULL;
    arguments.mount = "h264";
    arguments.port = 8554;
    arguments.snapshot = NULL;
    arguments.output = "output.png";
    arguments.codec = "h264";
    arguments.user = NULL;
    arguments.pass = NULL;

    /* Default values. */

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(arguments.snapshot){
        GST_INFO("Taking snapshot of '%s'",arguments.snapshot);
        take_snapshot(arguments.snapshot, arguments.output);
        return 0;
    }

    if(!arguments.encoder){
        arguments.encoder = retrieve_videoencoder(arguments.codec);
        if(!arguments.encoder){
            GST_ERROR("No functional video encoder found.");
            return 1;
        }
    } else {
        GST_WARNING("Video Encoder override '%s'",arguments.encoder);
    }

     /* Retrieve appropriate audio source. (pulse vs alsa) and try list of alsa recording devices */
    char mic_element[13], mic_device[6];
    if(!arguments.adev || arguments.adev[0] == '\0'){
        GST_WARNING("Retreiving Audio Source Device ...'");
        retrieve_audiosrc(mic_element,mic_device);
        arguments.adev = &mic_device[0];
    } else {
        strcpy(mic_element,arguments.adev);
        GST_WARNING("Audio Source Device override '%s'",arguments.adev);
    }

    /* Retrieve appropriate audio source. (pulse vs alsa) and try list of alsa recording devices */
    char audiosink[35];
    retrieve_audiosink(audiosink);

    GST_DEBUG ("vdev : %s", arguments.vdev);
    GST_DEBUG ("adev : %s", arguments.adev);
    GST_DEBUG ("width : %i", arguments.width);
    GST_DEBUG ("height : %i", arguments.height);
    GST_DEBUG ("format : %s", arguments.format);
    GST_DEBUG ("encoder : %s", arguments.encoder);
    GST_DEBUG ("mount : %s", arguments.mount);
    GST_DEBUG ("port : %i", arguments.port);
    GST_DEBUG ("fps : %i", arguments.fps);
    GST_DEBUG ("mic src element : %s",mic_element);
    GST_DEBUG ("mic sink element : %s",audiosink);
    loop = g_main_loop_new (NULL, FALSE);
    /* create a server instance */
    server = gst_rtsp_onvif_server_new ();

    /* get the mount points for this server, every server has a default object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server);
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
    
    GST_DEBUG("Backchannel : %s",backchannel_lauch);
    factory = ext_rtsp_onvif_media_factory_new ();

    if(arguments.user && arguments.pass && strcmp(arguments.user,"") && strcmp(arguments.pass,"")){
        GST_DEBUG("Setting authentication for user : %s", arguments.user);
        gst_rtsp_media_factory_add_role (factory, "admin",
            GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
            GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);

        /* make a new authentication manager */
        GstRTSPAuth * auth = gst_rtsp_auth_new ();
        gst_rtsp_auth_set_supported_methods (auth, GST_RTSP_AUTH_DIGEST);

        /* make admin token */
        GstRTSPToken * token = gst_rtsp_token_new (GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "admin", NULL);
        gst_rtsp_auth_add_digest (auth, arguments.user, arguments.pass, token);
        gst_rtsp_token_unref (token);

        /* set as the server authentication manager */
        gst_rtsp_server_set_auth (server, auth);
        g_object_unref (auth);
    } else {
        GST_WARNING("RTSP Stream is accessible anonymously!!");
    }

    //Easy way to override custom pipeline creation in favor or legacy launch string.
    // char * launch = "videotestsrc is-live=true ! video/x-raw, format=YUY2, width=640, height=480, framerate=10/1 ! videoconvert !  v4l2h264enc extra-controls=\"encode,video_bitrate=25000000\" ! video/x-h264,profile=(string)high,level=(string)4,framerate=(fraction)10/1,width=640, height=480 ! h264parse ! rtph264pay name=pay0 pt=96";
    // printf("launch : %s\n",launch);
    // gst_rtsp_media_factory_set_launch (GST_RTSP_ONVIF_MEDIA_FACTORY(factory),launch);

    //TODO handle format
    if(strcmp(arguments.codec,"h265") ==0){
        ext_rtsp_onvif_media_factory_set_codec(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),EXT_RTSP_CODEC_H265);
    } else if(strcmp(arguments.codec,"mjpeg") == 0) {
        ext_rtsp_onvif_media_factory_set_codec(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),EXT_RTSP_CODEC_MJPEG);
    } else {
        ext_rtsp_onvif_media_factory_set_codec(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),EXT_RTSP_CODEC_H264);
    }

    //Set v4l2Parameter or quit if not found
    if(strcmp(arguments.vdev,"test")){
        v4l2ParameterInput desires;
        desires.desired_fps = arguments.fps;
        desires.desired_width = arguments.width;
        desires.desired_height = arguments.height;
        desires.desired_pixelformat = ext_rtsp_onvif_media_factory_get_codec(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory));
        v4l2ParameterResults * ret_val = configure_v4l2_device(arguments.vdev, desires, RAW_BAD_MATCH);
        if(ret_val == NULL){
            g_printerr ("Unable to configure v4l2 source device...\n");
            return 1;
        }
    
        ext_rtsp_onvif_media_factory_set_v4l2_params(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),ret_val);
    }    

    ext_rtsp_onvif_media_factory_set_video_device(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.vdev);
    ext_rtsp_onvif_media_factory_set_microphone_device(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.adev);
    ext_rtsp_onvif_media_factory_set_microphone_element(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),mic_element);
    ext_rtsp_onvif_media_factory_set_video_encoder(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.encoder);
    ext_rtsp_onvif_media_factory_set_width(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.width);
    ext_rtsp_onvif_media_factory_set_height(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.height);
    ext_rtsp_onvif_media_factory_set_fps(EXT_RTSP_ONVIF_MEDIA_FACTORY(factory),arguments.fps);

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
    GST_DEBUG ("stream ready at rtsp://%s:%s/%s",serviceaddr,serviceport,arguments.mount);
    g_main_loop_run (loop);

    return 0;
}
