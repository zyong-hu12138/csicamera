#include <gst/gst.h>
#include <cstdint>
#include <stdio.h>
#include "semmap.h"
class CSIcamera
{
    public:
    int id;

    GstElement *pipeline;
    GstElement *nvv4l2camerasrc;
    GstElement *srccaps;
    GstElement *nvvideoconvert1;
    GstElement *capsfilter;
    GstElement *tee;
    GstElement *queue1;
    GstElement *queue2;
    GstElement *appsink;
    GstElement *nvvideoconvert2;
    GstElement *encoder;
    GstElement *rtmpcaps;
    GstElement *flvmux;
    GstElement *rtmpsink;
    GstElement *queue2filter;
    GstElement *convertfilter;
    GstElement *bgrfilter;
    GstElement *rgbconvert;
    GstElement *rgbfilter;

    GstStateChangeReturn ret;
    GstPad *tee_pad1;
    GstPad *tee_pad2;
    GstPad *queue1_pad;
    GstPad *queue2_pad;

    Sem_map sem_map;//实现共享内存的类
    //静态成员变量，用于保存类的对象指针
    static CSIcamera *instance;
    using CallbackFunctionType = void (*)(int ,char* , unsigned char * , long int, char*);
    CallbackFunctionType callback;
    public:
    CSIcamera(int id ,char *dev_name , int width , int height , CallbackFunctionType input_callback);
    ~CSIcamera();
    void add_rtmp(char *rtmp_url);
    void remove_rtmp();
    static GstFlowReturn sample_callback(GstElement *sink , gpointer data );
    void create();
    void link();
};