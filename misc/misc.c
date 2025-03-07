#include <linux/fs.h>  // file_operations
//#include <linux/init.h> // THIS_MODULE
#include <linux/miscdevice.h> // struct miscdevice
#include <linux/module.h>  //MODULE_LICENSE 

static struct file_operations misc_fops = {
	.owner = THIS_MODULE
};

static struct miscdevice misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "misc_dev",
	.fops = &misc_fops
};

static int misc_init(void)
{
	int ret;

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


