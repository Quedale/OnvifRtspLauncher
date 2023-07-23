#ifndef V4L2_DEV_H_ 
#define V4L2_DEV_H_

#include <linux/videodev2.h>

typedef enum { 
    V4L2_FMT_YUYV = V4L2_PIX_FMT_YUYV //TODO detect fourcc to support YUV raw videos
} SupportedRawFormats;

typedef enum { 
    V4L2_FMT_H264 = V4L2_PIX_FMT_H264,
    V4L2_FMT_HEVC = V4L2_PIX_FMT_HEVC,
    V4L2_FMT_MJPEG = V4L2_PIX_FMT_MJPEG
} SupportedPixelFormats;

typedef enum {
    PERFECT_MATCH       = 1,
    GOOD_MATCH          = 2,
    OK_MATCH            = 3,
    BAD_MATCH           = 4,
    RAW_PERFECT_MATCH   = 5,
    RAW_GOOD_MATCH      = 6,
    RAW_OK_MATCH        = 7,
    RAW_BAD_MATCH       = 8,
    ANY_MATCH           = 5
} MatchScope;

typedef struct {
    unsigned int desired_width;
    unsigned int desired_height;
    unsigned int desired_fps;
    SupportedPixelFormats desired_pixelformat;
} v4l2ParameterInput;

typedef struct {
    unsigned int device_width;
    unsigned int device_height;
    unsigned int device_denominator;
    unsigned int device_numerator;
    SupportedPixelFormats device_pixelformat;
    v4l2ParameterInput desired_parameters;
} v4l2ParameterResults;

v4l2ParameterResults *  configure_v4l2_device(char * device, v4l2ParameterInput desires, MatchScope scope);

#endif