#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/init.h>      /* for __init and __exit macros */
#include <linux/slab.h>     /* for kmalloc() */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

#include "message_slot.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amit Nissenbaum");
MODULE_DESCRIPTION("A message slot device driver");

// The message the device will give when asked
static struct device_msg* dev;

// creates a new device struct with given minor number
int create_device(struct device_msg** d, unsigned int minor_number){
    *d = kmalloc(sizeof(device_msg), GFP_KERNEL);
    if(*d == NULL)
      return FAILURE;
    (*d)->minor_number = minor_number;
    (*d)->channels = NULL;
    (*d)->next = NULL;
    return SUCCESS;
}

void free_all_devices(void){
  struct device_msg* d = dev;
  struct device_msg* d_free;
  struct channel* c;
  struct channel* c_free;

  while(d != NULL){
    c = d->channels;

    while( c != NULL ){
      c_free = c;
      c = c->next;
      kfree(c_free);
    }

    d_free = d;
    d = d->next;
    kfree(d_free);
  }
}

device_msg* search_device(unsigned int minor_number){
  struct device_msg* d = dev;
  while(d != NULL){
    if( d->minor_number == minor_number ){
      return d;
    }
    d=d->next;
  }
  return NULL;
}


channel* search_channel(struct device_msg* d, unsigned long channel_id){
  struct channel* c = d->channels;
  
  while( c != NULL){
    if( c->channel_id == channel_id ){
      return c;
    }
    c = c->next;
  }
    
  return NULL;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode, struct file* file ){

  pd* data = kmalloc(sizeof(pd), GFP_KERNEL);

  if(data == NULL)
    return FAILURE;

  // set channel ID to 0 (not set - not a real channel) and censorship mode to 0
  file->private_data = data;
  data->channel_id = 0;
  data->minor_number = iminor(inode);
  data->censorship_mode = 0;

  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode, struct file* file){
  pd *data = (pd*)file->private_data;
  kfree(data);
  return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_write( struct file*  file, 
                            const char __user*  buffer, 
                            size_t        length, 
                            loff_t*       offset ){

  struct device_msg* d;
  struct channel* c;
  ssize_t i;
  pd* data = (pd*)file->private_data;
  int censorship_mode = data->censorship_mode;

  if(data->channel_id == 0)
    return -EINVAL;

  if (length == 0 || length > BUF_LEN)
      return -EMSGSIZE;

  d = search_device(data->minor_number);
  if (d == NULL) {
    if (create_device(&d, data->minor_number) == FAILURE)
        return -FAILURE;  
    d->next = dev;
    dev = d;
}

  c = search_channel(d, data->channel_id);

  if(c == NULL){
    c = (channel*)kmalloc(sizeof(channel), GFP_KERNEL);
    if(c == NULL)
      return FAILURE;
    c->channel_id = data->channel_id;
    c->message_length = 0; 
    c->next = d->channels;
    d->channels = c;
  }
                       
  for( i = 0; i < length && i < BUF_LEN; ++i ) {
    if (get_user(c->message[i], &buffer[i]) != 0)
    return -EINVAL;
    if( 1 == censorship_mode && i%4 == 3 )
      c->message[i] = '#';
  }

  c->message_length = length;
  return i;
}

//---------------------------------------------------------------
        
static ssize_t device_read( struct file*       file,
                            char __user* buffer,
                            size_t             length,
                            loff_t*            offset){
        
  pd* data = (pd*)file->private_data;
  struct device_msg* d = search_device(data->minor_number);
  struct channel* c = (d == NULL) ? NULL : search_channel(d, data->channel_id);

  ssize_t i;

  if(data->channel_id == 0){
    return -EINVAL;
  }

  if(c == NULL)
    return -EWOULDBLOCK;

  if (length < c->message_length)
    return -ENOSPC;
  
  for( i = 0; i < c->message_length; ++i ) {
    if (put_user(c->message[i], &buffer[i]) != 0)
    return -EINVAL;
  }

  return i;
}

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param ){

  pd* data = (pd*)file->private_data;
  if( ioctl_command_id == MSG_SLOT_CHANNEL ){
    // sets the file descriptors channel id (passed through ioctl_param)
    if(ioctl_param == 0)
      return -EINVAL;
    data->channel_id = ioctl_param;
  }
  else if( ioctl_command_id == MSG_SLOT_SET_CEN ){
    // sets the file descriptors encryption flag
    if(ioctl_param != 0 && ioctl_param != 1)
      return -EINVAL;
    
    data->censorship_mode = ioctl_param;
  }
  else{
    return -EINVAL;
  }

  return SUCCESS;
}

struct file_operations Fpos = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,                                               // CAN I DELETE?
};

// Initialize the module
static int __init message_slot_init(void){
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fpos );
    if( rc < 0 ){
      printk(KERN_ERR "%s registraion failed for  %d\n",
                         DEVICE_FILE_NAME, MAJOR_NUM );
      return FAILURE;
    }

    // initialize messages linked list structure to NULL
    dev = NULL;

    return SUCCESS;
}

static void __exit message_slot_cleanup(void){
    // Unregister the device
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

    // Free any allocated memory
    free_all_devices();

}
         
module_init(message_slot_init);
module_exit(message_slot_cleanup);