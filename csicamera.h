#include <gst/gst.h>
#include <cstdint>

class CSIcamera
{
    public:
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
    //静态成员变量，用于保存类的对象指针
    static CSIcamera *instance;
    using CallbackFunctionType = void (*)(uint8_t * , long int);
    CallbackFunctionType callback;
    public:
    CSIcamera(char *dev_name , int width , int height , CallbackFunctionType input_callback);
    ~CSIcamera();
    void add_rtmp(char *rtmp_url);
    void remove_rtmp();
    static GstFlowReturn sample_callback(GstElement *sink );
    void create();
    void link();
};