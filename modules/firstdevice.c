#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/semaphore.h>
#include<asm/uaccess.h>
//1.create a structure for our fake device 
struct fake_device{
	char data[10];
	struct semaphore sem;
} virtual_device;

//2.to later register our device we need a cdev object and some other variables 

//Why not static?
struct cdev *mcdev;
int major_number; // Do we really need a global major number and ret
int ret;

dev_t dev_num;
#define DEVICE_NAME    "solidusdevice"

int device_open(struct inode *inode, struct file *filp){
if(down_interruptible(&virtual_device.sem) !=0){
	printk(KERN_ALERT"COULD NOT LOCK DEVICE DURING OPEN");
	return -1;

}
printk(KERN_INFO"opened device");
return 0;

}
static int char_device_read(struct file *file, char *buf,size_t lbuf,
loff_t *ppos)
{
            int maxbytes; /* number of bytes from ppos to MAX_LENGTH */
            int bytes_to_do; /* number of bytes to read */
            int nbytes; /* number of bytes actually read */
 
            maxbytes = 10 - *ppos;
           
            if( maxbytes > lbuf ) bytes_to_do = lbuf;
            else bytes_to_do = maxbytes;
           
            if( bytes_to_do == 0 )
{
                        printk("Reached end of device\n");
                        return -ENOSPC; /* Causes read() to return EOF */
            }
           
            nbytes = bytes_to_do -
                         copy_to_user( buf, /* to */
                                           virtual_device.data + *ppos, /* from */
                                           bytes_to_do ); /* how many bytes */
            *ppos += nbytes;
            return nbytes;   
}




static int char_device_write(struct file *file,const char *buf,
                                               size_t lbuf,loff_t *ppos)
{
            int nbytes; /* Number of bytes written */
            int bytes_to_do; /* Number of bytes to write */
            int maxbytes; /* Maximum number of bytes that can be written */
 
            maxbytes = 10 - *ppos;
 
            if( maxbytes > lbuf ) bytes_to_do = lbuf;
            else bytes_to_do = maxbytes;
 
            if( bytes_to_do == 0 )
      {
                        printk("Reached end of device\n");
                        return -ENOSPC; /* Returns EOF at write() */
            }
 
            nbytes = bytes_to_do -
                     copy_from_user( virtual_device.data + *ppos, /* to */
                                                 buf, /* from */
                                                 bytes_to_do ); /* how many bytes */
            *ppos += nbytes;
            return nbytes;
}



int device_close(struct inode *inode, struct file *filp){
up(&virtual_device.sem);
printk(KERN_INFO "closed device");
return 0;

}




struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = char_device_write,
	.read = char_device_read

};

static int driver_entry(void){
	//register our device with the system:a two step process
	//1.use dynamic allocation to assign our device


	ret = alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME);
	if(ret < 0){
		printk(KERN_ALERT"failed to allocate a major number");
	return ret;
}

major_number = MAJOR(dev_num);
printk(KERN_INFO "major number is %d",major_number);
printk(KERN_INFO "\tuse \"mknod /dev/%s c  %d 0\" for device file",DEVICE_NAME,major_number);

mcdev = cdev_alloc();
mcdev->ops = &fops;
mcdev->owner = THIS_MODULE;
ret = cdev_add(mcdev,dev_num,1);
if(ret<0){
	printk(KERN_ALERT "unable to add cdev to kernel");
	return ret;
}

sema_init(&virtual_device.sem,1);


return 0;
}

static void driver_exit(void){
cdev_del(mcdev);
unregister_chrdev_region(dev_num,1);
printk(KERN_ALERT "unloaded module");

}

module_init(driver_entry);
module_exit(driver_exit);
