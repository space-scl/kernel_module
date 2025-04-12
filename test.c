#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

// #define _IOC(dir,type,nr,size)
#define CMD_TEST0  _IO('l',1)
#define CMD_TEST2  _IO('l',3)
#define CMD_TEST3  _IOR('l',5, int)
#define CMD_TEST4  _IOW('l',7, int)


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
    printf("Data length from device: %d\n", ret);
    printf("Data from device: %s\n", buffer);

    memset (buffer, 0, 100);

    // 读取设备数据
    ret = read(fd, buffer, 17);
    if (ret < 0) {
        perror("Failed to read from device");
        close(fd);
        return -1;
    }
    printf("Data length from device: %d\n", ret);
    printf("Data from device: %s\n", buffer);

    // 写入设备数据
    char *message = "Hello from user space!";
    if (write(fd, message, strlen(message)+1) < 0) {
        perror("Failed to write to device");
        close(fd);
        return -1;
    }

	unsigned long  val = 99;

	ret = ioctl (fd, CMD_TEST2);
	ret = ioctl (fd, CMD_TEST0);
	ret |= ioctl (fd, CMD_TEST4, val);
	ret |= ioctl (fd, CMD_TEST3, &val);
	printf("read from kernel ioctl: %ld\n", val);
	ret |= ioctl (fd, CMD_TEST4, val);
	if (ret == -1) {
		printf("fail to do ioctl\n");
		return -1;
	}

	while (1) {
    	printf("User is reading \n");
    	ret = read(fd, buffer, 1);
    	if (ret < 0) {
    	    perror("Failed to read from device");
    	    close(fd);
    	    return -1;
    	}
    	printf("Data from device: %d\n", buffer[0]);
	}

    // 关闭设备文件
    close(fd);
    return 0;
}
