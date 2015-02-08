/* ----------------------------------------------- QUEUE driver --------------------------------------------------
 
						A driver which writes to the queue and reads the value from the queue.
 
 ----------------------------------------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
//#include <asm/msr.h>
#include <linux/semaphore.h>
#include <asm/errno.h> 
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#define DEVICE_NAME_                 "Queue"  							/* device name to be created and registered */
#define QUEUE_LENGTH					10


/*********** Element is the structure of the message which is identical to the one in User space.*******************/
typedef struct { 
	int msg_id;
	int src_id;
	int dest_id;
	char value[10]; 
} Element;        


/********************* Per device structure *****************************************/
struct QUEUE {
	struct semaphore sem;
	char 		name[6];  				/* To store the name of the device */
	struct cdev cdev;          
    int         start;  				/* Write pointer to the queue      */             
    int         end; 					/* Read pointer to the queue       */
    Element 	elem[10];				/* Creating 10 queues              */
    int num_elements_in_queue;          /* Indicate s the number of elements in the shared queue */
} *QUEUE_p;

static dev_t QUEUE_number;      		/* Allotted device number 	 */
struct class *QUEUE_class;          	/* Tie with the device model */
static struct device *QUEUE_device;


/************************ Opening Queue driver *************************************/

int QUEUE_driver_open(struct inode *inode, struct file *file)
{
	
	struct QUEUE *QUEUE_p;	
	QUEUE_p = container_of(inode->i_cdev, struct QUEUE, cdev);				/* Get the per-device structure that contains this cdev */
	file->private_data = QUEUE_p;											/* Easy access to cmos_devp from rest of the entry points */
	return 0;
}

 /*********************** Release QUEUE driver *************************************/
 
int QUEUE_driver_release(struct inode *inode, struct file *file)
{

	struct QUEUE *QUEUE_p = file->private_data;	
	return 0;
}

 /********************* Driver write function i.e. writing to the Queue **************/
 
ssize_t QUEUE_driver_write(struct file *file, const char *buf,
                              size_t count, loff_t *ppos)
{
	struct QUEUE *QUEUE_p = file->private_data;
	int ret;
	down(&QUEUE_p->sem);							    /* This semaphore function locks the Write function to prevent access to other threads*/
	
	if(QUEUE_p->num_elements_in_queue == QUEUE_LENGTH)	/* Checking whether queue is full and unlock the function if yes*/
	{
		printk("\n Sorry Buffer is full");
		up(&QUEUE_p->sem);						   
		return -EINVAL;
		
	}	
	else
	{	
		gpio_set_value(149,1);
		msleep(500);
		gpio_set_value(149,0);
		msleep(500);	
		ret = copy_from_user((char *)&QUEUE_p->elem[QUEUE_p->end] , buf, count);
		QUEUE_p->end = (QUEUE_p->end +1) % QUEUE_LENGTH;                   			/* Incrementing the write pointer and wrapping around when full */
		QUEUE_p->num_elements_in_queue++;								   			/* Incrementing the number of elements in the queue */		
	}
	
	up(&QUEUE_p->sem);							    	/* This semaphore function unlocks the Write function so that other threads can access it */
	return 0;
}


 /******************* Receiver read function i.e. reading from the Queue **************/

ssize_t QUEUE_driver_read(struct file *file, char *buf,size_t count, loff_t *ppos)
{
	struct QUEUE *QUEUE_p = file->private_data;
	int ret, res;
	down(&QUEUE_p->sem);
	
	if(QUEUE_p->num_elements_in_queue == 0)		   	 /* Checking whether queue is empty and unlock the function if yes */
	{
		//printk("\n Sorry Buffer is empty");
		up(&QUEUE_p->sem);
		return -EINVAL;
	}	
	else
	{
		ret = (int)sizeof(QUEUE_p->elem[QUEUE_p->start]);					     	/* Copies the data from kernel space to user space */
		res = copy_to_user(buf,(char *)&QUEUE_p->elem[QUEUE_p->start], sizeof(QUEUE_p->elem[QUEUE_p->start]));
		QUEUE_p->start = (QUEUE_p->start +1) % QUEUE_LENGTH;
		QUEUE_p->num_elements_in_queue--;	
	}	
	up(&QUEUE_p->sem);
	return (ret) ;
}

/********************************** File operations structure. Defined in linux/fs.h ********************/
static struct file_operations QUEUE_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= QUEUE_driver_open,        /* Open method */
    .release	= QUEUE_driver_release,     /* Release method */
    .write		= QUEUE_driver_write,       /* Write method */
    .read		= QUEUE_driver_read,        /* Read method */
};

 
 /********************************** Initializing the driver ********************************************/
int __init QUEUE_driver_init(void)
{
	int ret,i,j;
	const char *DEVICE_NAME[4] = {"bus_in_q", "bus_out_q", "bus_out_q", "bus_out_q"};  						/* Names of the devices */

	/****************** Request dynamic allocation of a device major number **************/
	if (alloc_chrdev_region(&QUEUE_number, 0, 4, DEVICE_NAME_) < 0) 
	{
			return -1;
	}

	/************************** Populate sysfs entries **********************************/
	QUEUE_class = class_create(THIS_MODULE, DEVICE_NAME_);

	/********************* Allocate memory for the per-device structure *****************/
	QUEUE_p = kmalloc(4*sizeof(struct QUEUE), GFP_KERNEL);
	
		
	if (!QUEUE_p) 
	{
		return -ENOMEM;
	}

	for(j=0;j<4;j++)
	{
		/**************************** Request I/O region *******************************/
		sprintf(QUEUE_p[j].name,"%s%d",DEVICE_NAME[j],j);

		/**************** Connect the file operations with the cdev ********************/
		cdev_init(&QUEUE_p[j].cdev, &QUEUE_fops);
		QUEUE_p[j].cdev.owner = THIS_MODULE;

		/********************** Connect the major/minor number to the cdev ************/
		ret = cdev_add(&QUEUE_p[j].cdev,MKDEV(MAJOR(QUEUE_number),j), 1);

		if (ret) 
		{
			//printk("\n Bad cdev");
			return ret;
		}

		/******************** Send uevents to udev, so it'll create /dev nodes ********/
		QUEUE_device = device_create(QUEUE_class, NULL, MKDEV(MAJOR(QUEUE_number), j), NULL,"%s%d", DEVICE_NAME[j],j);	
		
		/********** Initializing the write and read pointer and also the number of elements in queue to zero ********************/
		QUEUE_p[j].start = 0;
		QUEUE_p[j].end = 0;
		QUEUE_p[j].num_elements_in_queue = 0;

		/************************* Initialization of the semaphore ********************/
		sema_init(&QUEUE_p[j].sem, 1);
	}
	
	/******************************* Initializing the data field of the message structure to NULL *******************************/
	for(i = 0; i<10;i++)
	{
		memset(&QUEUE_p->elem[i].value, '\0',sizeof(QUEUE_p->elem[i].value));
	}	
	
	printk("\n QUEUE driver is initialized.");
	return 0;
}


/************************************** Queue driver Exit *************************************************/
void __exit QUEUE_driver_exit(void)
{
	int i;

	/**************************** Release the major number ******************************/
	unregister_chrdev_region((QUEUE_number), 1);

	/******************************* Destroy device ************************************/
	for(i=0;i<4;i++)
	{
		device_destroy (QUEUE_class, MKDEV(MAJOR(QUEUE_number), i));
		cdev_del(&QUEUE_p[i].cdev);
    }

	kfree(QUEUE_p);
	
	/******************************Destroy driver_class ********************************/
	class_destroy(QUEUE_class);

	printk("\n QUEUE driver removed.");
}

module_init(QUEUE_driver_init);
module_exit(QUEUE_driver_exit);
MODULE_LICENSE("GPL v2");

