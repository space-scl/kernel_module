#include <linux/fs.h>  // file_operations
//#include <linux/init.h> // THIS_MODULE
#include <linux/miscdevice.h> // struct miscdevice
#include <linux/module.h>  //MODULE_LICENSE
#include <linux/of.h>  //MODULE_LICENSE

// Misc device major number is 10. And you can specify the minor dynamically
//
//

static struct file_operations misc_fops = {
	.owner = THIS_MODULE
};

static struct miscdevice misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "misc_dt",
	.fops = &misc_fops
};
static struct device_node * pNode = NULL;
static struct property * pProperty = NULL;
static int len;
static u32 out_values[2] = {0};
const char* str;


static int getDeviceTreeInfo(void)
{
	int ret;

	pNode = of_find_node_by_path("/test");

	if (pNode == NULL) {
		return -1;
	}
	printk("name of device node: %s\n", pNode->name);
	printk("full name of device node: %s\n", pNode->full_name);

	// get compatible property from device node
	pProperty = of_find_property(pNode, "reg", &len);
	if (pProperty == NULL) {
		return -1;
	}
	printk("name of property: %s\n", pProperty->name);
	printk("value of property: %x\n", *(((u32*)pProperty->value)+1));

	// get reg property from device node
    ret = of_property_read_u32_array(pNode, "reg", out_values, 2);
	if (ret != 0) {
		return -1;
	}
	printk("the values of reg %x, %x", out_values[0], out_values[1]);

	ret = of_property_read_string(pNode, "status", &str);
	if (ret != 0) {
		return -1;
	}
	printk("the values of status %s", str);
	return 0;
}


static int misc_init(void)
{
	int ret;

	getDeviceTreeInfo();

	ret = misc_register(&misc_dev);

	if (ret != 0) {
		printk("fail to register misc device\n");
		return -1;
	}

	printk("register misc device successfully\n");

	return 0;
}

static void misc_exit (void)
{
	printk("deregister misc device successfully\n");
	misc_deregister(&misc_dev);
}

module_init(misc_init);
module_exit(misc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple character misc device driver");


