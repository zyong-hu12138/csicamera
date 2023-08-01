#include "csicamera.h"
#include <stdio.h>
#include <unistd.h>
int main()
{
    void callback(guint8 *data , long int size)
    CSIcamera camera("/dev/video1" , 960 , 720 , callback);
    sleep(10);
    camera.add_rtmp("rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A");
    sleep(10);
}
void callsback(uint8_t *data , long int size)
{
    printf("size = %ld\n" , size);
}