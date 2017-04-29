#ifndef _XV6_FILE_H_
#define _XV6_FILE_H_

#include "types.h"
#include "param.h"
#include "fs.h"
#include <sys\queue.h>

// memory inode system
struct minode {
	LIST_ENTRY(minode) peers;
	// a MI_FILE is basicly a memory file buffer
	enum { MI_NONE, MI_PIPE, MI_MOUNT, MI_DEV, MI_DIR, MI_FILE } type;
	dev_t dev;
	ino_t ino;
	mode_t mode;
	uint32_t size;
	uint32_t nlinks;
	const char* name;
	int ref;		// ref count
	union {
		LIST_HEAD(,minode) head;
		void* file;
	};
};
struct fat_node {

};
struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE } type;
    int          ref;   // reference count
    char         readable;
    char         writable;
    uint32_t      off;
    // we can union this
    union {
    	struct pipe  *pipe;
    	struct inode *ip;
    };
};


// in-memory copy of an inode
struct inode {
    dev_t    dev;        // Device number
    uint32_t    inum;       // Inode number
    int     	ref;        // Reference count
    int     	flags;      // I_BUSY, I_VALID
    short   	type;       // copy of disk inode
    short   	nlink;
    uint32_t    size;
    uint32_t    addrs[NDIRECT+1];
};
#define I_BUSY 0x1
#define I_VALID 0x2

// table mapping major device number to
// device functions
#if 0
struct devsw {
    int (*read) (struct inode*, char*, int);
    int (*write)(struct inode*, char*, int);
};
extern struct devsw devsw[];
#endif

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.

struct file* getfs(int fd);
#define CONSOLE 1

#endif
