#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/string.h>

#define MODULE_NAME "avm"
#define STORAGE_SIZE 1024
#define TIME_INTERVAL_IN_MS 1000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fedy0");
MODULE_DESCRIPTION("AVM: Kernel module to store and retrieve data to and from userspace, with kernel log of 1 word/s");

int ret;
dev_t dev_num;
struct cdev *avm_cdev;
struct char_device {
    char data[STORAGE_SIZE];
    struct semaphore s;
} avm_device;

static char *string;
static char *token;
size_t storage_length = 0;
struct word_node {
    char *word;
    struct list_head list;
};
struct word_node *new_node;

static struct timer_list device_timer;
static struct list_head word_list = LIST_HEAD_INIT(word_list);
static struct list_head *current_word = &word_list;

void device_timer_handler(struct timer_list * data) {
    struct word_node *entry;
    
    entry = list_entry(current_word->next, struct word_node, list);
    if (entry->word != NULL) {
        printk(KERN_INFO MODULE_NAME ": %s\n", entry->word);
    }
    else {
        list_first_entry(current_word, struct word_node, list);
        current_word = current_word->next;
        entry = list_entry(current_word->next, struct word_node, list);
        printk(KERN_INFO MODULE_NAME ": %s\n", entry->word);
    }

    current_word = current_word->next;
    
    /* Restart the timer */
    mod_timer( &device_timer, jiffies + msecs_to_jiffies(TIME_INTERVAL_IN_MS)); 
}

static int device_open(struct inode *inode, struct file *file)
{
    if (down_interruptible(&avm_device.s) != 0) {
        printk(KERN_ALERT MODULE_NAME ": could not lock device, open operation failed!");
        return -EFAULT;
    }
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    up(&avm_device.s);
    return 0;
}

static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
    if (length > STORAGE_SIZE)
        return -EINVAL;

    if (copy_from_user(avm_device.data, buffer, length))
        return -EFAULT;

    storage_length = length;
    printk(KERN_INFO MODULE_NAME ": Stored %zu bytes of text\n", length);
    
    string = kstrdup(buffer, GFP_KERNEL);
    printk(KERN_INFO MODULE_NAME  ": Original string: '%s'", string);

    while( (token = strsep(&string," ")) != NULL ) {
        new_node = kmalloc(sizeof(struct word_node), GFP_KERNEL);
        if (!new_node) {
            return -ENOMEM;
        }
        new_node->word = kstrdup(token, GFP_KERNEL);
        INIT_LIST_HEAD(&new_node->list);
        list_add_tail(&new_node->list, &word_list);
    }
    
    return length;
}

static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
    ssize_t bytes_to_copy = min(storage_length, length);

    if (bytes_to_copy <= 0)
        return 0;

    if (copy_to_user(buffer, avm_device.data, bytes_to_copy))
        return -EFAULT;
        
    printk(KERN_INFO MODULE_NAME  ": Data copied to user");

    return bytes_to_copy;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .write = device_write,
    .read = device_read,
};

static int __init text_storage_init(void)
{
    int ret = alloc_chrdev_region(&dev_num, 0, 1, MODULE_NAME);
    if (ret < 0) {
        printk(KERN_ERR MODULE_NAME ": Failed to register a device\n");
        return ret;
    }
    
    printk(KERN_INFO MODULE_NAME  ": The avm device major number is %d\n", MAJOR(dev_num));
    
    avm_cdev = cdev_alloc();
    avm_cdev->ops = &fops;
    ret = cdev_add(avm_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ALERT MODULE_NAME ": failed to add device to kernel");
    }
    
    sema_init(&avm_device.s, 1);
    
    timer_setup(&device_timer, device_timer_handler, 0);
    mod_timer(&device_timer, jiffies + msecs_to_jiffies(TIME_INTERVAL_IN_MS));
    
    printk(KERN_INFO MODULE_NAME ": Module loaded\n");
    return 0;
}

static void __exit text_storage_exit(void)
{
    struct word_node *entry, *tmp;

    list_for_each_entry_safe(entry, tmp, &word_list, list) {
        list_del(&entry->list);
        kfree(entry->word);
        kfree(entry);
    }
    cdev_del(avm_cdev);
    unregister_chrdev_region(dev_num, 0);
    del_timer(&device_timer);

    printk(KERN_INFO MODULE_NAME ": Module unloaded\n");
}

module_init(text_storage_init);
module_exit(text_storage_exit);

