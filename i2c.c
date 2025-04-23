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

unsigned char i2c_read_data(unsigned int slave_addr, unsigned char reg_addr)
{
	struct i2c_rdwr_ioctl_data  i2c_read_dev;
	unsigned char data = 1;
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
			.flags = 1, // read operation
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

		printf("hihi data is %d, %c\n", data, data);
	return data;
}

int main ()
{
	unsigned char data = 0;

	fd = open("/dev/i2c-0", O_RDWR);
	if (fd < 0) {
		printf("Fail to open i2c-0\n");
		return -1;
	}

	while(1) {
		sleep(2);
		// i2c slave address is 0x51, the register address is 0x2
		data = i2c_read_data(0x51, 0x2);
		printf("data is %d\n", data);
	}

    return 0;
}


