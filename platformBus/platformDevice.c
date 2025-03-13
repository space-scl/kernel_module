#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/array_size.h>
#include <linux/device.h>

static void releasePlatformDevice (struct device *dev)
{
	printk("Release platfrom device");
	return;
}

struct resource deviceResource[] = {
    {
	    .start = 0x1,
	    .end = 0x8,
	    .name = "GPIO_5_DR",
	    .flags = IORESOURCE_MEM,
	}
};

struct platform_device myPlatformDevice[] = {
    {
		.name = "myPlatformDevice",  // This name is used to match platform driver
		.id = -1,
		.num_resources = ARRAY_SIZE(deviceResource),
		.resource = deviceResource,
		.dev = {.release = releasePlatformDevice}
    }
};

static int platform_device_init (void)
{
	int ret;

	ret = platform_device_register(myPlatformDevice);

	if (ret != 0) {
		printk("Fail to register platform device\n");
		return -1;
	}

	printk("Platform device registered\n");

	return 0;
}

static void platform_device_exit (void)
{

	platform_device_unregister(myPlatformDevice);

	printk("Platform device unregistered");
}

module_init(platform_device_init);
module_exit(platform_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple platform bus device");

