#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<asm/uaccess.h>
/*--------Headers necessary for module programming.--------------
-init.h: required for initialization of module. 
-module.h: required to let kernel know that it is an LKM.
-linux/fs.h: required to get functions related to device driver coding. 
-asm/uaccess.h: required to get data from userspace to kernel and vice versa. 
*/

#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WILL CHRISTIE");

static char device_buffer[BUFFER_SIZE];

// count variables for open/close functions.
int open_count = 0;
int close_count = 0;

/*
-------------simple_char_driver_read, simple_char_driver_write: takes 4 params.------------ 
-file *pfile: file pointer
-char __user *buffer: user space buffer for storing read data. 
-size_t length: available user space buffer. 
-loff_t *offset: current position fo the opened file. 
*/
ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/* length is the length of the userspace buffer*/
	/* current position of the opened file*/
	/* copy_to_user function. source is device_buffer (the buffer defined at the start of the code) and destination is the userspace buffer *buffer */
	/* Get the size of what we are to copy from the device_buffer */
	int dbuff_size = strlen(device_buffer);
	/* If there's nothing in our device_buffer (*offset is 0), there's nothing to read so return 0 */
	if (dbuff_size == 0){
		printk(KERN_ALERT "The device buffer is currently empty");
		return 0;
	}
	/* Copy the data from offset to the end of the buffer */
	printk(KERN_ALERT "Reading from device\n");
	copy_to_user(buffer, device_buffer, dbuff_size);
	/* Print out the amount of bytes that is in the device buffer */
	printk(KERN_ALERT "Device has read %d bytes\n", dbuff_size);
	
	return 0;
}

ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{

	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/* length is the length of the userspace buffer*/
	/* current position of the opened file*/
	/* copy_from_user function. destination is device_buffer (the buffer defined at the start of the code) and source is the userspace 		buffer *buffer */
	/* Get the size of the user space buffer that we are writing from */
	int dbuff_size = strlen(device_buffer);
	/* If the user space buffer is empty, there's nothing to write to return 0 */
	if (length == 0){
		return 0;
	}
	/* Set the offset to the current size of the device buffer so we don't overwrite what's currently there */
	*offset = dbuff_size;
	/* Write from the user space buffer to the device buffer */
	printk(KERN_ALERT "Writing to device\n");
	copy_from_user(device_buffer + *offset, buffer, length);
	/* Print out the amount of bytes that the user wrote */
	printk(KERN_ALERT "Device has written %zu bytes", strlen(buffer));
	return length;
}

/*
--------------simple_char_driver_open/close: takes 2 params--------------
-inode which represents physical file on the disk
-abstract open file which contains all necessary file operations in the file_operations struct.
*/
int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	printk(KERN_ALERT "Device is open\n");
	open_count++;
	printk(KERN_ALERT "Device has been opened %d times\n", open_count);
	return 0;
}

int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	printk(KERN_ALERT "Device is closed\n");
	close_count++;
	printk(KERN_ALERT "Device has been closed %d times\n", close_count);
	return 0;
}

/* ------FILE OPERATIONS STRUCT--------
create similar structure to that found in file_operations from lib/modules/$(uname-r)/build/include/linux/fs.h
more info found at The Linux Kernel Module Programming Guide: http://www.tldp.org/LDP/lkmpg/2.4/html/c577.htm
*/
struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
	.open 	 = simple_char_driver_open,
	.release = simple_char_driver_close,
	.read	 = simple_char_driver_read,
	.write	 = simple_char_driver_write,

};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	/* register the device */
	printk(KERN_ALERT "Within %s function\n",__FUNCTION__);
	register_chrdev(247, "simple_character_device", &simple_char_driver_file_operations);
	return 0;
}

static int simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	/* unregister  the device using the register_chrdev() function. */
	printk(KERN_ALERT "Within %s function\n",__FUNCTION__);
	unregister_chrdev(247, "simple_character_device");
	return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
