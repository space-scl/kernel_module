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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>


#include <linux/wait.h>
#include <linux/sched.h>


// Register character device in probe routine for platform bus driver
#if 1
#define DEVICE_NAME "mydevice"
#define CLASS_NAME "myclass"

// Declare and initialze the variable of wait queue, test_wq
DECLARE_WAIT_QUEUE_HEAD(test_wq);
static int wq_condition = 0;
static int test_value = 0;

static struct tasklet_struct test_tasklet;

static void __iomem *reg_base;
static int irq_num;

static struct class *myclass = NULL;
static struct cdev my_cdev;

static int parameter;

// user can specify device number when installing module
// if user does not specify device number explicitly, it will be allocated dynamically
static int major;
static int minor;

static int array[10];
static int count;
static int trigger_irq = 0;
module_param(trigger_irq, int, 0644);

// pass parameters when insmod modules
// insmod <module_name> array=1,2,3
// cat /sys/module/dtPlatformDriver/parameters/array
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
    //printk(KERN_INFO "User want %zu chars\n", length);
    int message_size = strlen(message);
    int read_size = 0;

    if (*offset >= message_size) {
		wait_event_interruptible(test_wq, wq_condition);
    	if (copy_to_user(buffer, &test_value, 1))
    	    return -EFAULT;
		// set condition to 0 to block wait queue
        wq_condition = 0;
        return 0; // EOF
	}

	// offset: the file offset
	// length: the length that user want to read
	// check if the left content has the <length> bytes
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
    printk(KERN_INFO "the irq number is : %d\n", irq_num);
	if (kernel_buffer[0]==  '1') {
        // 注册中断处理函数
	    unsigned long flags;  // 它不会模拟硬件中断上下文的特性（如自动关闭中断、栈切换等）,需要手动关中断和恢复中断
        local_irq_save(flags);  // 保存并禁用中断
        generic_handle_irq(irq_num);
	    local_irq_restore(flags); // 恢复中断状态
    }

    // 释放内核缓冲区
    kfree(kernel_buffer);

    return length;
}

// #define _IOC(dir,type,nr,size)
#define CMD_TEST0  _IO('l',1)
#define CMD_TEST2  _IO('l',3)
#define CMD_TEST3  _IOR('l',5, int)
#define CMD_TEST4  _IOW('l',7, int)

long test_ioctl (struct file * pFile, unsigned int cmd, unsigned long value);
long test_ioctl (struct file * pFile, unsigned int cmd, unsigned long value)
{
	unsigned long* pValue = (unsigned long*)value;
	unsigned long tmp = 88;
	switch (cmd) {
		case CMD_TEST0:
			printk("this is ioctl: test0\n");
			break;
		case CMD_TEST2:
			printk("this is ioctl: test2\n");
			break;
		case CMD_TEST3:
			// when reading, user need to pass address to get data and use copy_to_user must be used
			printk("this is ioctl: test3\n");
            if (copy_to_user(pValue, &tmp, sizeof(*pValue)))
                return -EFAULT;
			break;
		case CMD_TEST4:
			// when writting, if user pass value not address, we can use the value directly
			// If user pass the address of data, copy_from_user must be used
			printk("this is ioctl: test4, write to device %ld\n", value);
			break;
		default:
			printk("this is ioctl: default\n");
			break;
	}

	return 0;
}

// 文件操作结构体
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
	.unlocked_ioctl = test_ioctl,
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
    free_irq(irq_num, NULL);

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

void taskletFunc(unsigned long data);
void taskletFunc(unsigned long data)
{
	int i=0xffffff;

	for (i = 0; i < data; i++) {
        printk(KERN_INFO "tasklet triggered: %d\n", i);
	}
}

static irqreturn_t test_irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "Interrupt triggered! start IRQ: %d\n", irq);
	test_value = test_value ^ 1;
    tasklet_schedule (&test_tasklet);
    printk(KERN_INFO "Interrupt triggered! end IRQ: %d\n", irq);
	printk(KERN_INFO "Interrupts disabled? %d\n", irqs_disabled()); // If interrupt is enalbed, the routine will return 0
	// set condition to 1 to unblock wait queue
    wq_condition = 1;
	wake_up(&test_wq);
    return IRQ_HANDLED;
}

static struct device_node * pNode = NULL;
static struct property * pProperty = NULL;
static int len;
static u32 out_values[2] = {0};
const char* str;


static int testprobe (struct platform_device * pDev)
{
    struct resource *res;
    int ret;

    // 映射寄存器
    res = platform_get_resource(pDev, IORESOURCE_MEM, 0);
    reg_base = devm_ioremap_resource(&pDev->dev, res);
    if (IS_ERR(reg_base))
        return PTR_ERR(reg_base);

    // 获取中断号
    irq_num = platform_get_irq(pDev, 0);
    if (irq_num < 0)
        return irq_num;

    ret = request_irq(irq_num, test_irq_handler, IRQF_TRIGGER_HIGH, DEVICE_NAME, NULL);
    if (ret) {
        return ret;
	}


    printk(KERN_INFO "Device ready, IRQ: %d, Reg: 0x%px\n", irq_num, reg_base);

	printk("name of device node: %s\n",pDev->dev.of_node->name);

	tasklet_init(&test_tasklet, taskletFunc, 100);

	// find device node by path
	pNode = of_find_node_by_path("/test");

	if (pNode == NULL) {
		return -1;
	}
	printk("name of device node: %s\n", pNode->name);
	printk("full name of device node: %s\n", pNode->full_name);

	// Get device node from platform_device which matches the device tree
	pNode = pDev->dev.of_node;

	// get compatible property from device node
	pProperty = of_find_property(pNode, "compatible", &len);
	if (pProperty == NULL) {
		return -1;
	}
	printk("name of property: %s\n", pProperty->name);
	printk("value of property: %s\n", (char*)pProperty->value);

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

	ret = mydevice_init();

	if (ret != 0) {
	    printk("Fail to init device in probe routine\n");
	    return -1;
	}
	static u32 * virtual_gpio_dr = NULL;

//			reg = <0x10 0x8   // index 0
//			       0x18 0x8   // index 1
//			  	  >;
	virtual_gpio_dr = of_iomap(pNode, 1); // map at 0x18 with 0x8 bytes
	return 0;

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


}


static void testremove (struct platform_device *pDev)
{
	mydevice_exit();
	tasklet_kill (&test_tasklet);
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
	{.compatible = "test1234"} // this name has the highest priority to match device in device tree
};

struct platform_driver pDriver =  {
	.probe = testprobe,
    .remove = testremove,
	.driver = {
        .name = "1234", // This name is also used to match platform device, but has low priority
        .owner = THIS_MODULE,
		.of_match_table = of_match_table_test,
    },
	.id_table = &test_id_table,
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

/* module_platform_driver() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
//#define module_platform_driver(__platform_driver)

/*
example:
static struct platform_driver mtk_btcvsd_snd_driver = {
	.driver = {
		.name = "mtk-btcvsd-snd",
		.of_match_table = mtk_btcvsd_snd_dt_match,
	},
	.probe = mtk_btcvsd_snd_probe,
	.remove = mtk_btcvsd_snd_remove,
};

module_platform_driver(mtk_btcvsd_snd_driver);
*/

