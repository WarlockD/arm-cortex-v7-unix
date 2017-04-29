#ifndef _XV6_FS_H_
#define _XV6_FS_H_


#include "param.h"
#include "types.h"
// On-disk file system format.
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

#define ROOTINO 1  // root i-number
#define FSBSIZE 512  // fs block sizeblock size

struct fat_superblock {
	uint8_t		fs_type;	/* FAT sub type */
	uint8_t		flag;		/* File status flags */
	uint8_t		csize;		/* Number of sectors per cluster */
	uint8_t		pad1;
	uint16_t	n_rootdir;	/* Number of root directory entries (0 on FAT32) */
	uint32_t	n_fatent;	/* Number of FAT entries (= number of clusters + 2) */
	uint32_t	fatbase;	/* FAT start sector */
	uint32_t    max_clust; // max amout of clusters
	uint32_t	dirbase;	/* Root directory start sector (Cluster# on FAT32) */
	uint32_t	database;	/* Data start sector */
	uint32_t	fptr;		/* File R/W pointer */
	uint32_t	fsize;		/* File size */
	uint32_t	org_clust;	/* File start cluster */
	uint32_t	curr_clust;	/* File current cluster */
	uint32_t	dsect;		/* File current data sector */
};

struct fat_dir {
	uint16_t	index;		/* Current read/write index number */
	struct buf* fn; 		/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
	//BYTE*	fn;			/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
	uint32_t	sclust;		/* Table start cluster (0:Static table) */
	uint32_t	clust;		/* Current cluster */
	uint32_t	sect;		/* Current sector */
};

struct fat_finfo{
	uint32_t	fsize;		/* File size */
	uint16_t	fdate;		/* Last modified date */
	uint16_t	ftime;		/* Last modified time */
	uint8_t		fattrib;	/* Attribute */
	char		fname[13];	/* File name */
} ;

// File system super block
struct superblock {
    uint32_t    size;           // Size of file system image (blocks)
    uint32_t    nblocks;        // Number of data blocks
    uint32_t    ninodes;        // Number of inodes.
    uint32_t    nlog;           // Number of log blocks
} __ALLIGNED32;
#if 0
struct	mount
{
	dev_t	dev;				/* device mounted */
	struct buf	*sb_buf;		/* pointer to superblock */
	struct inode* ip;
	TAILQ_HEAD(,buf) list;		// blocks tied to mount
	int	*m_inodp;	/* pointer to mounted on inode */
};
#endif

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint32_t))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
	uint16_t   	type;           // File type
    uint16_t   	nlink;          // Number of links to inode in file system
    uint32_t    size;           // Size of file (bytes)
    uint32_t    addrs[NDIRECT+1]; // Data block addresses
} __ALLIGNED(32);  // 32 bytes an entry

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

#define BBLOCK_START(sb) 3 /* bitmap block starting location */


// Block containing bit for block b
#define BBLOCK(b, ninodes) (((b)/BPB) + ((ninodes)/IPB) + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
    uint16_t  inum;
    char    name[DIRSIZ];
} __ALLIGNED32;

#endif
