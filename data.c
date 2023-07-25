#include <stdio.h>
#include <stdlib.h>

int main() {
    // 文件名
    const char *filename = "test.yuv";

    // 打开文件
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return 1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 分配内存缓冲区，用于存储文件内容
    unsigned char *buffer = (unsigned char *)malloc(file_size);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error.\n");
        fclose(file);
        return 1;
    }

    // 读取文件内容到缓冲区
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Error reading file.\n");
        fclose(file);
        free(buffer);
        return 1;
    }

    // 关闭文件
    fclose(file);

    // 在这里，您可以对读取的数据进行进一步处理
    // 例如，根据 YUV 数据的格式提取 Y、U 和 V 分量

    // 假设数据格式是 I420 (YUV420)，每像素分量分别是 Y、U 和 V
    int width = 1280;  // 图像宽度
    int height = 960;  // 图像高度
    int Y_size = width * height;
    int UV_size = Y_size / 4;

    // 提取 Y 分量
    unsigned char *Y_data = buffer;
    // 提取 U 分量
    unsigned char *U_data = buffer + Y_size;
    // 提取 V 分量
    unsigned char *V_data = buffer + Y_size + UV_size;

    // 输出前几个像素的 Y、U 和 V 分量值
    for (int i = 0; i < 5; i++) {
        printf("Pixel %d: Y=%u, U=%u, V=%u\n", i,
               Y_data[i], U_data[i], V_data[i]);
    }

    // 释放缓冲区内存
    free(buffer);

    return 0;
}
