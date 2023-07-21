#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#define VIDEO_DEVICE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV

int main() {
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
    vfmt.fmt.pix.pixelformat = PIXEL_FORMAT;

    if (ioctl(fd, VIDIOC_S_FMT, &vfmt) == -1) {
        perror("Failed to set video format");
        close(fd);
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        close(fd);
        return 1;
    }

    // Create SDL window and renderer
    SDL_Window* window = SDL_CreateWindow("Camera Feed", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT, 0);
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

    // Allocate memory for video buffer
    size_t buffer_size = vfmt.fmt.pix.sizeimage;
    void* buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate buffer memory\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(fd);
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             WIDTH, HEIGHT);

    SDL_Event event;
    int quit = 0;
    while (!quit) {
        // Read video frame
        ssize_t bytes_read = read(fd, buffer, buffer_size);//直接用read或许会有一些问题
        if (bytes_read == -1) {
            perror("Failed to read video frame");
            break;
        }

        // Update SDL texture
        if (SDL_UpdateTexture(texture, NULL, buffer, vfmt.fmt.pix.width * 2) != 0) {
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
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    close(fd);

    return 0;
}
