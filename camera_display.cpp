#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <SDL2/SDL.h>
using namespace std;
#define VIDEO_DEVICE "/dev/video0"
#define WIDTH 1280
#define HEIGHT 960
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV

int main() {
    int ret;
    int fd = open(VIDEO_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("Failed to open video device");
        return 1;
    }
//设置视频格式
    struct v4l2_format vfmt;
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vfmt.fmt.pix.width = WIDTH;
    vfmt.fmt.pix.height = HEIGHT;
    vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    
    if (ioctl(fd, VIDIOC_S_FMT, &vfmt) == -1) {
        perror("Failed to set video format");
        close(fd);
        return 1;
    }
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_G_FMT, &vfmt);
    if (ret < 0) {
        perror("Get format failed:");
        return -1;
    }
    cout<<"width:"<<vfmt.fmt.pix.width<<"height"<<vfmt.fmt.pix.height<<endl;
    cout<<"pixelformat:"<<vfmt.fmt.pix.pixelformat<<endl;
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        close(fd);
        return 1;
    }

    // Create SDL window and renderer
    SDL_Window* window = SDL_CreateWindow("Camera Feed", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,WIDTH, HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        close(fd);
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(fd);
        return 1;
    }



    //申请内核缓冲区队列
    struct v4l2_requestbuffers reqbuffer;
    reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffer.count = 4;
    reqbuffer.memory = V4L2_MEMORY_MMAP;//映射方式
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuffer);
    if (ret < 0) {
        perror("Request Queue space failed:");
        return -1;
    }
    //映射到用户空间
    struct v4l2_buffer mapbuffer;
    int i;
    unsigned char *mptr[4];
    unsigned int size[4];//存储大小，方便释放
    mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (i = 0; i < 4; i++) {
        mapbuffer.index = i;
        ret = ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer);
        if (ret < 0) {
            perror("VIDIOC_QUERYBUF failed:");
            return -1;
        }
        size[i] = mapbuffer.length;
        mptr[i] = (unsigned char *)mmap(NULL, mapbuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mapbuffer.m.offset);
        if (mptr[i] == MAP_FAILED) {
            perror("mmap failed:");
            return -1;
        }
        memset(mptr[i], 0, mapbuffer.length);
        ret = ioctl(fd, VIDIOC_QBUF, &mapbuffer);
        if (ret < 0) {
            perror("VIDIOC_QBUF failed:");
            return -1;
        }
    }
    //开始采集图像
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        perror("VIDIOC_STREAMON failed:");
        return -1;
    }
    //从队列中提取一帧数据
    unsigned char *buffer = NULL;

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             WIDTH, HEIGHT);

    SDL_Event event;
    int quit = 0;
    struct v4l2_buffer readbuffer;
    while (!quit) {
        readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(fd, VIDIOC_DQBUF, &readbuffer);
        if (ret < 0) {
            perror("VIDIOC_DQBUF failed:");
            return -1;
        }
        //将图像显示到屏幕
        // Update SDL texture
        cout<<mptr[readbuffer.index].size()<<endl;
        if (SDL_UpdateTexture(texture, NULL, mptr[readbuffer.index], vfmt.fmt.pix.width * 2) != 0) {
            fprintf(stderr, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
            break;
        }

        // Clear the window
        SDL_RenderClear(renderer);

        // Copy texture to the renderer
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Update the renderer
        SDL_RenderPresent(renderer);

        // Handle quit event
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
                break;
            }
        }
    }

    // Free resources
    free(buffer);
    ret = ioctl(fd, VIDIOC_QBUF,&readbuffer);
    if(ret<0){
        perror("stop failed");
        return -1;
    }
        
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    close(fd);

    return 0;
}
