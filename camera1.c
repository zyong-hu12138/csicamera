#include <gst/gst.h>
#include <stdio.h>
#include <gst/video/video.h>
#include <pthread.h>
#include <stdlib.h>
// 回调函数，处理从appsink接收到的数据
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
    GstElement *queue2filter;
    

    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GstPad *tee_pad1;
    GstPad *tee_pad2;
    GstPad *queue1_pad;
    GstPad *queue2_pad;
void *dynamic();
void test();
int flag = 1;
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
    // printf("%ld \n ",map.size);
    // FILE *file = fopen(filename, "wb");
    // if (file != NULL) {
    //     fwrite(map.data, 1, map.size, file);
    //     fclose(file);
    // }
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
    gst_init(&argc,&argv);
    ////create elements
    nvv4l2camerasrc = gst_element_factory_make("nvv4l2camerasrc" , "nvv4l2camerasrc" );
    srccaps = gst_element_factory_make("capsfilter" , "srccaps" );
    nvvideoconvert1 = gst_element_factory_make("nvvidconv" , "nvvideoconvert1" );
    capsfilter = gst_element_factory_make("capsfilter" , "capsfilter" );
    tee = gst_element_factory_make("tee" , "tee" );
    queue1 = gst_element_factory_make("queue" , "queue1" );
    nvjpegenc = gst_element_factory_make("nvjpegenc" , "nvjpegenc" );
    appsink = gst_element_factory_make("appsink" , "appsink" );
    pipeline = gst_pipeline_new("test-pipeline");

    //convert NV12 to GBR
    GstElement *convertfilter;
    GstElement *bgrfilter;
    GstElement *rgbconvert;
    GstElement *rgbfilter;
    convertfilter = gst_element_factory_make("nvvidconv" , "convertfilter" );
    bgrfilter = gst_element_factory_make("capsfilter" , "bgrfilter" );
    rgbconvert = gst_element_factory_make("nvvidconv" , "rgbconvert" );
    rgbfilter = gst_element_factory_make("capsfilter" , "rgbfilter" );

    if(!pipeline || !nvv4l2camerasrc || !srccaps || !nvvideoconvert1 || !capsfilter || 
    !tee || !queue1 ||  !nvjpegenc || !appsink || !convertfilter || !bgrfilter || !rgbconvert || !rgbfilter)
    {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }
    
    gst_bin_add_many (GST_BIN(pipeline) , nvv4l2camerasrc , srccaps , nvvideoconvert1 , capsfilter ,
    tee , queue1 , nvjpegenc , appsink, convertfilter , bgrfilter , rgbconvert , rgbfilter , NULL);// queue2 , , encoder , rtmpcaps , flvmux , rtmpsink , nvvideoconvert2 , queue2filter 
    
    //connect tee and queue1 to get the data from camera
    GstPadTemplate *tee_src_pad_template;
    tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee),"src_%u");
    tee_pad1 = gst_element_request_pad(tee,tee_src_pad_template,NULL,NULL);
    queue1_pad = gst_element_get_static_pad(queue1,"sink");
  
    if(gst_pad_link(tee_pad1,queue1_pad) != GST_PAD_LINK_OK )
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_object_unref(queue1_pad); 
    ///connect all the elements
    if(gst_element_link_many(nvv4l2camerasrc , srccaps , nvvideoconvert1 , capsfilter , tee , NULL)!=TRUE)
    {
        g_printerr("Elements1 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    if(gst_element_link_many(queue1 , convertfilter , bgrfilter  , rgbconvert , rgbfilter , appsink , NULL)!=TRUE ) ///// nvjpegenc 
    {
        g_printerr("Elements2 could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // 设置appsink的回调函数
    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample_callback), "test.rgb");
    g_object_set(nvv4l2camerasrc , "device" , "/dev/video1" ,  NULL);
    ////设置srccaps格式     
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
    //设置tee输出的格式NV12
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
    //希望将NV12转换为RGBA
    GstCaps *bgrcaps;
    bgrcaps = gst_caps_new_simple("video/x-raw",
                                    "format" , G_TYPE_STRING ,"I420",NULL);
    feature = gst_caps_features_new("memory:NVMM",NULL);
    gst_caps_set_features(bgrcaps,0,feature);
    g_object_set(G_OBJECT(bgrfilter) , "caps" , bgrcaps , NULL);
    gst_caps_unref(bgrcaps);
    //希望得到RGB格式
    GstCaps *rgbcaps;
    rgbcaps = gst_caps_new_simple("video/x-raw",
                                    "format" , G_TYPE_STRING ,"RGBA",
                                     "width" , G_TYPE_INT , 960 ,
                                     "height" , G_TYPE_INT , 720 , 
                                     "framerate" , GST_TYPE_FRACTION , 30 , 1 ,NULL);
    g_object_set(G_OBJECT(rgbfilter) , "caps" , rgbcaps , NULL);
    gst_caps_unref(rgbcaps);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    // 启动pipeline
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }

    pthread_t tid;
    pthread_create(&tid,NULL,dynamic,NULL);
    pthread_detach(tid);
    sleep(20);
    // // 等待直到错误发生或EOS
    // bus = gst_element_get_bus(pipeline);
    // msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // // 处理消息
    // if (msg != NULL) {
    //     GError *err;
    //     gchar *debug_info;

    //     switch (GST_MESSAGE_TYPE(msg)) {
    //         case GST_MESSAGE_ERROR:
    //             gst_message_parse_error(msg, &err, &debug_info);
    //             g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    //             g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    //             g_clear_error(&err);
    //             g_free(debug_info);
    //             break;
    //         case GST_MESSAGE_EOS:
    //             g_print("End-Of-Stream reached.\n");
    //             break;
    //         default:
    //             // We should not reach here because we only asked for ERRORs and EOS
    //             g_printerr("Unexpected message received.\n");
    //             break;
    //     }
    //     gst_message_unref(msg);
    // }

    // // 停止pipeline
    // gst_element_set_state(pipeline, GST_STATE_NULL);

    // // 释放资源
    // gst_object_unref(bus);
    // gst_object_unref(pipeline);
    return 0;
}

void *dynamic()
{
    queue2 = gst_element_factory_make("queue" , "queue2" );
    encoder = gst_element_factory_make("omxh264enc" , "encoder" );
    rtmpcaps = gst_element_factory_make("capsfilter" , "rtmpcaps" );
    flvmux = gst_element_factory_make("flvmux" , "flvmux" );
    rtmpsink = gst_element_factory_make("rtmpsink" , "rtmpsink" );
    nvvideoconvert2 = gst_element_factory_make("nvvidconv" , "nvvideoconvert2" );

    g_object_set(rtmpsink , "location" , "rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A",NULL);
    g_object_set(encoder , "bitrate" , 2000000 , NULL);
    g_object_set(encoder , "insert-sps-pps" , TRUE , NULL);
    g_object_set(encoder , "control-rate" , 1 , NULL);
    g_object_set(encoder , "preset-level" , 1 , NULL);
    g_object_set(encoder , "iframeinterval" , 30 , NULL);

    GstCaps *capsrtmp;
    capsrtmp = gst_caps_new_simple("video/x-h264",
                                    "stream-format" , G_TYPE_STRING , "avc",
                                    NULL);
    g_object_set(rtmpcaps , "caps" , capsrtmp , NULL);
    gst_caps_unref(capsrtmp);
    int ret;
    g_print("??????????????????????\n");
    sleep(5);
    g_print("!!!!!!!!!!!!!!!!!!!!!!\n");
    ret = gst_element_set_state(pipeline , GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the paused state.\n");
        gst_object_unref(pipeline);
    }
    gst_bin_add_many(GST_BIN(pipeline),queue2 , encoder , rtmpcaps , flvmux , rtmpsink ,NULL);
    g_print("add sucess !!!\n");
    GstPadTemplate *tee_src_pad_template;
    tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
    tee_pad2 = gst_element_request_pad(tee, tee_src_pad_template, NULL, NULL);
    queue2_pad = gst_element_get_static_pad(queue2, "sink");
    if(gst_pad_link(tee_pad2, queue2_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
    }
    gst_object_unref(queue2_pad);
    gst_pad_activate_mode(tee_pad2, GST_PAD_MODE_PUSH, TRUE);
    if( gst_element_link_many(queue2 ,  encoder , rtmpcaps , flvmux , rtmpsink , NULL)!=TRUE)
    {
        g_printerr("Elements3 could not be linked.\n");
        gst_object_unref(pipeline);
    }
    gst_element_sync_state_with_parent(queue2);
    ret = gst_element_set_state(pipeline , GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
    g_print("dynamic\n");

    sleep(10);
    ///动态移除
    g_print("dynamic remove begin\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
    queue2_pad = gst_element_get_static_pad(queue2, "sink");;
    gst_pad_unlink(tee_pad2, queue2_pad);
    gst_object_unref(queue2_pad);
    g_print("0000000000000000000000000000\n");
    gst_element_set_state(queue2 , GST_STATE_NULL);
    gst_element_set_state(encoder , GST_STATE_NULL);
    gst_element_set_state(rtmpcaps , GST_STATE_NULL);
    gst_element_set_state(flvmux , GST_STATE_NULL);
    gst_element_set_state(rtmpsink , GST_STATE_NULL);

    g_print("1111111111111111111111111111\n");

    gst_bin_remove(GST_BIN(pipeline),queue2);
    gst_bin_remove(GST_BIN(pipeline),encoder);
    gst_bin_remove(GST_BIN(pipeline),rtmpcaps);
    gst_bin_remove(GST_BIN(pipeline),flvmux);
    gst_bin_remove(GST_BIN(pipeline),rtmpsink);

    g_print("2222222222222222222222222222\n");
    g_print("end to rtmp sink!!!!!!!\n");

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
    g_print("333333333333333333333333333\n");
    sleep(5);
    //第二次加入
    g_print("444444444444444444444444444444\n");
     queue2 = gst_element_factory_make("queue" , "queue2" );
    encoder = gst_element_factory_make("omxh264enc" , "encoder" );
    rtmpcaps = gst_element_factory_make("capsfilter" , "rtmpcaps" );
    flvmux = gst_element_factory_make("flvmux" , "flvmux" );
    rtmpsink = gst_element_factory_make("rtmpsink" , "rtmpsink" );
    nvvideoconvert2 = gst_element_factory_make("nvvidconv" , "nvvideoconvert2" );

    g_object_set(rtmpsink , "location" , "rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A",NULL);
    g_object_set(encoder , "bitrate" , 2000000 , NULL);
    g_object_set(encoder , "insert-sps-pps" , TRUE , NULL);
    g_object_set(encoder , "control-rate" , 1 , NULL);
    g_object_set(encoder , "preset-level" , 1 , NULL);
    g_object_set(encoder , "iframeinterval" , 30 , NULL);

    // GstCaps *capsrtmp;
    capsrtmp = gst_caps_new_simple("video/x-h264",
                                    "stream-format" , G_TYPE_STRING , "avc",
                                    NULL);
    g_object_set(rtmpcaps , "caps" , capsrtmp , NULL);
    gst_caps_unref(capsrtmp);
    ret = gst_element_set_state(pipeline , GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the paused state.\n");
        gst_object_unref(pipeline);
    }

    gst_bin_add_many(GST_BIN(pipeline),queue2 , encoder , rtmpcaps , flvmux , rtmpsink ,NULL);
    g_print("add sucess !!!\n");
    // GstPadTemplate *tee_src_pad_template;
    tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
    tee_pad2 = gst_element_request_pad(tee, tee_src_pad_template, NULL, NULL);
    queue2_pad = gst_element_get_static_pad(queue2, "sink");
    if(gst_pad_link(tee_pad2, queue2_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
    }
    gst_object_unref(queue2_pad);
    gst_pad_activate_mode(tee_pad2, GST_PAD_MODE_PUSH, TRUE);
    if( gst_element_link_many(queue2 ,  encoder , rtmpcaps , flvmux , rtmpsink , NULL)!=TRUE)
    {
        g_printerr("Elements3 could not be linked.\n");
        gst_object_unref(pipeline);
    }
    gst_element_sync_state_with_parent(queue2);
    ret = gst_element_set_state(pipeline , GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
    g_print("dynamic again !!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}
