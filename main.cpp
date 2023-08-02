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
{
    int shm_id;
    shm_id = shmget(key_t(2220+id) ,size,0666);
    // char *addr;
    sleep(5);
    // addr = (char *)shmat(shm_id , NULL,0);
    // if(addr == (void *)-1)
    //     {
    //          perror("shmat");
    //         printf("Fail father shmat.\n");
    //         exit(-1);
    //     }

    printf("Shared memory string : %s\n",addr);
}
int main()
{
    CSIcamera camera1(1,"/dev/video1" , 960 , 720 , callback);
    CSIcamera camera2(2,"/dev/video2" , 1280 , 960 , callback);
    CSIcamera camera3(4,"/dev/video4" , 640 , 320 , callback);
    sleep(10);
    camera1.add_rtmp("rtmp://livepush.orca-tech.cn/live/Testttttt?txSecret=8534bcc7c701c866c9a9ca4b1bde28e1&txTime=653B985A");
    sleep(10);
    camera1.remove_rtmp();
    sleep(10);
}
