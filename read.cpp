#include "semmap.h"
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


 
int main()
{
    int width = 960;
    int height = 720;
    key_t key1,key2;
    key1 = ftok("/dev/video1",2220+1);     //获取键值
    key2 = ftok("/dev/video1",1110+1);
    
    int shmid = shmget(key1,width*height*4,IPC_CREAT|0666); //打开或者创建共享内存
    int semid = semget(key2,1,IPC_CREAT|0666);//打开或者创建信号量组
   	    
    printf("semid:%d\n",semid);
    int num = semctl(semid , 0 , GETVAL);
    printf("????????????,%d",num);
    unsigned char *shmaddr = (unsigned char*)shmat(shmid,0,0); //共享内存连接到当前进程的地址空间
    printf("shmat ok\n");       //表示连接成功
    int length = strlen((const char*)shmaddr);
    printf("length :%d",length);
    // printf("data : %s\n",shmaddr); //将内存地址中的数据读出，打印
    shmdt(shmaddr);              //断开内存和当前进程的连接

    printf("quit\n");
    return 0;
}
