#include <gst/gst.h>

int main(int argc,char *argv[])
{
    GstElement *pipeline;
    GstElement *v4l2src;
    GstElement *videoconvert;
    GstElement *tee;
    GstElement *queue1;
    GstElement *queue2;
    GstElement *x264enc;
    GstElement *rtph264pay;
    GstElement *ximagesink;
    GstElement *rtmpsink;
    GstElement *flvmux;//////////////////
    GstElement *fpsdisplaysink;/////////
    GstElement *nvvidconv;//////////
    GstElement *encoder;//////////
    
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GstPad *tee_pad1;
    GstPad *tee_pad2;
    GstPad *queue1_pad;
    GstPad *queue2_pad;
    
    gst_init(&argc,&argv);

    v4l2src = gst_element_factory_make("v4l2src","v4l2src");
    videoconvert = gst_element_factory_make("videoconvert","videoconvert");
    tee = gst_element_factory_make("tee","tee");
    queue1 = gst_element_factory_make("queue","queue1");
    queue2 = gst_element_factory_make("queue","queue2");
    x264enc = gst_element_factory_make("x264enc","x264enc");
    rtph264pay = gst_element_factory_make("rtph264pay","rtph264pay");
    ximagesink = gst_element_factory_make("xvimagesink","ximagesink");//xvimagesink 显示图片；
    rtmpsink = gst_element_factory_make("rtmpsink","rtmpsink");
    flvmux = gst_element_factory_make("flvmux","flvmux");////////////////////
    fpsdisplaysink = gst_element_factory_make("fpsdisplaysink" , "fpsdisplaysink");////////
    nvvidconv = gst_element_factory_make("nvvidconv" , "nvvidconv");
    encoder = gst_element_factory_make("omxh264enc" , "encoder");///////////

    pipeline = gst_pipeline_new("test-pipeline");

    if(!pipeline || !v4l2src || !videoconvert || !tee || !queue1|| !queue2 || 
    !x264enc || !rtph264pay || !ximagesink || !rtmpsink ||
    !flvmux || !fpsdisplaysink ||!nvvidconv || !encoder)//////////////
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }
    
    gst_bin_add_many (GST_BIN(pipeline) , v4l2src , videoconvert , tee , queue1 ,
     queue2 , x264enc , rtph264pay , ximagesink , rtmpsink , 
     flvmux , fpsdisplaysink , nvvidconv , encoder, NULL);///////////

    GstPadTemplate *tee_src_pad_template;
    tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee),"src_%u");
    tee_pad1 = gst_element_request_pad(tee,tee_src_pad_template,NULL,NULL);
    g_print("Obtained request pad %s for video branch.\n",gst_pad_get_name(tee_pad1));
    queue1_pad = gst_element_get_static_pad(queue1,"sink");
    tee_pad2 = gst_element_request_pad(tee,tee_src_pad_template,NULL,NULL);
    g_print("Obtained request pad %s for audio branch.\n",gst_pad_get_name(tee_pad2));
    queue2_pad = gst_element_get_static_pad(queue2,"sink");
    if(gst_pad_link(tee_pad1,queue1_pad) != GST_PAD_LINK_OK ||
    gst_pad_link(tee_pad2,queue2_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_object_unref(queue1_pad);
    gst_object_unref(queue2_pad);


    if(gst_element_link_many(v4l2src , videoconvert , tee , NULL) != TRUE )
        {
        g_printerr("Elements1 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
//    if( gst_element_link_many(queue1 , nvvidconv , x264enc , rtph264pay ,flvmux , rtmpsink, NULL) != TRUE )/////rtmp brunch
    if( gst_element_link_many(queue1 , nvvidconv , encoder , flvmux , rtmpsink, NULL) != TRUE )  
       {
        g_printerr("Elements2 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    if(gst_element_link_many(queue2 ,  fpsdisplaysink , NULL) != TRUE)///////////display brunch
    {
        g_printerr("Elements3 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_object_set(fpsdisplaysink,"video-sink",ximagesink,NULL);
    g_object_set(v4l2src,"device","/dev/video1",NULL);
    g_object_set(x264enc,"tune",4,NULL);
    g_object_set(x264enc,"bitrate",1000,NULL);
    g_object_set(rtph264pay,"pt",96,NULL);
    g_object_set(rtph264pay,"config-interval",1,NULL);
    g_object_set(rtmpsink,"location","rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A",NULL);
    // g_object_set(fpsdisplaysink,"video-sink",ximagesink,NULL);
    g_object_set(encoder, "bitrate", 2000000, NULL);
    GstCaps *caps;
    caps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "avc", NULL);
    g_object_set(encoder, "caps", caps, NULL);
    gst_caps_unref(caps);


    g_object_set(nvvidconv, "nvbuf-memory-type", 4, "nvbuf-memory-type", 4, "nvbuf-memory-type", 4, "nvbuf-memory-type", 4, NULL);
    gst_element_set_state(pipeline,GST_STATE_PLAYING);

    g_print("Running...\n");
    GMainLoop *loop =g_main_loop_new(NULL,FALSE);
    g_print("Press Enter to stop streaming...\n");
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_main_loop_unref(loop);

    gst_object_unref(pipeline);
    return 0;

}