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
// unsigned char *p = map.data;
// for (size_t i = 0; i < map.size; i += 2) {
//         // YUYV 格式的数据是交错的，每两个字节表示一个像素的Y和U/V分量
//         unsigned char Y = map.data[i];
//         unsigned char U = map.data[i + 1];
//         unsigned char V = map.data[i + 3];
        
//         // printf("YUV: Y=%u, U=%u, V=%u\n", Y, U, V);
//     }
    // 取消内存映射并释放sample
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstElement *nvv4l2camerasrc;
    GstElement *nvvideoconvert;
    GstElement *tee;
    GstElement *queue1;
    GstElement *queue2;
    GstElement *appsink;
    GstElement *rtmpsink;
    GstElement *flvmux;
    GstElement *nvvidconv;
    GstElement *encoder;
    GstElement *nvvidconv2;//////////////////////
    GstElement *nvjpegenc;/////////////////////////////
    GstElement *capsfilter;/////////////////////////////
    GstElement *capsfilter1;
    GstElement *capsfilter2;
    GstElement *capsfilter3;

    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GstPad *tee_pad1;
    GstPad *tee_pad2;
    GstPad *queue1_pad;
    GstPad *queue2_pad;

    gst_init(&argc,&argv);

    nvv4l2camerasrc = gst_element_factory_make("nvv4l2camerasrc" , "nvv4l2camerasrc" );/////////////////////
    nvvideoconvert = gst_element_factory_make("nvvidconv" , "nvvideoconvert" );
    tee = gst_element_factory_make("tee" , "tee" );
    queue1 = gst_element_factory_make("queue" , "queue1" );
    queue2 = gst_element_factory_make("queue" , "queue2" );
    appsink = gst_element_factory_make("appsink" , "appsink" );
    rtmpsink = gst_element_factory_make("rtmpsink" , "rtmpsink" );
    flvmux = gst_element_factory_make("flvmux" , "flvmux" );
    nvvidconv = gst_element_factory_make("nvvidconv" , "nvvidconv" );
    encoder = gst_element_factory_make("omxh264enc" , "encoder" );
    nvvidconv2 = gst_element_factory_make("nvvidconv","nvvidconv2");///////////////////
    nvjpegenc = gst_element_factory_make("nvjpegenc","nvjpegenc");////////////////////
    capsfilter = gst_element_factory_make("capsfilter","capsfilter");////////////////////
    capsfilter1 = gst_element_factory_make("capsfilter","capsfilter1");
    capsfilter2 = gst_element_factory_make("capsfilter","capsfilter2");
    capsfilter3 = gst_element_factory_make("capsfilter","capsfilter3");

    pipeline = gst_pipeline_new("test-pipeline");

    if(!pipeline || !nvv4l2camerasrc || !nvvideoconvert || !tee || !queue1|| !queue2 ||
     !appsink || !rtmpsink || !flvmux || !nvvidconv || !encoder || !nvvidconv2 || 
     !nvjpegenc || !capsfilter || !capsfilter1 || !capsfilter2 || !capsfilter3)///////////////////
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }
    
    gst_bin_add_many (GST_BIN(pipeline) , nvv4l2camerasrc , nvvideoconvert , tee , queue1 ,
     queue2 , appsink , rtmpsink , flvmux , nvvidconv , encoder, nvvidconv2 , nvjpegenc , 
     capsfilter , capsfilter1 , capsfilter2 , capsfilter3 , NULL);////////////////////

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

    // if(gst_element_link_many(nvv4l2camerasrc  ,  capsfilter , tee , NULL)!=TRUE)
    // if(gst_element_link_many(nvv4l2camerasrc  ,  capsfilter , tee , NULL)!=TRUE)
    // {
    //     g_printerr("Elements1 could not be linked.\n");
    //     gst_object_unref(pipeline);
    //     return -1;
    // }
    if(gst_element_link_many(queue1 , nvvidconv2 , capsfilter1 , nvjpegenc ,appsink , NULL)!=TRUE ) /////
    {
        g_printerr("Elements2 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    if( gst_element_link_many(queue2 ,  nvvidconv , capsfilter2 , encoder , capsfilter3 , flvmux , rtmpsink , NULL)!=TRUE)
        {
            g_printerr("Elements3 could not be linked.\n");
            gst_object_unref(pipeline);
            return -1;
        }

    // 设置appsink的回调函数
    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample_callback), "test.jpg ");
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

    g_object_set(G_OBJECT(capsfilter) , "caps" , caps , NULL);
    
    if(gst_element_link_filtered(nvv4l2camerasrc  ,  tee , caps )!=TRUE)
    {
        g_printerr("Elements1 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_object_set(rtmpsink , "location" , "rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A",NULL);
    g_object_set(encoder , "bitrate" , 2000000 , NULL);
    g_object_set(encoder , "insert-sps-pps" , TRUE , NULL);
    g_object_set(encoder , "control-rate" , 1 , NULL);
    g_object_set(encoder , "preset-level" , 1 , NULL);
    g_object_set(encoder , "iframeinterval" , 30 , NULL);


    caps = gst_caps_new_simple("video/x-raw","format" , G_TYPE_STRING ,"NV12",NULL);
    
    gst_caps_set_features(caps,0,feature);
    g_object_set(G_OBJECT(capsfilter2) , "caps" , caps , NULL);
 

    caps = gst_caps_new_simple("video/x-raw",
                                "width" , G_TYPE_INT , 640,
                                "height" , G_TYPE_INT , 480 , 
                                "format" , G_TYPE_STRING , "NV12",NULL);
                                  
    gst_caps_set_features(caps,0,feature);
    g_object_set(G_OBJECT(capsfilter1) , "caps" , caps , NULL);

    
    caps = gst_caps_new_simple("video/x-h264",
                                "stream-format" , G_TYPE_STRING , "avc",
                                NULL);
    g_object_set(G_OBJECT(capsfilter3) , "caps" , caps , NULL);
    gst_caps_unref(caps);

    // 启动pipeline
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
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
