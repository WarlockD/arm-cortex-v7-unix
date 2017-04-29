#ifndef _MXSYS_INODE_H_
#define _MXSYS_INODE_H_

#include "types.h"
#include "param.h"


/*
 * The I node is the focus of all
 * file activity in unix. There is a unique
 * inode allocated for each active file,
 * each current directory, each mounted-on
 * file, text file, and the root. An inode is 'named'
 * by its dev/inumber pair. (iget/iget.c)
 * Data, from mode on, is read in
 * from permanent inode on volume.
 */
/* flags */
struct filsys;

enum inode_flags {
ILOCK	=01,		/* inode is locked */
IUPD	=02,	/* inode has been modified */
IACC	=04,		/* inode access time to be updated */
IMOUNT	=010,		/* inode is mounted on */
IWANT	=020,		/* some process waiting on lock */
ITEXT	=040,		/* inode is pure text prototype */
ICHG	=0100,		/* inode has been changed */
};
enum inode_modes {
	/* modes */
	IALLOC	=0100000,		/* file is used */
	IFMT	=060000,		/* type of file */
		IFDIR	=040000,	/* directory */
		IFCHR	=020000,	/* character special */
		IFBLK	=060000,	/* block special, 0 is regular */
		IFREG	=0100000,	/* regular */
		IFMPC	=0030000,	/* multiplexed char special */
		IFMPB	=0070000,	/* multiplexed block special */
	ILARG	=010000	,	/* large addressing algorithm */
	ISUID	=04000,		/* set user id on execution */
	IREAD	=0400,		/* read, write, execute permissions */
	IWRITE	=0200,
	IEXEC	=0100,
};

struct	inode
{
	enum inode_flags	i_flag;
	enum inode_modes	i_mode;
	uint16_t	i_count;	/* reference count */
	dev_t		i_dev;		/* device where inode resides */
	ino_t		i_number;	/* i number, 1-to-1 with device address */
	uint16_t	i_nlink;	/* directory entries */
	uid_t		i_uid;		/* owner */
	gid_t		i_gid;		/* group of owner */
	size_t		i_size;
	uint16_t	i_addr[8];	/* device addresses constituting file */
};// inode[NINODE];
extern struct inode inode[NINODE]; // refrence of all inodes



/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
struct	mount
{
	dev_t			m_dev;		/* device mounted */
	struct buf* 		m_bufp;	/* pointer to superblock */
	struct inode*	m_inodp;	/* pointer to mounted on inode */
};
extern struct mount mount[NMOUNT]; // refrence of all inodes
void iinit();
struct buf * balloc(dev_t dev);
void bfree(dev_t dev,daddr_t bno);
struct inode* ialloc(dev_t dev);
void ifree(dev_t dev, ino_t ino);
struct filsys*  getfs(dev_t dev);
void iupdate();
struct inode* iget(dev_t dev, ino_t ino);
void iput(struct inode* ip);
void  iupdat(struct inode*ip);
void  prele(struct inode*ip); // pipe release but used for other locking
void itrunc(struct inode*ip);
struct inode* maknode(mode_t mode);
void wdir(struct inode* ip,const char* name);
void prele(struct inode* rp);
daddr_t bmap(struct inode *ip, daddr_t bn);
struct inode *namei(int flag);
// redwri.c
void readi( struct inode *ip);
void writei( struct inode *ip);
void iomove(struct buf* bp, off_t on,size_t n,int flag);
// fio.c // file?

#endif
