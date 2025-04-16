#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	struct input_event testInput;
	int    fd;
	ssize_t  size;

	fd = open("/dev/input/event1", O_RDWR);

	if (fd <= 0) {
		printf("Fail to open device file\n");
		return -1;
	}
	printf("open device file\n");

	while (1) {
		size = read(fd, &testInput, sizeof(testInput));

		if (size <= 0) 
			continue;
		if (testInput.type == 1)
			printf("the type is %d, code is %d, value is %d\n", testInput.type, testInput.code, testInput.value);

		// the type is 1, code is 2, value is 1
		// type: #define EV_KEY			0x01
		// code: #define KEY_1			2   # I press 1, it encoded number is 2
		// value: 1 for press, 0 for up. 2 for keep pressed

	}

	return 0;
}

