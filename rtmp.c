#include <string.h>
#include <gst/gst.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//gst-launch-1.0 -v v4l2src device=/dev/video0 ! 'video/x-h264, width=640, height=360, framerate=30/1' ! queue !  h264parse ! flvmux ! rtmpsink location='rtmp://192.168.1.102/live'

typedef struct _GstDataStruct
{
	GstElement *pipeline;
	GstElement *v4l2src;
	GstElement *queue;
	GstElement *h264parse;
	GstElement *flvmux;
	GstElement *rtmpsink;
	GstBus *bus;
	guint bus_watch_id;
	guint sourceid;        /* To control the GSource */
	GMainLoop *loop;  /* GLib's Main Loop */
} GstDataStruct;

static GstDataStruct GstData;
static unsigned int frame_width;
static unsigned int frame_height;
static unsigned int frame_rate;
static unsigned int frame_bps;
static char devname[32] = {0};

gboolean bus_msg_call(GstBus *bus, GstMessage *msg, GstDataStruct *pGstData)
{
	gchar *debug;
	GError *error;
	GMainLoop *loop = pGstData->loop;

	GST_DEBUG ("got message %s",gst_message_type_get_name (GST_MESSAGE_TYPE (msg)));
	switch (GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_EOS:
			printf("End of stream\n");
			g_main_loop_quit(loop);
			break;
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &error, &debug);
			g_free(debug);
			g_printerr("Error: %s\n", error->message);
			g_error_free(error);
			g_main_loop_quit(loop);
			break;
		default:
			break;
	}
	return TRUE;
}

int main(int argc, char *argv[])
{
	if(argc != 6)
	{
		frame_width = 1280;
		frame_height = 720;
		frame_rate = 30;
		frame_bps = 1500000;
		sprintf(devname, "%s", "/dev/video0");
	}
	else
	{
		frame_width = atoi(argv[2]);
		frame_height = atoi(argv[3]);
		frame_rate = atoi(argv[4]);
		frame_bps = atoi(argv[5]);
		sprintf(devname, "%s", argv[1]);
	}
	printf("width:%d, height:%d, rate:%d, bps:%d, dev:%s\n", frame_width, frame_height, frame_rate, frame_bps, devname);

	printf("============= v4l2 rtmp gst init start ============\n");
	gst_init (NULL, NULL);
	printf("=========== create v4l2 rtmp pipeline =============\n");
	GstData.pipeline           	= gst_pipeline_new ("v4l2_rtmp");
	GstData.pipeline           	= gst_pipeline_new ("v4l2_rtmp");
	GstData.v4l2src        	   	= gst_element_factory_make ("v4l2src",      "v4l2src");
	GstData.queue      		   	= gst_element_factory_make ("queue",  		"queue");
	GstData.h264parse      	   	= gst_element_factory_make ("h264parse",	"h264parse");
	GstData.flvmux           	= gst_element_factory_make ("flvmux",      	"flvmux");
	GstData.rtmpsink            = gst_element_factory_make ("rtmpsink",     "rtmpsink");

	if (!GstData.pipeline || !GstData.v4l2src || !GstData.queue ||
		!GstData.h264parse || !GstData.flvmux || !GstData.rtmpsink)
	{
		g_printerr ("One element could not be created... Exit\n");
		return -1;
	}

	printf("============ link v4l2 rtmp pipeline ==============\n");
	GstCaps *caps_v4l2src;
	caps_v4l2src = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING,"byte-stream",
									   "alignment", G_TYPE_STRING, "au",
									   "width", G_TYPE_INT, frame_width,
									   "height", G_TYPE_INT, frame_height,
									   "framerate",GST_TYPE_FRACTION, frame_rate, 1, NULL);
	GstCaps *caps_flv_sink;
	caps_flv_sink = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING,"avc",
									    "alignment", G_TYPE_STRING, "au",
									    "width", G_TYPE_INT, frame_width,
									    "height", G_TYPE_INT, frame_height,
									    "framerate",GST_TYPE_FRACTION, frame_rate, 1, NULL);

	g_object_set(G_OBJECT(GstData.v4l2src), "device", devname, NULL);
	g_object_set(G_OBJECT(GstData.rtmpsink), "location", "rtmp://192.168.1.102/live", NULL);
//注意：此处的location参数代表rtmp的url，其取值必须与html文件的rtmp的URL保持一致，才可观看视频。
	GstData.bus = gst_pipeline_get_bus(GST_PIPELINE(GstData.pipeline));
	GstData.bus_watch_id = gst_bus_add_watch(GstData.bus, (GstBusFunc)bus_msg_call, (gpointer)&GstData);
	gst_object_unref(GstData.bus);

	gst_bin_add_many(GST_BIN(GstData.pipeline), GstData.v4l2src, GstData.queue,
					 GstData.h264parse, GstData.flvmux, GstData.rtmpsink,NULL);

	if(gst_element_link_filtered(GstData.v4l2src, GstData.queue, caps_v4l2src) != TRUE)
	{
		g_printerr ("GstData.v4l2src could not link GstData.queue\n");
		gst_object_unref (GstData.pipeline);
		return -1;
	}
	gst_caps_unref (caps_v4l2src);

	if(gst_element_link(GstData.queue, GstData.h264parse) != TRUE)
	{
		g_printerr ("GstData.queue could not link GstData.h264parse\n");
		gst_object_unref (GstData.pipeline);
		return -1;
	}

	if(gst_element_link_filtered(GstData.h264parse, GstData.flvmux, caps_flv_sink) != TRUE)
	{
		g_printerr ("GstData.h264parse could not link GstData.flvmux\n");
		gst_object_unref (GstData.pipeline);
		return -1;
	}
	gst_caps_unref (caps_flv_sink);

	if(gst_element_link(GstData.flvmux, GstData.rtmpsink) != TRUE)
	{
		g_printerr ("GstData.h264parse could not link GstData.flvmux\n");
		gst_object_unref (GstData.pipeline);
		return -1;
	}

	printf("========= link v4l2 rtmp pipeline running ==========\n");
	gst_element_set_state (GstData.pipeline, GST_STATE_PLAYING);
	GstData.loop = g_main_loop_new(NULL, FALSE);	// Create gstreamer loop
	g_main_loop_run(GstData.loop);					// Loop will run until receiving EOS (end-of-stream), will block here
	printf("g_main_loop_run returned, stopping rtmp!\n");
	gst_element_set_state (GstData.pipeline, GST_STATE_NULL);		// Stop pipeline to be released
	printf("Deleting pipeline\n");
	gst_object_unref (GstData.pipeline);							// THis will also delete all pipeline elements
	g_source_remove(GstData.bus_watch_id);
	g_main_loop_unref(GstData.loop);

	return 0;
}
