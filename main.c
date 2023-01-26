/*
More info:

https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html#waiting-queues
http://embeddedguruji.blogspot.com/2019/01/linux-character-driver-creating.html
https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
https://sysprog21.github.io/lkmpg/

*/

#include <linux/version.h> /* LINUX_VERSION_CODE, KERNEL_VERSION */
#include <linux/compiler.h> /* __must_check */

#include <linux/kernel.h> /* printk, pr_err, pr_debug */
#include <linux/init.h>
#include <linux/module.h> /* THIS_MODULE */
#include <linux/fs.h>
#include <linux/cdev.h> /* cdev */
#include <asm/ioctl.h>
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

struct my_device_data
{
    struct cdev cdev;
    // Device data
    char *buffer;
    ssize_t size;
};

struct my_device_data devs[NBR_MINORS];

static dev_t device_number;
struct class *device_class;

static int open_device(struct inode *inode, struct file *file)
{
    struct my_device_data *my_data = &devs[iminor(inode)];

    pr_debug("[open_device]\n");

    file->private_data = my_data;

    return 0;
}

static int close_device(struct inode *inode, struct file *file)
{
    pr_debug("[close_device]\n");

    return 0;
}

static ssize_t read_device(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data =
        (struct my_device_data *)file->private_data;
    ssize_t len = min((ssize_t)(my_data->size - *offset), (ssize_t)size);

    pr_debug("[read_device]\n");

    if (len <= 0)
    {
        return 0;
    }

    if (copy_to_user(user_buffer, my_data->buffer + *offset, len))
    {
        pr_err("copy_to_user error");
        return -EFAULT;
    }
    
    *offset += len;
    return len;
}

static ssize_t write_device(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data *my_data = (struct my_device_data *)file->private_data;
    ssize_t len = min((ssize_t)(my_data->size - *offset), (ssize_t)size);

    pr_debug("[write_device]\n");
    if (len <= 0)
    {
        return 0;
    }
    
    if (size != my_data->size) {
        if(my_data->buffer = krealloc(my_data->buffer, sizeof(char) * size, GFP_KERNEL | GFP_ATOMIC))
        {
            return -ENOMEM;
        }

        my_data->size = size;
    }

    if (copy_from_user(my_data->buffer + *offset, user_buffer, len))
    {
        pr_err("copy_from_user error");
        return -EFAULT;
    }

    *offset += len;
    return len;
}

struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = open_device,
    .read = read_device,
    .write = write_device,
    .release = close_device,
};

static int __init register_device(void)
{
    int i = 0;
    int major = 0;
    dev_t my_device = 0;

    pr_debug("Start initialization\n");

    if (alloc_chrdev_region(&device_number, 0, NBR_MINORS, "my_device_driver"))
    {
        pr_err("alloc_chrdev_region error");
        return -ENOMEM;
    }

    device_class = class_create(THIS_MODULE, "my_driver_class");
    major = MAJOR(device_number);

    for (i = 0; i < NBR_MINORS; i++)
    {
        my_device = MKDEV(major, i);
        cdev_init(&devs[i].cdev, &my_fops);

        if (cdev_add(&devs[i].cdev, my_device, 1))
        {
            pr_err("cdev_add error");
            return -ENOMEM;
        }
        device_create(device_class, NULL, my_device, NULL, "my_device_driver%d", i);
        pr_info("Device created on /dev/%s%d\n", "my_device_driver", i);

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

static void __exit unregister_device(void)
{
    int i = 0;
    int major = 0;
    dev_t my_device = 0;

    pr_debug("Start termination\n");

    pr_debug("[cleanup_module]\n");
    major = MAJOR(device_number);

    for (i = 0; i < NBR_MINORS; i++)
    {
        my_device = MKDEV(major, i);
        cdev_del(&devs[i].cdev);
        device_destroy(device_class, my_device);

        kfree(devs[i].buffer);
        devs[i].size = 0;
    }

    class_destroy(device_class);
    unregister_chrdev_region(device_number, NBR_MINORS);

    pr_debug("Termination done\n");
}

module_init(register_device);
module_exit(unregister_device);
