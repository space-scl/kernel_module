#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd;
    char buffer[100] = {0};
    ssize_t ret;

    // 打开设备文件
    fd = open("/dev/mydevice", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    // 读取设备数据
    ret = read(fd, buffer, 5);
    if (ret < 0) {
        perror("Failed to read from device");
        close(fd);
        return -1;
    }
    printf("Data length from device: %ld\n", ret);
    printf("Data from device: %s\n", buffer);

    memset (buffer, 0, 100);

    // 读取设备数据
    ret = read(fd, buffer, 17);
    if (ret < 0) {
        perror("Failed to read from device");
        close(fd);
        return -1;
    }
    printf("Data length from device: %ld\n", ret);
    printf("Data from device: %s\n", buffer);

    // 写入设备数据
    char *message = "Hello from user space!";
    if (write(fd, message, strlen(message)+1) < 0) {
        perror("Failed to write to device");
        close(fd);
        return -1;
    }

    // 关闭设备文件
    close(fd);
    return 0;
}
