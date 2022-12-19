#include <string.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>

#include <sys/ioctl.h> 
#include <errno.h>

#include "v4l2_match_result.h"
#include "v4l2-device.h"
#include <gst/gstinfo.h>

GST_DEBUG_CATEGORY_STATIC (ext_v4l2_debug);
#define GST_CAT_DEFAULT (ext_v4l2_debug)

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

int SetVideoFMT(int fd, v4l2MatchResult * match)
{
    GST_DEBUG("---------- Setting Device FMT [%d/%d] ----------", match->width,match->height);
    /* set framerate */
    int i;
    struct v4l2_format fmt;
    memset(&fmt,0,sizeof(fmt));
    char fourcc[5] = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = match->width;
    fmt.fmt.pix.height = match->height;
    fmt.fmt.pix.pixelformat = match->pixel_format;
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        perror("Setting Pixel Format");
    }else{
        strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
        GST_DEBUG( "\nSelected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fourcc,
            fmt.fmt.pix.field);
    }
    GST_DEBUG("---------- Done Setting Device FMT ----------");
	return 0;
}

void setFramerate(int fd, unsigned int denominator, unsigned int numerator){
    GST_DEBUG("---------- Setting Device Framerate ----------");
    /* set framerate */
    uint32_t fps = denominator / numerator;
    struct v4l2_streamparm parm;
    memset(&parm,0,sizeof(parm));
    parm.parm.capture.timeperframe.denominator = denominator;
    parm.parm.capture.timeperframe.numerator = numerator;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 > xioctl(fd, VIDIOC_S_PARM, &parm)){
            perror("VIDIOC_S_PARM");
    }
    if(parm.parm.capture.timeperframe.numerator){
        double fps_new = parm.parm.capture.timeperframe.denominator
                    / parm.parm.capture.timeperframe.numerator;
        if ((double)fps != fps_new) {
            GST_DEBUG("unsupported frame rate [%d,%f]", fps, fps_new);
            return;
        }else{
            GST_DEBUG("\n  Framerate : %u/%u", parm.parm.capture.timeperframe.denominator,
            parm.parm.capture.timeperframe.numerator);
        }
    } else {
        GST_DEBUG ("Error : Unexpected numerator value");
    }

    GST_DEBUG("---------- Doen Setting Framerate ----------");
}

v4l2MatchResult * create_v4l2_result(MatchTypes type, struct v4l2_frmivalenum frmival){
    v4l2MatchResult * ret = malloc(sizeof(v4l2MatchResult));
    ret->height = (unsigned int) frmival.height;
    ret->width = (unsigned int) frmival.width;
    ret->pixel_format = (unsigned int) frmival.pixel_format;
    ret->denominator = (unsigned int) frmival.discrete.denominator;
    ret->numerator = (unsigned int) frmival.discrete.numerator;

    return ret;
}

void find_compatible_format_frame_interval(unsigned int fd, struct v4l2_frmsizeenum frmsize, unsigned int width, unsigned int height, v4l2ParameterInput desires, v4l2MatchResults * ret)
{
    struct v4l2_frmivalenum frmival;
    memset(&frmival,0,sizeof(frmival));
    frmival.pixel_format = frmsize.pixel_format;
    frmival.width = width;
    frmival.height = height;
    while (xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0)
    {   
        if(desires.desired_fps > 1.0*frmival.discrete.denominator/frmival.discrete.numerator){
            //Discard increasing FPS 
            // GST_DEBUG("inserting frames...");
        } else if (desires.desired_width > width ||
            desires.desired_height > height){
            //Discard scaling up
            // GST_DEBUG("Scaling up");
        } else if(desires.desired_pixelformat == frmsize.pixel_format) {
            if(desires.desired_width == width &&
                desires.desired_height == height &&
                desires.desired_fps == 1.0*frmival.discrete.denominator/frmival.discrete.numerator){
                GST_DEBUG("PERFECT MATCH");
                v4l2MatchResult * vret = create_v4l2_result(PERFECT,frmival);
                ret->p_match = vret;
            } else if(desires.desired_width == width &&
                desires.desired_height == height &&
                desires.desired_fps <= 1.0*frmival.discrete.denominator/frmival.discrete.numerator){
                v4l2MatchResult * vret = create_v4l2_result(GOOD,frmival);
                GST_DEBUG("GOOD MATCH"); //Drop frames
                v4l2MatchResult__insert_element(ret,vret,0);
            }else if(desires.desired_width <= width &&
                desires.desired_height <= height &&
                desires.desired_fps == 1.0*frmival.discrete.denominator/frmival.discrete.numerator){
                v4l2MatchResult * vret = create_v4l2_result(OK,frmival);
                GST_DEBUG("OK MATCH"); //Scale down
                v4l2MatchResult__insert_element(ret,vret,0);
            } else if(desires.desired_width <= width &&
                desires.desired_height <= height &&
                desires.desired_fps <= 1.0*frmival.discrete.denominator/frmival.discrete.numerator){
                v4l2MatchResult * vret = create_v4l2_result(BAD,frmival);
                GST_DEBUG("BAD MATCH"); //Drop frames and Scale down
                v4l2MatchResult__insert_element(ret,vret,0);
            } else {
                GST_DEBUG("Unexpected outcome-------");
                //Scaling up resolution uselessly will increase bandwidth for no good reason
            }
        } else {
            //TODO handle fallback where decoding/encoding would be required.
        }

        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) 
            GST_DEBUG("[%d/%d-%d]xx[%dx%d] %f fps", desires.desired_width, desires.desired_height, desires.desired_fps, (int)width, height, 1.0*frmival.discrete.denominator/frmival.discrete.numerator);
        else
            GST_DEBUG("[%d/%d-%d]xx[%dx%d] [%f,%f] fps", desires.desired_width, desires.desired_height, desires.desired_fps, (int)width, height, 1.0*frmival.stepwise.max.denominator/frmival.stepwise.max.numerator, 1.0*frmival.stepwise.min.denominator/frmival.stepwise.min.numerator);
        frmival.index++;    
    }
}

void find_compatible_format_frame_size(int fd, struct v4l2_fmtdesc fmtdesc, v4l2ParameterInput desires, v4l2MatchResults * ret){
    unsigned int width=0, height=0;;
    struct v4l2_frmsizeenum frmsize;
    memset(&frmsize,0,sizeof(frmsize));
    frmsize.pixel_format = fmtdesc.pixelformat;//V4L2_PIX_FMT_MJPEG;
    while (xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0)
    {
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) 
        {
            //frmsize.pixel_format
            find_compatible_format_frame_interval(fd, frmsize, frmsize.discrete.width, frmsize.discrete.height, desires, ret);
        }
        else
        {
            for (width=frmsize.stepwise.min_width; width< frmsize.stepwise.max_width; width+=frmsize.stepwise.step_width)
                for (height=frmsize.stepwise.min_height; height< frmsize.stepwise.max_height; height+=frmsize.stepwise.step_height)
                    find_compatible_format_frame_interval(fd, frmsize, width, height, desires, ret);

        }
        frmsize.index++;
    }
}

void find_compatible_format(int fd, v4l2MatchResults * ret, v4l2ParameterInput desires){

    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc,0,sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (xioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) == 0)
    {    
        GST_DEBUG("---- %s ----", fmtdesc.description);
        find_compatible_format_frame_size(fd,fmtdesc,desires, ret);
        fmtdesc.index++;
    }
}

void set_v4l2_match(int fd, v4l2MatchResult * match){
    SetVideoFMT(fd, match);
    setFramerate(fd, match->denominator,match->numerator);
}

v4l2ParameterResults * create_return_result(v4l2MatchResult * match){
    v4l2ParameterResults * ret_val = malloc(sizeof(v4l2ParameterResults));
    ret_val->device_width = match->width;
    ret_val->device_height= match->height;
    ret_val->device_numerator = match->numerator;
    ret_val->device_denominator = match->denominator;
    ret_val->device_pixelformat = match->pixel_format;
    return ret_val;
}

v4l2ParameterResults * configure_v4l2_device(char * device, v4l2ParameterInput desires, MatchScope scope)
{   
    GST_DEBUG_CATEGORY_INIT (ext_v4l2_debug, "ext-v4l2", 0, "Extension to support v4l capabilities");
    gst_debug_set_threshold_for_name ("ext-v4l2", GST_LEVEL_LOG);
    
    v4l2ParameterResults * ret_val = NULL;
    v4l2MatchResults * results = v4l2MatchResult__create();
    int fd = v4l2_open(device, O_RDWR);
    if (fd != -1)
    {   
        
        find_compatible_format(fd, results, desires);
        if(results->p_match != NULL){
            GST_DEBUG("Found perfect match");
            set_v4l2_match(fd, results->p_match);
            ret_val = create_return_result(results->p_match);
        } else if(scope >= GOOD_MATCH && results->good_matches_count > 0){
            GST_DEBUG("Found good match(s)");
            set_v4l2_match(fd, results->good_matches[0]);
            //TODO Find match with closest framerate to drop as least as possible
            ret_val = create_return_result(results->good_matches[0]);
        } else if(scope >= OK_MATCH && results->ok_matches_count > 0){
            GST_DEBUG("Found ok match(s)");
            set_v4l2_match(fd, results->ok_matches[0]);
            //TODO Find match with closest resolution to minimize scale down
            ret_val = create_return_result(results->ok_matches[0]);
        } else if(scope >= BAD_MATCH && results->bad_matches_count > 0){
            GST_DEBUG("Found bad match(s)");
            set_v4l2_match(fd, results->bad_matches[0]);
            //TODO Find match with closest resolution to minimize scale down over frame drops.
            //It takes less computing to drop frame and has lesser potential impact on image quality
            ret_val = create_return_result(results->bad_matches[0]);
        } else {
            GST_DEBUG("No bad match");
        }
        v4l2_close(fd);

    }

    return ret_val;
}