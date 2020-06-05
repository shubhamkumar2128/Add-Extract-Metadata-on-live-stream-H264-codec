//compile - gcc SendMetadataH264.c -o snd `pkg-config --cflags --libs gstreamer-1.0`
//run - ./snd

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <gst/gst.h>
#include<string.h>

struct timestamp 
{
	guint64 cameraId;
	struct timeval tv;
}ts;

gsize buf_size;  
GstBuffer *buffer;
GstMapInfo mapinfo;
guint8 *ptr;
GstMemory *memory;
long *cameraId, *seconds, *usec;  

static GstPadProbeReturn set_tag_cb(GstPad *srcpad, GstPadProbeInfo *info, gpointer user_data)
{
  gettimeofday(&ts.tv, NULL);
  ts.cameraId = 100;  
  buffer = GST_PAD_PROBE_INFO_BUFFER (info);
  buf_size = gst_buffer_get_size (buffer);
  g_print("size : %ld\n", buf_size);
  buffer = gst_buffer_make_writable (buffer);
  memory = gst_allocator_alloc (NULL, 24, NULL);
  gst_buffer_insert_memory (buffer, -1, memory);
  gst_buffer_map (buffer, &mapinfo, GST_MAP_WRITE);
  ptr = (guint8 *) mapinfo.data;
  ptr += buf_size; 
  memcpy(ptr, &ts, 24);
  ptr = (guint8 *) mapinfo.data;
  ptr += buf_size; 
  cameraId = (long *)ptr;
  seconds = (long *) ptr+1;  
  usec = (long *) ptr+2;  
  g_print("Camera ID : %lu\n", *cameraId);
  g_print("Seconds : %lu\t", ts.tv.tv_sec);
  g_print("Seconds : %lu\n", *seconds);
  g_print("Microseconds : %lu\t", ts.tv.tv_usec);
  g_print("Microseconds : %lu\n", *usec);
  gst_buffer_unmap (buffer, &mapinfo);
  GST_PAD_PROBE_INFO_DATA (info) = buffer;
  return GST_PAD_PROBE_OK;

}

gint main (gint argc, gchar *argv[])
{

  GMainLoop *loop;
  GstElement *pipeline, *src, *filter, *videoenc, *videoparse, *pl, *netsink;
  GstCaps *filtercaps;
  GstPad *pad;
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);
  pipeline = gst_pipeline_new ("send_pipeline");
  
  src = gst_element_factory_make ("videotestsrc", "src");
  if (src == NULL)
    g_error ("Could not create 'videotestsrc' element");

  filter = gst_element_factory_make ("capsfilter", "filter");
  g_assert (filter != NULL); 
  
  videoenc = gst_element_factory_make ("x264enc", "videoenc");
  if (videoenc == NULL)
    g_error ("Could not create 'x264enc' element");

  videoparse = gst_element_factory_make ("h264parse", "videoparse");
  if (videoparse == NULL) 
      g_error ("Could not create 'h264parse' element");

  pl = gst_element_factory_make ("rtpgstpay", "pl");
  if (pl == NULL) 
      g_error ("Could not create 'rtpgstpay' element");

  netsink = gst_element_factory_make ("udpsink", "netsink");
  if (netsink == NULL) 
      g_error ("Could not create 'udpsink' element");

  gst_bin_add_many (GST_BIN (pipeline), src, filter, videoenc, videoparse, pl, netsink, NULL);
  gst_element_link_many (src, filter, videoenc, videoparse, pl, netsink, NULL);
  filtercaps = gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, 800, "height", G_TYPE_INT, 600, NULL);
  g_object_set (G_OBJECT (filter), "caps", filtercaps, NULL);
  gst_caps_unref (filtercaps);
  pad = gst_element_get_static_pad (videoparse, "src");
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) set_tag_cb, NULL, NULL);
  gst_object_unref(pad);
  g_object_set (G_OBJECT (netsink), "host", "127.0.0.1", "port", 5009, NULL);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  
  if (gst_element_get_state (pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
    g_error ("Failed to go into PLAYING state");
  }

  g_print ("Running ...\n");
  g_main_loop_run (loop);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  return 0;

}



