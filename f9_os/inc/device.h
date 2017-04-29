/*
 * device.h
 *
 *  Created on: Mar 6, 2017
 *      Author: warlo
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/queue.h>

struct file;
struct inode;
struct file_operations
{
  /* The device driver open method differs from the mountpoint open method */
	  int     (*open)(struct file *filep);
	  int     (*ioctl)(struct file *filep, int cmd, unsigned long arg);
	  int     (*close)(struct file *filep);
	  int     (*read)(struct file *filep, char *buffer, size_t buflen);
	  int     (*write)(struct file *filep, const char *buffer, size_t buflen);
	  int     (*seek)(struct file *filep, int offset, int whence);
	  int     (*ioctl)(struct file *filep, int cmd, unsigned long arg);

	  /* The two structures need not be common after this point */

	#ifndef CONFIG_DISABLE_POLL
	  int     (*poll)(struct file *filep, struct pollfd *fds, bool setup);
	#endif
	#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	  int     (*unlink)(struct inode *inode);
};
#define INODE_NAME_LEN 5
struct inode
{
	LIST_ENTRY(inode) i_peer;      /* vnodes for mount point */
	LIST_HEAD(,inode) children;
   struct inode *i_peer;     /* Link to same level inode */
   struct inode *i_child;    /* Link to lower level inode */
  int16_t           i_crefs;    /* References to inode */
  uint16_t          i_flags;    /* Flags for inode */
  union {
	  struct file_operations file_ops;
  }u;
//  union inode_ops_u u;          /* Inode operations */
  mode_t            i_mode;     /* Access mode flags */
  void              *i_private;  /* Per inode driver private data */

  //char              i_name[1];  /* Name of inode (variable) */
  char				i_name[INODE_NAME_LEN];
};
struct block_operations
{
  int     (*open)(struct inode *inode);
  int     (*close)(struct inode *inode);
  int 	  (*read)(struct inode *inode, unsigned char *buffer,
            size_t start_sector, unsigned int nsectors);
  int     (*write)(struct inode *inode, const unsigned char *buffer,
            size_t start_sector, unsigned int nsectors);
  int     (*geometry)(struct inode *inode, struct geometry *geometry);
  int     (*ioctl)(struct inode *inode, int cmd, unsigned long arg);
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  int     (*unlink)(struct inode *inode);
#endif
};

#endif /* DEVICE_H_ */
