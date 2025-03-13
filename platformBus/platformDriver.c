#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/array_size.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>

static int testprobe (struct platform_device * pDev)
{
    printk("probe the device in platfrom driver\n");
	return 0;
}


static void testremove (struct platform_device *pDev)
{
    printk("Remove the device in platform driver\n");
	return;
}


/*
static struct device_driver driver = {
    .name = "myPlatformDevice",
	.owner = THIS_MODULE,
};
*/

const struct platform_device_id test_id_table = {
	.name = "myPlatformDevice"  // This name is used to match platform device
	//.driver_data = 0,
};

struct platform_driver pDriver =  {
	.probe = testprobe,
    .remove = testremove,
	.driver = {
        .name = "123", // This name is also used to match platform device, but has low priority
        .owner = THIS_MODULE,
    },
	.id_table = &test_id_table
};

static int platform_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&pDriver);
	if (ret != 0) {
	    printk("Fail to register platform driver\n");
		return -1;
	}
	printk("register platform driver\n");
	return 0;

}

static void platform_driver_exit(void)
{
	platform_driver_unregister(&pDriver);
	return;
}



module_init(platform_driver_init);
module_exit(platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple platform bus driver");




