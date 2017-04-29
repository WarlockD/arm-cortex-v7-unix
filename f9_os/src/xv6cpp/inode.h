/*
 * fs.h
 *
 *  Created on: Apr 18, 2017
 *      Author: warlo
 */

#ifndef XV6CPP_INODE_H_
#define XV6CPP_INODE_H_



#include <cstdint>
#include <sys\types.h>
#include <sys\stat.h>
#include <cstring>
#include "os.h"
#include "device.h"


namespace xv6 {
	static constexpr size_t NDIRECT = 12;
	static constexpr size_t ROOTINO =1 ; // root i-number
	static constexpr size_t FSBSIZE =512;  // fs block sizeblock size
	struct inode;
	struct pipe;

	struct file {
	    enum file_type { FD_NONE, FD_PIPE, FD_INODE };
	    file_type 	type;
	    char         readable;
	    char         writable;
	    uint32_t      off;
	    // we can union this
	    union {
	    	struct pipe  *pipe;
	    	struct inode *ip;
	    };
	    int stat(struct stat *st);
	    int read(uint8_t *addr, size_t n);
	    int write(const uint8_t *addr, size_t n);
	    void close();
	    static file* filealloc();
	};


	class inode {
	public:
		enum class FLAG {
			FREE =0,
			LOCK	=01,		/* inode is locked */
			UPD		=02,		/* inode has been modified */
			MOUNT	=010,		/* inode is mounted on */
			WANT	=020,		/* some process waiting on lock */
		};
		enum class MODE {
			FREE	=0,
			ALLOC	=0100000,
			//FMT		=060000,
				FDIR	=040000,
				FCHR	=020000,
				FBLK	=060000,
			LARG	=010000,
			SUID	=04000,
			SGID	=02000,
			SVTX	=01000,
			READ	=0400,
			WRITE	=0200,
			EXEC	=0100
		};
		ino_t inum() const { return _inum; }
		device* dev() const { return _dev; }
		enum_helper<FLAG> flag() const { return _flag; }
		virtual enum_helper<MODE> mode() const =0;
		virtual int nlink() const =0;
		virtual size_t size() const = 0;
		virtual uid_t uid() const = 0;
		virtual gid_t gid() const = 0;
		virtual time_t mtime() const =0;	// modify time
		virtual time_t atime() const =0;	// create time
		virtual off_t read(void* data, off_t offset, size_t size) = 0;
		virtual off_t write(void* data, off_t offset, size_t size) = 0;
		virtual void iput() =0; // save the node, delete it if links are o
		virtual void itrunc() =0; // delete the node, recurisivly
		inode() : _dev(device::NODEV), _inum(0), _flag(FLAG::FREE), _ref(0) {}


	protected:
		inode(device* dev, ino_t inum) : _dev(dev), _inum(inum), _flag(FLAG::FREE), _ref(1) {}
		device* _dev;
		ino_t _inum;
		enum_helper<FLAG> _flag;
		int _ref;
	};




} /* namespace xv6 */

#endif /* XV6CPP_INODE_H_ */
