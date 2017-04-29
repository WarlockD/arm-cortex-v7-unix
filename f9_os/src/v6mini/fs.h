#ifndef _MXSYS_FS_H_
#define _MXSYS_FS_H_

#include "types.h"
#include "param.h"

/*
 * Definition of the unix super block.
 * The root super block is allocated and
 * read in iinit/alloc.c. Subsequently
 * a super block is allocated and read
 * with each mount (smount/sys3.c) and
 * released with unmount (sumount/sys3.c).
 * A disk block is ripped off for storage.
 * See alloc.c for general alloc/free
 * routines for free list and I list.
 */
struct	filsys
{
	uint16_t	s_isize;	/* size in blocks of I list */
	uint16_t	s_fsize;	/* size in blocks of entire volume */
	uint16_t	s_nfree;	/* number of in core free blocks (0-100) */
	uint16_t	s_free[100];	/* in core free blocks */
	uint16_t	s_ninode;	/* number of in core I nodes (0-100) */
	uint16_t	s_inode[100];	/* in core free I nodes */
	uint8_t		s_flock;	/* lock during free list manipulation */
	uint8_t		s_ilock;	/* lock during I list manipulation */
	uint8_t		s_fmod;		/* super block modified flag */
	uint8_t		s_ronly;	/* mounted read-only flag */
	uint16_t	s_time[2];	/* current date of last update */
	uint16_t	pad[50];
};

/*
 * Inode structure as it appears on
 * the disk. Not used by the system,
 * but by things like check, df, dump.
 */
struct	dinode
{
	uint16_t	i_mode;
	uint8_t		i_nlink;
	uint8_t		i_uid;
	uint8_t		i_gid;
	uint8_t		i_size0;
	uint16_t	i_size1;
	uint16_t	i_addr[8];
	uint32_t	i_atime;
	uint32_t	i_mtime;
} __attribute__((aligned(32)));

#define DINODESZ 32 /* is this right? */
struct	dirent {			/* current directory entry */
	uint16_t		d_ino;
	char			d_name[DIRSIZ];
};
// helper union for each disk block
union fsblk {
	char 		  block[BSIZE];						 // raw data
	uint16_t	  indr[BSIZE/sizeof(uint16_t)];		 // indirect inodes
	struct dirent  dir[BSIZE/sizeof(struct dinode)];	// dir entry block
	struct dinode dinode[BSIZE/sizeof(struct dinode)];// node block
	struct {
		uint16_t	nfree;							/* number of in core free blocks (0-100) */
		uint16_t	free[100];						/* in core free blocks */
	} free;
	struct filsys fs;								// super block
};

// I think we can use sys/stat here, have these values changed in 50 odd years?
#if 0
/* modes */
#define	IALLOC	0100000
#define	IFMT	060000
#define		IFDIR	040000
#define		IFCHR	020000
#define		IFBLK	060000
#define	ILARG	010000
#define	ISUID	04000
#define	ISGID	02000
#define ISVTX	01000
#define	IREAD	0400
#define	IWRITE	0200
#define	IEXEC	0100
#endif


#endif
