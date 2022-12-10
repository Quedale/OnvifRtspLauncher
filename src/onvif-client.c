#include <gst/gst.h>
#include "onvifinitstaticplugins.h"
// #include "onvifinitstaticserverplugins.h"
/*
    THIS IS ONLY MEANT AS A TESTING TOOL
*/
static GMainLoop *loop = NULL;
static GstElement *backpipe = NULL;
static gint stream_id = -1;

#define PCMU_CAPS "application/x-rtp, media=audio, payload=0, clock-rate=8000, encoding-name=PCMU"

static GstFlowReturn
new_sample (GstElement * appsink, GstElement * rtspsrc)
{
  GstSample *sample;
  GstFlowReturn ret = GST_FLOW_OK;

  g_assert (stream_id != -1);

  /* get the sample from appsink */
  g_signal_emit_by_name (appsink, "pull-sample", &sample);

  if (!sample)
    goto nosample;

  g_signal_emit_by_name (rtspsrc, "push-backchannel-sample", stream_id, sample,
      &ret);

  //I know this was added in version 1.21, but it generates //gst_mini_object_unlock: assertion 'state >= SHARE_ONE' failed
  /* Action signal callbacks don't take ownership of the arguments passed, so we must unref the sample here.
   * (The "push-backchannel-buffer" callback unrefs the sample, which is wrong and doesn't work with bindings
   * but could not be changed, hence the new "push-backchannel-sample" callback that behaves correctly.)  */
  // gst_sample_unref (sample);

nosample:
  return ret;
}

static void
setup_backchannel_shoveler (GstElement * rtspsrc, GstCaps * caps)
{
  GstElement *appsink;

  backpipe = gst_parse_launch ("audiotestsrc is-live=true wave=red-noise ! "
      "mulawenc ! rtppcmupay ! appsink name=out", NULL);
  if (!backpipe)
    g_error ("Could not setup backchannel pipeline");

  appsink = gst_bin_get_by_name (GST_BIN (backpipe), "out");
  g_object_set (G_OBJECT (appsink), "caps", caps, "emit-signals", TRUE, NULL);

  g_signal_connect (appsink, "new-sample", G_CALLBACK (new_sample), rtspsrc);

  g_print ("Playing backchannel shoveler\n");
  gst_element_set_state (backpipe, GST_STATE_PLAYING);
}

static gboolean
remove_extra_fields (GQuark field_id, GValue * value G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED)
{
  return !g_str_has_prefix (g_quark_to_string (field_id), "a-");
}

static gboolean
find_backchannel (GstElement * rtspsrc, guint idx, GstCaps * caps,
    gpointer user_data G_GNUC_UNUSED)
{
  GstStructure *s;
  gchar *caps_str = gst_caps_to_string (caps);
  g_print ("Selecting stream idx %u, caps %s\n", idx, caps_str);
  g_free (caps_str);

  s = gst_caps_get_structure (caps, 0);
  if (gst_structure_has_field (s, "a-sendonly")) {
    stream_id = idx;
    caps = gst_caps_new_empty ();
    s = gst_structure_copy (s);
    gst_structure_set_name (s, "application/x-rtp");
    gst_structure_filter_and_map_in_place (s, remove_extra_fields, NULL);
    gst_caps_append_structure (caps, s);
    setup_backchannel_shoveler (rtspsrc, caps);
  }

  return TRUE;
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *rtspsrc;
  const gchar *location;

  gst_init (&argc, &argv);
  
  onvif_init_static_plugins();
  if (argc >= 2)
    location = argv[1];
  else
    location = "rtsp://127.0.0.1:8554/test";

  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_parse_launch ("rtspsrc backchannel=onvif onvif-mode=true debug=true name=r "
      "r. ! queue ! decodebin ! queue ! xvimagesink async=false "
      "r. ! queue ! decodebin ! queue ! pulsesink async=false ", NULL);
  if (!pipeline)
    g_error ("Failed to parse pipeline");

  rtspsrc = gst_bin_get_by_name (GST_BIN (pipeline), "r");
  g_object_set (G_OBJECT (rtspsrc), "location", location, NULL);
  g_signal_connect (rtspsrc, "select-stream", G_CALLBACK (find_backchannel),
      NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_main_loop_run (loop);

  gst_element_set_state (backpipe, GST_STATE_NULL);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  
  return 0;
}
