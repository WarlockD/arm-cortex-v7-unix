#ifndef _V7_INODE_H_
#define _V7_INODE_H_

#include "v7_param.h"
#include <unistd.h>
#include <fcntl.h>

struct	dirent
{
	ino_t	d_ino;
	char	d_name[DIRSIZ];
};
#define NDIRENT (BSIZE/sizeof(struct dirent))

struct fblk
{
	int    	df_nfree;
	daddr_t	df_free[NICFREE];
};


/*
 * Structure of the super-block
 */
struct	filsys {
	unsigned short s_isize;	/* size in blocks of i-list */
	daddr_t	s_fsize;   	/* size in blocks of entire volume */
	short  	s_nfree;   	/* number of addresses in s_free */
	daddr_t	s_free[NICFREE];/* free block list */
	short  	s_ninode;  	/* number of i-nodes in s_inode */
	ino_t  	s_inode[NICINOD];/* free i-node list */
	char   	s_flock;   	/* lock during free list manipulation */
	char   	s_ilock;   	/* lock during i-list manipulation */
	char   	s_fmod;    	/* super block modified flag */
	char   	s_ronly;   	/* mounted read-only flag */
	time_t 	s_time;    	/* last super block update */
	/* remainder not maintained by this version of the system */
	daddr_t	s_tfree;   	/* total free blocks*/
	ino_t  	s_tinode;  	/* total free inodes */
	short  	s_m;       	/* interleave factor */
	short  	s_n;       	/* " " */
	char   	s_fname[6];	/* file system name */
	char   	s_fpack[6];	/* file system pack name */
};

/*
 * Inode structure as it appears on
 * a disk block.
 */
struct dinode
{
	unsigned short	di_mode;     	/* mode and type of file */
	short	di_nlink;    	/* number of links to file */
	short	di_uid;      	/* owner's user id */
	short	di_gid;      	/* owner's group id */
	off_t	di_size;     	/* number of bytes in file */
	char  	di_addr[40];	/* disk block addresses */
	time_t	di_atime;   	/* time last accessed */
	time_t	di_mtime;   	/* time last modified */
	time_t	di_ctime;   	/* time created */
};
#define	INOPB	8	/* 8 inodes per block */
/*
 * the 40 address bytes:
 *	39 used; 13 addresses
 *	of 3 bytes each.
 */

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

#define	NADDR	13
#define	NINDEX	15


struct	inode
{
	char	i_flag;
	char	i_count;	/* reference count */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	unsigned short	i_mode;
	short	i_nlink;	/* directory entries */
	short	i_uid;		/* owner */
	short	i_gid;		/* group of owner */
	off_t	i_size;		/* size of file */
	union {
		struct {
			daddr_t i_addr[NADDR];	/* if normal file/directory */
			daddr_t	i_lastr;	/* last logical block read (for read-ahead) */
		};
		struct	{
			daddr_t	i_rdev;			/* i_addr[0] */
			//struct	group	i_group;	/*  multiplexor group file */
		};
	} i_un;
};

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
struct	mount
{
	dev_t	m_dev;		/* device mounted */
	struct buf *m_bufp;	/* pointer to superblock */
	struct inode *m_inodp;	/* pointer to mounted on inode */
};

/*
 * One file structure is allocated
 * for each open/creat/pipe call.
 * Main use is to hold the read/write
 * pointer associated with each open
 * file.
 */
struct	file
{
	char	f_flag;
	char	f_count;	/* reference count */
	char	f_desc;		// file discrpitor number
	LIST_ENTRY(file) f_list;	// is it really worth the extra 8 bytes?
	struct inode *f_inode;	/* pointer to inode structure */
	union {
		off_t	f_offset;	/* read/write character pointer */
		struct chan *f_chan;	/* mpx channel pointer */
	} f_un;
};

#define	FPIPE	04
// intresting FPIPE dosn't exisit in fcntl
// but that bit is not set either
/* flags */
#define	FAPPEND		_FAPPEND
#define	FSYNC		_FSYNC
#define	FASYNC		_FASYNC
#define	FNBIO		_FNBIO
#define	FNONBIO		_FNONBLOCK	/* XXX fix to be NONBLOCK everywhere */
#define	FNDELAY		_FNDELAY
#define	FREAD		_FREAD
#define	FWRITE		_FWRITE
#define	FMARK		_FMARK
#define	FDEFER		_FDEFER
#define	FSHLOCK		_FSHLOCK
#define	FEXLOCK		_FEXLOCK
#define	FOPEN		_FOPEN
#define	FCREAT		_FCREAT
#define	FTRUNC		_FTRUNC
#define	FEXCL		_FEXCL
#define	FNOCTTY		_FNOCTTY

/* flags */
#define	ILOCK	01		/* inode is locked */
#define	IUPD	02		/* file has been modified */
#define	IACC	04		/* inode access time to be updated */
#define	IMOUNT	010		/* inode is mounted on */
#define	IWANT	020		/* some process waiting on lock */
#define	ITEXT	040		/* inode is pure text prototype */
#define	ICHG	0100		/* inode has been changed */

/* modes */
#define	IFMT	0170000		/* type of file */
#define		IFDIR	0040000	/* directory */
#define		IFCHR	0020000	/* character special */
#define		IFBLK	0060000	/* block special */
#define		IFREG	0100000	/* regular */
#define		IFMPC	0030000	/* multiplexed char special */
#define		IFMPB	0070000	/* multiplexed block special */
#define	ISUID	04000		/* set user id on execution */
#define	ISGID	02000		/* set group id on execution */
#define ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read, write, execute permissions */
#define	IWRITE	0200
#define	IEXEC	0100


struct filsys * getfs (dev_t dev);
void plock (struct inode *ip);
void prele (struct inode *ip);
void iupdat (struct inode *ip, time_t *ta, time_t *tm);
void iput(struct inode*ip);
struct inode* iget(dev_t dev, ino_t ino);
void itrunc (struct inode *ip);
int writei(struct inode *ip, const void* base, off_t offset, size_t count);
int readi(struct inode *ip, void* base, off_t offset, size_t count);
//int writep(struct file *fp, const void* base, off_t offset, size_t count);
int readp(struct file *fp, void* base,  size_t count, off_t offset);
struct inode* namei(const char* path, int mode);
void ffree(struct file* fp);
void openi (struct inode *ip, int rw);

void 	v7_close(int fdesc);
int 	v7_read (int fdesc, caddr_t data, size_t count);
int 	v7_write(int fdesc, caddr_t data, size_t count);
off_t 	v7_seek (int fdesc, off_t	off,  int	 sbase);
int 	v7_open (const char	*fname, int mode,...);
int v7_stat (int fdes, struct stat *sb);
int v7_fstat (const char *fname, struct stat *sb);

#endif

