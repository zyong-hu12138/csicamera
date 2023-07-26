import cv2
import numpy as np

def read_yuv_file(file_path, width, height):
    yuv_size = width * height * 3 // 2

    with open(file_path, 'rb') as file:
        yuv_data = file.read(yuv_size)
    # yuv_data = yuv_data + b'\x00' * (1280*960 - len(yuv_data))
    print(len(yuv_data))
    return yuv_data

def convert_yuv_to_image(yuv_data, width, height):
    Y = np.frombuffer(yuv_data[:width*height], dtype=np.uint8).reshape((height, width))
    U = np.frombuffer(yuv_data[width*height:width*height*5//4], dtype=np.uint8).reshape((height//2, width//2))
    V = np.frombuffer(yuv_data[width*height*5//4:], dtype=np.uint8).reshape((height//2, width//2))

    U = cv2.resize(U, (width, height))
    V = cv2.resize(V, (width, height))

    YUV = cv2.merge((Y, U, V))
    image = cv2.cvtColor(YUV, cv2.COLOR_YUV2BGR_I420)

    return image

def main():
    width = 1280
    height = 960
    file_path = "test.yuv"

    yuv_data = read_yuv_file(file_path, width, height)
    image = convert_yuv_to_image(yuv_data, width, height)

    cv2.imshow("YUV Image", image)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
