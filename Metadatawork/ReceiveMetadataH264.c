//compile - gcc ReceiveMetadataH264.c -o rec `pkg-config --cflags --libs gstreamer-1.0`
//run - ./rec

#include <gst/gst.h>
#include <string.h>
struct metadatainfo
{
	guint64 cameraId;
	guint64 seconds;
	guint64 usecs; 
}mdi;

GstBuffer *buffer;
guint8 *ptr;
gsize buf_size;
GstMapInfo mapinfo;

static GstPadProbeReturn get_gstbuf_cb(GstPad *srcpad, GstPadProbeInfo *info, gpointer user_data)
{
  buffer = GST_PAD_PROBE_INFO_BUFFER (info);
  buf_size = gst_buffer_get_size (buffer);
  g_print("size : %ld\n", buf_size); 
  gst_buffer_map (buffer, &mapinfo, GST_MAP_READ);
  ptr = (guint8 *) mapinfo.data;
  ptr += buf_size-24;     
  memcpy(&mdi, ptr, 24);
  g_print("Camera ID : %lu\n", mdi.cameraId);
  g_print("Seconds : %lu\n", mdi.seconds);
  g_print("Microseconds : %lu\n", mdi.usecs);
  g_print("***********************************\n");
  gst_buffer_unmap (buffer, &mapinfo);
  return GST_PAD_PROBE_OK;
}

gint main (gint argc, gchar *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline, *netrcv, *filter, *depay, *h264decode, *vc,  *vs;
  GstCaps *filtercaps;
  GstPad *pad;
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);
  pipeline = gst_pipeline_new ("receive_pipeline");
  
  netrcv = gst_element_factory_make ("udpsrc", "netrcv");
  if (netrcv == NULL)
    g_error ("Could not create 'udpsrc' element");

  filter = gst_element_factory_make ("capsfilter", "filter");
  g_assert (filter != NULL); 

  depay = gst_element_factory_make ("rtpgstdepay", "depay");
  if (depay == NULL)
    g_error ("Could not create 'rtpgstdepay' element");

  h264decode = gst_element_factory_make ("avdec_h264", "filesink");
  if (h264decode == NULL) 
      g_error ("Could not create 'avdec_h264' element");

  vc = gst_element_factory_make ("videoconvert", "vc");
  if (vc == NULL) 
      g_error ("Could not create 'videoconvert' element");

  vs = gst_element_factory_make ("autovideosink", "vs");
  if (vs == NULL) 
      g_error ("Could not create 'autovideosink' element");

  gst_bin_add_many (GST_BIN (pipeline), netrcv, filter, depay, h264decode, vc, vs, NULL);
  gst_element_link_many (netrcv, filter, depay, h264decode, vc, vs, NULL);

  filtercaps = gst_caps_new_simple ("application/x-rtp",
 "media", G_TYPE_STRING, "application",
 "clock-rate", G_TYPE_INT, 90000,
 "encoding-name", G_TYPE_STRING, "X-GST",
 "capsversion", G_TYPE_STRING, "0",
 "payload", G_TYPE_INT, 96, 
  NULL);

  g_object_set (G_OBJECT (filter), "caps", filtercaps, NULL);
  gst_caps_unref (filtercaps);
  g_object_set (G_OBJECT (netrcv), "port", 5009, NULL);
  pad = gst_element_get_static_pad (h264decode, "sink");
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) get_gstbuf_cb, NULL, NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  
  if (gst_element_get_state (pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
    g_error ("Failed to go into PLAYING state");

  }

  loop = g_main_loop_new (NULL, FALSE);
  g_print ("Running ...\n");
  g_main_loop_run (loop);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  return 0;

}



