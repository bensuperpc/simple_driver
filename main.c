/*
    * Simple Linux driver
    * 
    * This is a simple Linux driver that can be used as a template for other drivers.
    * Multiple devices can be created with this driver on /dev/device_driverX where X is the minor number.
    * Code is written in C89 and tested with Linux 5.15.
    * 
    * This driver is based on the following sources:
    * - https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html
    * - http://embeddedguruji.blogspot.com/2019/01/linux-character-driver-creating.html
    * - https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
    * - https://sysprog21.github.io/lkmpg/
*/

#include <linux/version.h> /* LINUX_VERSION_CODE, KERNEL_VERSION */
#include <linux/compiler.h> /* __must_check */

#include <linux/kernel.h> /* printk, pr_err, pr_debug */
#include <linux/init.h> /* __init, __exit */
#include <linux/module.h> /* THIS_MODULE */
#include <linux/fs.h> /* file_operations */
#include <linux/cdev.h> /* cdev */
#include <asm/ioctl.h> /* ioctl */
#include <linux/slab.h> /* kmalloc / kfree */
#include <linux/sched.h> /* wait_* */

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
#warning "Only linux 5.15 or below is tested with this module"
#endif

#if __STDC_VERSION__ > 201112L
#warning "This module was only tested with C89"
#endif

#define NBR_MINORS 2

MODULE_DESCRIPTION("Simple Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bensuperpc");
MODULE_INFO(name, "simple_linux_driver");
MODULE_INFO(OS, "Linux");
MODULE_VERSION("1.0.0");

/* Device data structure */
struct device_data
{
    /* Char device */
    struct cdev cdev;
    /* Device data */
    char *buffer;
    ssize_t size;
};

/* Store device data */
static struct device_data devs[NBR_MINORS];

/* Store device number */
static dev_t device_number;

/* Store device class */
static struct class *device_class;

/* When the device is opened */
static int open_device(struct inode *inode, struct file *file)
{
    struct device_data *data = &devs[iminor(inode)];

    pr_debug("[open_device]\n");

    file->private_data = data;

    return 0;
}

/* When the device is closed */
static int close_device(struct inode *inode, struct file *file)
{
    pr_debug("[close_device]\n");

    return 0;
}

/* When the device is read */
static ssize_t read_device(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct device_data *data =
        (struct device_data *)file->private_data;
    ssize_t len = min((ssize_t)(data->size - *offset), (ssize_t)size);

    pr_debug("[read_device]\n");

    if (len <= 0)
    {
        return 0;
    }

    /* Copy data from kernel space to user space */
    if (copy_to_user(user_buffer, data->buffer + *offset, len))
    {
        pr_err("copy_to_user error");
        return -EFAULT;
    }
    
    *offset += len;
    return len;
}

/* When the device is written */
static ssize_t write_device(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset)
{
    struct device_data *data = (struct device_data *)file->private_data;
    ssize_t len = min((ssize_t)(data->size - *offset), (ssize_t)size);

    pr_debug("[write_device]\n");
    if (len <= 0)
    {
        return 0;
    }
    
    if (size != data->size) {
        if(data->buffer = krealloc(data->buffer, sizeof(char) * size, GFP_KERNEL | GFP_ATOMIC))
        {
            return -ENOMEM;
        }

        data->size = size;
    }

    /* Copy data from user space to kernel space */
    if (copy_from_user(data->buffer + *offset, user_buffer, len))
    {
        pr_err("copy_from_user error");
        return -EFAULT;
    }

    *offset += len;
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open_device,
    .read = read_device,
    .write = write_device,
    .release = close_device,
};

/* When the module is loaded */
static int __init register_device(void)
{
    int i = 0;
    int major = 0;
    dev_t device = 0;

    pr_debug("Start initialization\n");

    if (alloc_chrdev_region(&device_number, 0, NBR_MINORS, "device_driver"))
    {
        pr_err("alloc_chrdev_region error");
        return -ENOMEM;
    }

    device_class = class_create(THIS_MODULE, "driver_class");
    major = MAJOR(device_number);

    for (i = 0; i < NBR_MINORS; i++)
    {
        device = MKDEV(major, i);
        cdev_init(&devs[i].cdev, &fops);

        if (cdev_add(&devs[i].cdev, device, 1))
        {
            pr_err("cdev_add error");
            return -ENOMEM;
        }
        
        device_create(device_class, NULL, device, NULL, "device_driver%d", i);
        pr_info("Device created on /dev/%s%d\n", "device_driver", i);

        devs[i].size = 6;
        devs[i].buffer = kmalloc(sizeof(char) * 6, GFP_KERNEL | GFP_ATOMIC);
        memcpy(devs[i].buffer, "0123\n", 6);

        if (!devs[i].buffer)
        {
            return -ENOMEM;
        }
    }
    pr_debug("Initialization done\n");
    return 0;
}

/* When the module is unloaded */
static void __exit unregister_device(void)
{
    int i = 0;
    int major = 0;
    dev_t device = 0;

    pr_debug("Start termination\n");

    pr_debug("[cleanup_module]\n");
    major = MAJOR(device_number);

    for (i = 0; i < NBR_MINORS; i++)
    {
        device = MKDEV(major, i);
        cdev_del(&devs[i].cdev);
        device_destroy(device_class, device);

        kfree(devs[i].buffer);
        devs[i].size = 0;
    }

    class_destroy(device_class);
    unregister_chrdev_region(device_number, NBR_MINORS);

    pr_debug("Termination done\n");
}

module_init(register_device);
module_exit(unregister_device);
