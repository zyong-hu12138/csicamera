#include "csicamera.h"
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdint>
#include <sys/shm.h>
int count=0;
void callback(int id, char *name , unsigned char *data , long int size, char *addr)
{}
int main()
{
    CSIcamera camera3(4, "/dev/video4" , 640 , 320 , callback);
    sleep(5);
    CSIcamera camera2(2, "/dev/video2" , 1280 , 960 , callback);
    sleep(5);
    CSIcamera camera1(1, "/dev/video1" , 960 , 720 , callback);
    sleep(20);
    camera1.add_rtmp("rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A");
    sleep(10);
    camera1.remove_rtmp();
    sleep(10);
}
