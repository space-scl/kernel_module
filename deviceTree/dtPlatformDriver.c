#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/array_size.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>


// Register character device in probe routine for platform bus driver
#if 1
#define DEVICE_NAME "mydevice"
#define CLASS_NAME "myclass"


static struct class *myclass = NULL;
static struct cdev my_cdev;

static int parameter;

// user can specify device number when installing module
// if user does not specify device number explicitly, it will be allocated dynamically
static int major;
static int minor;

static int array[10];
static int count;

module_param (parameter, int, 0444);
module_param (major, int, 0444);
module_param (minor, int, 0444);
module_param_array (array, int, &count, 0444);

// 设备文件的打开操作
static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device opened\n");
    return 0;
}

// 设备文件的关闭操作
static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

// 设备文件的读操作
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    char *message = "Hello from the kernel!\n";
    printk(KERN_INFO "User want %zu chars\n", length);
    int message_size = strlen(message);
    int read_size = 0;

    if (*offset >= message_size)
        return 0; // EOF

    if (message_size - *offset < length) {
	read_size = message_size - *offset;
    } else {
	read_size = length;
    }

    if (copy_to_user(buffer, message + *offset, read_size))
        return -EFAULT;

    *offset += read_size;
    return read_size;
}

// 设备文件的写操作
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    char *kernel_buffer;

    // 分配内核缓冲区
    kernel_buffer = kmalloc(length, GFP_KERNEL);
    if (!kernel_buffer)
        return -ENOMEM;

    // 从用户空间复制数据
    if (copy_from_user(kernel_buffer, buffer, length)) {
        kfree(kernel_buffer);
        return -EFAULT;
    }

    printk(KERN_INFO "Received data from user: %s\n", kernel_buffer);

    // 释放内核缓冲区
    kfree(kernel_buffer);
    return length;
}

// 文件操作结构体
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

// 模块初始化函数
static int __init mydevice_init(void) {
    dev_t dev;
    int i;
    int ret;

    printk("major device number from user: %d\n", major);
    printk("minor device number form user: %d\n", minor);

    // Try to allocate device number from user input
    // if there is user input, allocate device numbert statically
    // otherwise, allocate device number dynamically
    if (major != 0) {
        dev = MKDEV(major, minor);
        ret = register_chrdev_region(dev, 1, DEVICE_NAME);
        if (ret != 0) {
            printk("Fail to allocate device number\n");
            return -1;
        }
    } else {
        // 动态分配主设备号
        if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
            printk(KERN_ERR "Failed to allocate device number\n");
            return -1;
        }
    }

    printk("parameter from user: %d", parameter);
    printk("the number of array: %d", count);
    for (i = 0; i < parameter; i++) {
        printk("array[%d] from user: %d", i, array[i]);
    }

    major = MAJOR(dev);
    minor = MINOR(dev);
    printk(KERN_INFO "Allocated major number: %d\n", major);
    printk(KERN_INFO "Allocated minor number: %d\n", minor);

    // 创建设备类
    myclass = class_create(CLASS_NAME);
    if (IS_ERR(myclass)) {
        unregister_chrdev_region(dev, 1);
        printk(KERN_ERR "Failed to create device class\n");
        return PTR_ERR(myclass);
    }


    // 初始化字符设备
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    // 注册字符设备
    if (cdev_add(&my_cdev, dev, 1) < 0) {
        class_destroy(myclass);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ERR "Failed to add cdev\n");
        return -1;
    }

    // 创建设备文件
    device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
    printk(KERN_INFO "Device created: /dev/%s\n", DEVICE_NAME);

    return 0;
}

// 模块退出函数
static void __exit mydevice_exit(void) {
    dev_t dev = MKDEV(major, minor);

    // 销毁设备文件
    device_destroy(myclass, dev);
    class_destroy(myclass);

    // 注销字符设备
    cdev_del(&my_cdev);

    // 释放设备号
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "Device unregistered\n");
}

#endif

static int testprobe (struct platform_device * pDev)
{
	struct resource *pRsc = NULL;
	int ret;

	// Neet to specify the type of resource and the index of this resource
	pRsc = platform_get_resource (pDev, IORESOURCE_MEM, 0);
	if (pRsc == NULL) {
        printk("Fail to get resource in probe\n");
		return -EBUSY;
	}
	printk("name of device resource: %s\n", pRsc->name);
	printk("start of device resource: %x\n", pRsc->start);
	printk("end of device resource: %x\n", pRsc->end);

	ret = mydevice_init();

	if (ret != 0) {
	    printk("Fail to init device in probe routine\n");
	    return -1;
	}
#if 0
    static int __iomem * virMap;

	virMap = ioremap(pRsc->start, 4);
	if (virMap != 0) {
	    printk("Fail to execute ioremap\n");
	    return -1;
	}

	struct resource *pRscMap = NULL;

	// Reserve this MMIO space, this acts as a mutex lock
	// If accessing this MMIO space without calling request_mem_region, it may have potential problems
    pRscMap = request_mem_region(pRsc->start, pRsc->end - pRsc->start + 1, "reserveTheGpioMem");
	if (pRscMap == NULL) {
	    printk("Fail to request memory region\n");
		goto err_region;
	}

err_region:
    // release_mem_region(pRsc->start, pRsc->end - pRsc->start + 1);
	return -EBUSY;
#endif

	return 0;

}


static void testremove (struct platform_device *pDev)
{
	mydevice_exit();
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

const struct of_device_id of_match_table_test[] = {
	{.compatible = "test1234"}
};

struct platform_driver pDriver =  {
	.probe = testprobe,
    .remove = testremove,
	.driver = {
        .name = "123", // This name is also used to match platform device, but has low priority
        .owner = THIS_MODULE,
		.of_match_table = of_match_table_test,
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




