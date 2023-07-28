#include <gst/gst.h>
#include <stdio.h>
#include <gst/video/video.h>

// 回调函数，处理从appsink接收到的数据
static GstFlowReturn new_sample_callback(GstElement *sink, gpointer user_data) {
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    gchar *filename = (gchar *)user_data;

    // 从appsink中获取sample
    g_signal_emit_by_name(sink, "pull-sample", &sample, NULL);
    if (sample == NULL) {
        return GST_FLOW_ERROR;
    }

    // 获取sample中的buffer
    buffer = gst_sample_get_buffer(sample);
    if (buffer == NULL) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // 将buffer映射为内存
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // 处理数据
    // 这里可以对数据进行保存、处理或其他操作
    // 在本例中，我们将数据写入文件中
    printf("%ld  ",map.size);
    FILE *file = fopen(filename, "wb");
    if (file != NULL) {
        fwrite(map.data, 1, map.size, file);
        fclose(file);
    }

//     取消内存映射并释放sample
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstElement *nvv4l2camerasrc;
    GstElement *srccaps;
    GstElement *nvvideoconvert1;
    GstElement *capsfilter;
    GstElement *tee;
    GstElement *queue1;
    GstElement *queue2;
    GstElement *nvjpegenc;
    GstElement *appsink;
    GstElement *nvvideoconvert2;
    GstElement *encoder;
    GstElement *rtmpcaps;
    GstElement *flvmux;
    GstElement *rtmpsink;
    GstElement *queue1filter;
    // GstElement *queue2filter;
    GstElement *nvvideoconv;
    GstElement *rgbcaps;
    GstElement *videoconvert;
    
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GstPad *tee_pad1;
    GstPad *tee_pad2;
    GstPad *queue1_pad;
    GstPad *queue2_pad;

    gst_init(&argc,&argv);
    ////create elements
    nvv4l2camerasrc = gst_element_factory_make("nvv4l2camerasrc" , "nvv4l2camerasrc" );
    srccaps = gst_element_factory_make("capsfilter" , "srccaps" );
    nvvideoconvert1 = gst_element_factory_make("nvvidconv" , "nvvideoconvert1" );
    capsfilter = gst_element_factory_make("capsfilter" , "capsfilter" );
    tee = gst_element_factory_make("tee" , "tee" );
    queue1 = gst_element_factory_make("queue" , "queue1" );
    queue2 = gst_element_factory_make("queue" , "queue2" );
    nvjpegenc = gst_element_factory_make("nvjpegenc" , "nvjpegenc" );
    appsink = gst_element_factory_make("appsink" , "appsink" );
    encoder = gst_element_factory_make("omxh264enc" , "encoder" );
    rtmpcaps = gst_element_factory_make("capsfilter" , "rtmpcaps" );
    flvmux = gst_element_factory_make("flvmux" , "flvmux" );
    rtmpsink = gst_element_factory_make("rtmpsink" , "rtmpsink" );
    nvvideoconvert2 = gst_element_factory_make("nvvidconv" , "nvvideoconvert2" );
    pipeline = gst_pipeline_new("test-pipeline");
    queue1filter = gst_element_factory_make("capsfilter" , "queue1filter" );
    nvvideoconv = gst_element_factory_make("nvvidconv" , "nvvideoconv" );
    rgbcaps = gst_element_factory_make("capsfilter" , "rgbcaps" );
    videoconvert = gst_element_factory_make("videoconvert" , "videoconvert" );

    if(!pipeline || !nvv4l2camerasrc || !srccaps || !nvvideoconvert1 || !capsfilter || 
        !tee || !queue1 || !queue2 || !nvjpegenc || !appsink || 
        !encoder || !rtmpcaps || !flvmux || !rtmpsink || !nvvideoconvert2 ||!queue1filter || !nvvideoconv || !rgbcaps || !videoconvert)
        {
            g_printerr("Not all elements could be created.\n");
            return -1;
        }
    
    gst_bin_add_many (GST_BIN(pipeline) , nvv4l2camerasrc , srccaps , nvvideoconvert1 , capsfilter , nvvideoconv , rgbcaps , videoconvert ,
     tee , queue1 , queue2 , nvjpegenc , appsink , encoder , rtmpcaps , flvmux , rtmpsink , nvvideoconvert2 ,  queue1filter , NULL);
    
    //connect tee and queue1&queue2
    GstPadTemplate *tee_src_pad_template;
    tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee),"src_%u");
    tee_pad1 = gst_element_request_pad(tee,tee_src_pad_template,NULL,NULL);
    tee_pad2 = gst_element_request_pad(tee,tee_src_pad_template,NULL,NULL);
    queue1_pad = gst_element_get_static_pad(queue1,"sink");
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

    ///connect all the elements
    if(gst_element_link_many(nvv4l2camerasrc , srccaps , nvvideoconvert1 , capsfilter , tee , NULL)!=TRUE)
    {
        g_printerr("Elements1 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    // if(gst_element_link_many(queue1 , nvjpegenc ,appsink , NULL)!=TRUE ) /////
    // {
    //     g_printerr("Elements2 could not be linked.\n");
    //     gst_object_unref(pipeline);
    //     return -1;
    // }
    if(gst_element_link_many(queue1 , nvvideoconvert2 , queue1filter , appsink , NULL)!=TRUE)// , nvvideoconvert2 , queue1filter ,, nvvideoconv , rgbcaps , videoconvert ,
    {
        g_printerr("Elements2 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    if( gst_element_link_many(queue2 ,  encoder , rtmpcaps , flvmux , rtmpsink , NULL)!=TRUE)
        {
            g_printerr("Elements3 could not be linked.\n");
            gst_object_unref(pipeline);
            return -1;
        }

    // 设置appsink的回调函数
    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample_callback), "test.png");
    g_object_set(nvv4l2camerasrc , "device" , "/dev/video1" ,  NULL);

    GstCaps *caps;
    caps = gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, "UYVY",
                                "width", G_TYPE_INT, 1280,
                                "height", G_TYPE_INT, 960,
                                "framerate", GST_TYPE_FRACTION, 30, 1,
                                NULL);
    GstCapsFeatures *feature = gst_caps_features_new("memory:NVMM",NULL);
    gst_caps_set_features(caps,0,feature);

    g_object_set(G_OBJECT(srccaps) , "caps" , caps , NULL);
    gst_caps_unref(caps);

    g_object_set(rtmpsink , "location" , "rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A",NULL);
    g_object_set(encoder , "bitrate" , 2000000 , NULL);
    g_object_set(encoder , "insert-sps-pps" , TRUE , NULL);
    g_object_set(encoder , "control-rate" , 1 , NULL);
    g_object_set(encoder , "preset-level" , 1 , NULL);
    g_object_set(encoder , "iframeinterval" , 30 , NULL);

    GstCaps *filtercaps;
    filtercaps = gst_caps_new_simple("video/x-raw",
                                    "format" , G_TYPE_STRING ,"NV12",
                                     "width" , G_TYPE_INT , 960 ,
                                     "height" , G_TYPE_INT , 720 , 
                                     "framerate" , GST_TYPE_FRACTION , 30 , 1 ,NULL);
    feature = gst_caps_features_new("memory:NVMM",NULL);
    gst_caps_set_features(filtercaps,0,feature);
    g_object_set(G_OBJECT(capsfilter) , "caps" , filtercaps , NULL);
    gst_caps_unref(filtercaps);
    
    GstCaps *capsrtmp;
    capsrtmp = gst_caps_new_simple("video/x-h264",
                                "stream-format" , G_TYPE_STRING , "avc",
                                NULL);
    g_object_set(rtmpcaps , "caps" , capsrtmp , NULL);
    gst_caps_unref(capsrtmp);

    GstCaps *queue1caps;
    queue1caps = gst_caps_new_simple("video/x-raw",
                        "format" , G_TYPE_STRING , "RGBA",
                        "width" , G_TYPE_INT , 960 ,
                        "height" , G_TYPE_INT , 720 ,
                        "framerate" , GST_TYPE_FRACTION , 30 , 1 ,
                        NULL);
    feature = gst_caps_features_new("memory:NVMM",NULL);
    gst_caps_set_features(queue1caps,0,feature);
    g_object_set(G_OBJECT(queue1filter) , "caps" , queue1caps , NULL); 
    gst_caps_unref(queue1caps);
    
    GstCaps *capsrgb;
    capsrgb = gst_caps_new_simple("video/x-raw",
                                "format" , G_TYPE_STRING , "RGB",
                                "width" , G_TYPE_INT , 960 ,
                                "height" , G_TYPE_INT , 720 ,
                                "framerate" , GST_TYPE_FRACTION , 30 , 1 ,
                                NULL);
    // feature = gst_caps_features_new("memory:NVMM",NULL);
    gst_caps_set_features(capsrgb,0,feature);
    g_object_set(G_OBJECT(rgbcaps) , "caps" , capsrgb , NULL);
    gst_caps_unref(capsrgb);


    // 启动pipeline
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    int flag=0;
    ///////change the state of tee
    if(flag ==0)
    {
       gst_element_unlink(tee, queue2);
    }

    // 等待直到错误发生或EOS
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // 处理消息
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                break;
            default:
                // We should not reach here because we only asked for ERRORs and EOS
                g_printerr("Unexpected message received.\n");
                break;
        }
        gst_message_unref(msg);
    }

    // 停止pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // 释放资源
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    return 0;

}
