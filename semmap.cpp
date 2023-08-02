#include "semmap.h"
#include <iostream>
using namespace std;

void Sem_map::init(key_t Semkey, key_t Shmkey, int width, int height)
{
    _Sem_key = Semkey;
    _Shm_key = Shmkey;
    printf("Semkey = %d, Shmkey = %d \n", _Sem_key, _Shm_key);
    Sem_args.val = 0;
    Frame_size = width * height * 2;
    write_to_file = 0;
    memset(&BUF, 0, sizeof(BUF));
    int cnt = 0;
}   


int Sem_map::Write(unsigned char* Src,long int size)
{

    int ret = 0;
    memcpy(BUF.buf[BUF.head], Src, size);
    if(write_to_file)
        writensize = write(Save_fd, BUF.buf[BUF.head], size);
    BUF.head++;
    BUF.head %= BUFNUM;
    V();
    // if(++cnt %5 == 0)
    //     printf("camid = %d, semval = %d \n", this->_Sem_key - 1110, semctl(Semid, 0, GETVAL));
    // cnt %= 5;
    // if(write_to_file)
    //     while(1);
    ret = semctl(Semid, 0, GETVAL);
    if (++cnt % 10 == 0)
        printf("camid = %d, semval = %d \n", this->_Sem_key - 1110, ret);
    cnt %= 10;

    if (ret == -1)
    {
        printf("[ERROR] camid: %d, Sem dead, restart~~~", this->_Sem_key - 1110);
        return Sync_init();
    }
    if(semctl(Semid, 0, GETVAL) >= BUFNUM)
        B();
    
    return 1;
}

int Sem_map::Sync_init()
{
    Semid = semget(_Sem_key, 1, IPC_CREAT | 0666);
    if(Semid == -1)
    {
        printf("semaphore init failed, retry times remain %d, now semkey = %d", Retry_times, _Sem_key);
        return -1;
    }
    printf("semid = %d\n", Semid);
    int ret = semctl(Semid, 0, SETVAL, Sem_args);
    if(ret == -1)
    {
        printf("semctl failed \n");
        return -1;
    }
    return 1;
}


int Sem_map::V()  //operation V ,semph + 1
{
    struct sembuf sops={0,+1,SEM_UNDO};//第三个参数设置为SEM_UNDO时当信号量小于0时会阻塞，设置为IPC_NOWAIT则不会阻塞
    return (semop(Semid,&sops,1));
}
int Sem_map::P()  //operation V ,semph - 1
{
    struct sembuf sops={0,-1,SEM_UNDO};//第三个参数设置为SEM_UNDO时当信号量小于0时会阻塞，设置为IPC_NOWAIT则不会阻塞
    return (semop(Semid,&sops,1));
}
int Sem_map::B() //operation BACKUP, sempth - BUFNUM
{
    struct sembuf sops={0,-BUFNUM,SEM_UNDO};
    return (semop(Semid,&sops,1));
}    

int Sem_map::Proc(char* cam_file)
{
    if(write_to_file)
    {
        Save_fd = open(cam_file, O_RDWR | O_CREAT, 0666);
        if (-1 == Save_fd)
        {
            printf("open Save_file failed \n");
            return -1;
        }
        // printf("fd = %d \n", Save_fd);
    }
    Shm_ID = shmget(_Shm_key, BUFNUM * Frame_size, IPC_CREAT | 0666);
    if(Shm_ID == -1)
    {
        perror("shmget");
        return -1;
    }
        
    Save_buf_ptr =(char*) shmat(Shm_ID, 0, 0);
    if(Save_buf_ptr == (void*)-1)
    {
        perror("shmat");
        return -1;
    }
    BUF.buf[0] = (unsigned char*)Save_buf_ptr;

    for(int i = 1; i < BUFNUM; i++)
    {
        BUF.buf[i] = (unsigned char*)(Save_buf_ptr + Frame_size * i);
    }

    return Sync_init();
}
