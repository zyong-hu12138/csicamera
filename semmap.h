#ifndef _SEMMEP_H_
#define _SEMMEP_H_
/*
In order to realise a low cpu_load for camera, designed this module which used mmap and semaphore on POSIX.
mmap was used to avoid frenquently sys_call(write/read), reduce IO waste; and semaphore uesed to realise sync between producter and customer. 
Autuor: ZZ
Date: 202207

*/
#include <stdio.h>
#include <unistd.h>
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



#define BUFNUM 10
union semun {

   int   val;
   struct   semid_ds  *buf;
   unsigned short   *arrary;

};
struct ring_buf{
    int head;
    int tail;
    unsigned char* buf[BUFNUM];
};

class Sem_map
{
public:
    char* Save_buf_ptr;
    Sem_map(){};
    void init(key_t Semkey, key_t Shmkey, int width, int height, char *devname , int id);
    ~Sem_map(){};   
     int Proc(char* cam_file);   //main
     int Sync_init();
     int Write(unsigned char* Src,long int size);
     int P();
     int V();
     int B();
public:
    int Shm_ID;
    int write_to_file;
    int writensize;
    int buf_size;
    int Frame_size;
    struct ring_buf BUF;
    char Savefile[128];
    int Save_fd;
    int Retry_times;
    int Product_num;   
    key_t _Sem_key;
    key_t _Shm_key;
    int Semid;
    union semun Sem_args;
    int cnt;
    int id;
    char *dev_name;
};


#endif