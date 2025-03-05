#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd;
    char buffer[100];

    // 打开设备文件
    fd = open("/dev/mydevice", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    // 读取设备数据
    if (read(fd, buffer, sizeof(buffer)) < 0) {
        perror("Failed to read from device");
        close(fd);
        return -1;
    }
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
