
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
#include <fstream>

 
int main()
{
    int width = 960;
    int height = 720;
    key_t key1,key2;
    key1 = ftok("/dev/video1",2220+1);     //获取键值
    key2 = ftok("/dev/video1",1110+1);
    printf("%d",key2);
    int shmid = shmget(key1,width*height*40,IPC_CREAT|0666); //打开或者创建共享内存
    int semid = semget(key2,1,IPC_CREAT|0666);//打开或者创建信号量组
    char *buffer;
    unsigned char *shmaddr = (unsigned char*)shmat(shmid,0,0); //共享内存连接到当前进程的地址空间

    for (int i = 0; i < 10; i++)
    {
        buffer = (char *)malloc(width*height*4);
        sleep(1);
        int num = semctl(semid , 0 , GETVAL);
        memcpy(buffer,shmaddr+width*height*4*num,width*height*4);
        std::string filename = "image" + std::to_string(i) + ".jpg";
        std::ofstream imageFile(filename, std::ios::binary);
        imageFile.write(buffer , width*height*4);
        
    }
    printf("shmat ok\n");       //表示连接成功
    shmdt(shmaddr);              //断开内存和当前进程的连接

    printf("quit\n");
    return 0;
}
