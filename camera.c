#include <gst/gst.h>

int main(int argc,char *argv[])
{
    //定义需要的模块
    GstElement *pipeline,*v4l2src,*tee,*queue1,*queue2,*displaysink,*videorate,*videofilter,*x264enc,*rtph264pay,*avimux,*pushsink;
    GstBus *bus;
    GstMessage *msg;
    GstPad *tee_pad1,*tee_pad2,*queue1_pad,*queue2_pad;
    gst_init(&argc,&argv);
    //create the elements
    v4l2src = gst_element_factory_make("v4l2src","v4l2src");
    tee = gst_element_factory_make("tee","tee");
    // rawvideoparse = gst_element_factory_make("rawvideoparse","rawvideoparse");
    queue1 = gst_element_factory_make("queue","queue1");
    queue2 = gst_element_factory_make("queue","queue2");
    displaysink = gst_element_factory_make("autovideosink","displaysink");
    videorate = gst_element_factory_make("videorate","videorate");
    videofilter = gst_element_factory_make("capsfilter","videofilter");
    x264enc = gst_element_factory_make("x264enc","x264enc");
    rtph264pay = gst_element_factory_make("rtph264pay","rtph264pay");
    avimux = gst_element_factory_make("avimux","avimux");
    pushsink = gst_element_factory_make("rtmpsink","pushsink");

    //create the empty pipeline
    pipeline = gst_pipeline_new("test-pipeline");
    if(!pipeline ||!v4l2src ||!tee ||!queue1 ||!queue2 ||!displaysink ||!videorate ||!videofilter ||!x264enc ||!rtph264pay ||!avimux ||!pushsink)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }
    //configure elements
    g_object_set(v4l2src,"device","/dev/video1",NULL);
    // g_object_set(rawvideoparse,"width",1280,"height",960,"framerate",30,"format",4,NULL);
    // g_object_set(videofilter,"caps",gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "YUY2", NULL),NULL);
    caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "YUY2", NULL);
    g_object_set(videofilter, "caps", caps, NULL);
    gst_caps_unref(caps);
    g_object_set(pushsink,"location","rtmp://livepush.orca-tech.cn/live/test1?txSecret=8392bc3cc98da4c7a062f840d4ee0937&txTime=6430CCB3",NULL);
    // g_object_set(x264enc , "tune" ,4 ,"speed_preset",1,NULL);
    caps = gst_caps_new_simple("video/x-h264", "profile", G_TYPE_STRING, "baseline", NULL);
    g_object_set(x264enc, "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipeline) , v4l2src , tee , queue1 , queue2 , displaysink , videorate , videofilter , x264enc , rtph264pay , avimux , pushsink , NULL);
    if(gst_element_link_many(v4l2src , tee , NULL) != TRUE)
        {
            g_printerr("Elements1 could not be linked.\n");
            gst_object_unref(pipeline);
            return -1;
        }
    if(gst_element_link_many(queue1 , displaysink , NULL) != TRUE )
    {
            g_printerr("Elements2 could not be linked.\n");
            gst_object_unref(pipeline);
            return -1;
    } 
    if(gst_element_link_many(queue2 , videorate , videofilter , x264enc , rtph264pay , avimux , pushsink , NULL) != TRUE)
    {
        g_printerr("Elements3 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    //manually link the tee , which has "Request" pads
    // tee_pad1 = gst_element_request_pad_simple(tee , "src_%u");
    GstPadTemplate *tee_src_pad_template1 = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee) , "src_%u");
    // gchar *pad_name = gst_pad_template_get_name(tee_src_pad_template1,NULL);
    tee_pad1 = gst_element_request_pad(tee , tee_src_pad_template1 , NULL, NULL);
    // g_free(pad_name);

    g_print("Obtained request pad %s for sink branch.\n",gst_pad_get_name(tee_pad1));
    queue1_pad = gst_element_get_static_pad(queue1 , "sink");
    // tee_pad2 = gst_element_request_pad_simple(tee , "src_%u");
    GstPadTemplate *tee_src_pad_template2 = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee) , "src_%u");
    // gchar *pad_name1 = gst_pad_template_get_name(tee_src_pad_template2 , NULL);
    tee_pad2 = gst_element_request_pad(tee , tee_src_pad_template2 , NULL, NULL);
    // g_free(pad_name);
    
    g_print("Obtained request pad %s for rtmp branch.\n",gst_pad_get_name(tee_pad2));
    queue2_pad = gst_element_get_static_pad(queue2 , "sink");
    if(gst_pad_link(tee_pad1 , queue1_pad) != GST_PAD_LINK_OK || gst_pad_link(tee_pad2 , queue2_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_object_unref(queue1_pad);
    gst_object_unref(queue2_pad);

    //start playing
    gst_element_set_state(pipeline , GST_STATE_PLAYING);

    //wait until error or EOS
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus , GST_CLOCK_TIME_NONE , GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    //release the request pads from the tee , and unref them
    gst_element_release_request_pad(tee , tee_pad1);
    gst_element_release_request_pad(tee , tee_pad2);
    gst_object_unref(tee_pad1);
    gst_object_unref(tee_pad2);

    //free resources
    if(msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline , GST_STATE_NULL);
    gst_object_unref(pipeline);
    
    return 0;
}