#include <gst/gst.h>

int main(int argc,char*argv[])
{
    GstElement *pipeline;
    GstElement *nvv4l2camerasrc;
    GstElement *nvvideoconvert;
    GstElement *fakesink;
    GstElement *rawvideoformat;
    GstElement *capsfilter;
    

    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    gst_init(&argc,&argv);

    nvv4l2camerasrc = gst_element_factory_make("nvv4l2camerasrc" , "nvv4l2camerasrc" );
    nvvideoconvert = gst_element_factory_make("nvvidconv" , "nvvideoconvert" );
    fakesink = gst_element_factory_make("fakesink" , "fakesink" );
    rawvideoformat = gst_element_factory_make("rawvideoformat" , "rawvideoformat" );
    capsfilter = gst_element_factory_make("capsfilter" , "capsfilter" );

    pipeline = gst_pipeline_new("test-pipeline");

    if( !pipeline || !nvv4l2camerasrc || !nvvideoconvert || !fakesink  || !capsfilter)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    gst_bin_add_many(GST_BIN(pipeline) , nvv4l2camerasrc , nvvideoconvert , fakesink , rawvideoformat , capsfilter , NULL);

    GstCaps *caps;
    caps = gst_caps_new_simple("video/x-raw(memory:NVMM))" ,
                                "format" , G_TYPE_STRING , "UYVY",
                                "width" , G_TYPE_INT , 1280,
                                "height" , G_TYPE_INT , 960,
                                "framerate" , GST_TYPE_FRACTION , 30 , 1 , NULL);



    g_object_set(capsfilter , "caps" , caps , NULL);
    // g_object_set(capsfilter , "caps-change-mode", GST_CAPS_CHANGE_NOTIFY, NULL);
    gst_caps_unref(caps);
    if(! gst_element_link_many(nvv4l2camerasrc , capsfilter , nvvideoconvert ,  fakesink , NULL))
    {
        g_printerr("Failed to link nvv4l2camerasrc , capsfilter , nvvidconv and fakesink.\n");
        return -1;
    }
    // if(!gst_element_link_filtered(nvv4l2camerasrc , nvvideoconvert , caps))
    // {
    //     g_printerr("Failed to link rawvideoforamt and nvvideoconvert.\n");
    //     return -1;
    // }
    // if(!gst_element_link_filtered(nvvideoconvert , fakesink , caps))
    // {
    //     g_printerr("Failed to link nvvideoconvert and fakesink.\n");
    //     return -1;
    // }

    // if(!gst_element_link_many(nvv4l2camerasrc , rawvideoformat , NULL))
    // {
    //     g_printerr("Failed to link nvv4l2camerasrc , rawvideoformat , nvvideoconvert and fakesink.\n");
    //     return -1;
    // }
    gst_caps_unref(caps);
    g_object_set(nvv4l2camerasrc , "device" , "/dev/video1" ,  NULL);
    ret = gst_element_set_state(pipeline , GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus , GST_CLOCK_TIME_NONE , GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if(msg != NULL)
    {
        GError *err;
        gchar *debug_info;

        switch(GST_MESSAGE_TYPE(msg))
        {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg , &err , &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                break;
            default:
                g_printerr("Unexpected message received.\n");
                break;
        }
        gst_message_unref(msg);
    }
}