#ifndef _MXSYS_FILE_H_
#define _MXSYS_FILE_H_

#include "types.h"
#include "param.h"


struct	file
{
	mode_t			f_flag;
	int				f_desc;		// file descrptor
	uint8_t			f_count;	/* reference count */
	struct	inode*	f_inode;	/* pointer to inode structure */
	off_t 			f_offset; /* read/write character pointer */
};
//extern struct file file[NFILE]; // refrence of all inodes
struct file * getf(int f);
void closef(struct file * fp);
void closei(struct inode* ip, int rw);
void openi(struct inode* ip, int rw);
int iaccess(struct inode* ip, int m);
struct inode * owner();
int suser();

struct file* falloc();
#endif
