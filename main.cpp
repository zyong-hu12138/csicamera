#include "csicamera.h"
#include <stdio.h>
#include <unistd.h>
#include <cstdint>
int count=0;
void callback(char *name , unsigned char *data , long int size)
{
    count++;
    if(count ==10)
    {
        printf("%s size = %ld\n" ,name, size);
        count=0;
    }
    FILE *file = fopen("image.rpg", "wb");
    if (file != NULL) {
        fwrite(data, 1, size, file);
        printf("write %s\n", name);
        fclose(file);
    }
}
int main()
{
    CSIcamera camera1("/dev/video1" , 960 , 720 , callback);
    CSIcamera camera2("/dev/video2" , 1280 , 960 , callback);
    CSIcamera camera3("/dev/video4" , 640 , 320 , callback);
    sleep(10);
    camera1.add_rtmp("rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A");
    sleep(10);
    camera1.remove_rtmp();
    sleep(10);
}
