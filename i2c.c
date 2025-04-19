#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

int fd;

int i2c_read_data(unsigned int slave_addr, unsigned int reg_addr)
{
	struct i2c_rdwr_ioctl_data  i2c_read_dev;
	unsigned char data;
	int ret;

	struct i2c_msg msg[2] = {
	    [0] = {
		    .addr = slave_addr,
			.flags = 0, // write operation
		    .buf = (unsigned char*) &reg_addr,
			.len = sizeof(reg_addr)
		},
	    [1] = {
		    .addr = slave_addr,
			.flags = 1, // write operation
		    .buf = &data,
			.len = sizeof(data)
		}
	};

    i2c_read_dev.msgs = msg;
	i2c_read_dev.nmsgs = sizeof(msg)/sizeof(msg[0]);

	ret = ioctl(fd, I2C_RDWR, &i2c_read_dev);
	if (ret < 0) {
		printf("Fail to read");
		return -1;
	}

	return data;
}

int main ()
{
	unsigned char data;

	fd = open("/dev/i2c-1", O_RDWR);
	if (fd < 0) {
		printf("Fail to open i2c-1\n");
		return -1;
	}

	while(1) {
		sleep(1);
		data = i2c_read_data(0x38, 0x2);
		printf("data is %d\n", data);

	}

    return 0;
}


