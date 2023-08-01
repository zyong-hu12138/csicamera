#include "csicamera.h"
#include <unistd.h>
CSIcamera* CSIcamera::instance = NULL;
//create the main    element
void CSIcamera::create()
{
    gst_init(NULL,NULL);
    nvv4l2camerasrc = gst_element_factory_make("nvv4l2camerasrc" , "nvv4l2camerasrc" );
    srccaps = gst_element_factory_make("capsfilter" , "srccaps" );
    nvvideoconvert1 = gst_element_factory_make("nvvidconv" , "nvvideoconvert1" );
    capsfilter = gst_element_factory_make("capsfilter" , "capsfilter" );
    tee = gst_element_factory_make("tee" , "tee" );
    queue1 = gst_element_factory_make("queue" , "queue1" );
    appsink = gst_element_factory_make("appsink" , "appsink" );
    pipeline = gst_pipeline_new("test-pipeline");
    convertfilter = gst_element_factory_make("nvvidconv" , "convertfilter" );
    bgrfilter = gst_element_factory_make("capsfilter" , "bgrfilter" );
    rgbconvert = gst_element_factory_make("nvvidconv" , "rgbconvert" );
    rgbfilter = gst_element_factory_make("capsfilter" , "rgbfilter" );

    if(!pipeline || !nvv4l2camerasrc || !srccaps || !nvvideoconvert1 || !capsfilter || 
        !tee || !queue1 || !appsink || !convertfilter || !bgrfilter || !rgbconvert || !rgbfilter)
        {
            g_printerr("Not all elements could be created.\n");
        }
    gst_bin_add_many (GST_BIN(pipeline) , nvv4l2camerasrc , srccaps , nvvideoconvert1 , capsfilter ,
    tee , queue1 , appsink, convertfilter , bgrfilter , rgbconvert , rgbfilter , NULL);
}
//connect the elements
void CSIcamera::link()
{
    GstPadTemplate *tee_src_pad_template;
    tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee),"src_%u");
    tee_pad1 = gst_element_request_pad(tee,tee_src_pad_template,NULL,NULL);
    queue1_pad = gst_element_get_static_pad(queue1,"sink");

    if(gst_pad_link(tee_pad1 , queue1_pad) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
    }
    gst_object_unref(queue1_pad);

    if(gst_element_link_many(nvv4l2camerasrc , srccaps , nvvideoconvert1 , capsfilter , tee , NULL) != TRUE)
    {
        g_printerr("Elements1 could not be linked.\n");
        gst_object_unref(pipeline);
    }

    if(gst_element_link_many(queue1 , convertfilter , bgrfilter , rgbconvert , rgbfilter , appsink , NULL) != TRUE)
    {
        g_printerr("Elements2 could not be linked.\n");
        gst_object_unref(pipeline);
    }
}

CSIcamera::CSIcamera(char *dev_name , int width , int height , CallbackFunctionType input_callback)
:callback(input_callback)
{
    instance = this;
    create();
    link();
    
    g_object_set(nvv4l2camerasrc , "device" , dev_name , NULL);
    g_object_set(G_OBJECT(appsink) , "emit-signals" , TRUE , "sync" , FALSE , NULL);
    g_signal_connect(appsink , "new-sample" , G_CALLBACK(sample_callback) , dev_name);
    
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
                                     "width" , G_TYPE_INT , width ,
                                     "height" , G_TYPE_INT , height , 
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
                                     "width" , G_TYPE_INT , width ,
                                     "height" , G_TYPE_INT , height , 
                                     "framerate" , GST_TYPE_FRACTION , 30 , 1 ,NULL);
    g_object_set(G_OBJECT(rgbfilter) , "caps" , rgbcaps , NULL);
    gst_caps_unref(rgbcaps);

    ret = gst_element_set_state(pipeline , GST_STATE_PLAYING);
    g_print("???????????????????\n");
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
    // sleep(10);
}

GstFlowReturn CSIcamera:: sample_callback(GstElement *sink , gpointer data)
{
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    char *devname = (char *)data;
    // g_print("appsink callback\n");
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

    instance->callback(devname , map.data , map.size);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

void CSIcamera::add_rtmp(char *rtmp_url)
{
    queue2 = gst_element_factory_make("queue" , "queue2" );
    encoder = gst_element_factory_make("omxh264enc" , "encoder" );
    rtmpcaps = gst_element_factory_make("capsfilter" , "rtmpcaps" );
    flvmux = gst_element_factory_make("flvmux" , "flvmux" );
    rtmpsink = gst_element_factory_make("rtmpsink" , "rtmpsink" );
    nvvideoconvert2 = gst_element_factory_make("nvvidconv" , "nvvideoconvert2" );

    g_object_set(G_OBJECT(rtmpsink) , "location" , rtmp_url , NULL);
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
}

void CSIcamera::remove_rtmp()
{
    ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
    queue2_pad = gst_element_get_static_pad(queue2, "sink");;
    gst_pad_unlink(tee_pad2, queue2_pad);
    gst_object_unref(queue2_pad);

    gst_element_set_state(queue2 , GST_STATE_NULL);
    gst_element_set_state(encoder , GST_STATE_NULL);
    gst_element_set_state(rtmpcaps , GST_STATE_NULL);
    gst_element_set_state(flvmux , GST_STATE_NULL);
    gst_element_set_state(rtmpsink , GST_STATE_NULL);

    gst_bin_remove(GST_BIN(pipeline),queue2);
    gst_bin_remove(GST_BIN(pipeline),encoder);
    gst_bin_remove(GST_BIN(pipeline),rtmpcaps);
    gst_bin_remove(GST_BIN(pipeline),flvmux);
    gst_bin_remove(GST_BIN(pipeline),rtmpsink);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
    }
}

CSIcamera::~CSIcamera()
{
    gst_element_set_state(pipeline , GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_print("delete sucess !!!\n");
}