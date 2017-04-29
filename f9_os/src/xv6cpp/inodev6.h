/*
 * inodev6.h
 *
 *  Created on: Apr 19, 2017
 *      Author: Paul
 */

#ifndef XV6CPP_INODEV6_H_
#define XV6CPP_INODEV6_H_
#include "buf.h"
#include <functional>
#include <thread>
#include "inode.h"


namespace xv6 {
class inode_v6 : public inode {
	public:
		static constexpr size_t BSIZE = 512; // block size
		static constexpr size_t BMASK = BSIZE-1; // block size
		static constexpr size_t ISIZE = 32;  // inode size
		static constexpr size_t IMASK = ISIZE-1;  // inode size
		static constexpr size_t DIRSIZ = 14; // sizee of a dir name
		static constexpr daddr_t ROOTNO = 1;
	protected:
		static constexpr size_t IPB = BSIZE/ISIZE; // i nodes per block
		// inode to sector
		static constexpr daddr_t itod(ino_t i) { return ((uint16_t)i+(ISIZE-1))/IPB; }
		// inode to sector offset
		static constexpr daddr_t itoo(ino_t i) { return ((uint16_t)i+(ISIZE-1))%IPB; }
		struct blkuse {
			uint16_t	size;	/* number of in core free blocks (0-100) */
			uint16_t	addr[100];	/* in core free blocks */
		}__ALLIGNED(BSIZE);
		struct superblock {
			uint16_t	isize;	/* size in blocks of I list */
			uint16_t	fsize;	/* size in blocks of entire volume */
			blkuse		free;	/* number of in core free blocks (0-100) */
			blkuse		inode;	/* in core free I nodes */
			uint8_t		flock;	/* lock during free list manipulation */
			uint8_t		ilock;	/* lock during I list manipulation */
			uint8_t		fmod;	/* super block modified flag */
			uint8_t		ronly;	/* mounted read-only flag */
			uint16_t	time[2];	/* current date of last update */
			uint16_t	pad[50];
		} __ALLIGNED(BSIZE);
		struct diinode {
			uint16_t mode;
			uint16_t nlink;
			uint8_t uid;
			uint8_t gid;
			uint8_t size0;
			uint16_t size1;
			uint16_t addr[8];
			uint16_t atime[2];
			uint16_t mtime[2];
		}__ALLIGNED(ISIZE);
		struct dirent {
			uint16_t ino;
			char name[14];
		} __ALLIGNED(DIRSIZ + sizeof(uint16_t));
		diinode _node;
	public:
		off_t read(void* data, off_t offset, size_t size);
		off_t write(void* data, off_t offset, size_t size);
		enum_helper<MODE> mode() const { return static_cast<MODE>(_node.mode); }
		int nlink() const { return _node.nlink; }
		size_t size() const { return _node.size0 << 16 | _node.size1;; }
		uid_t uid() const { return _node.uid; }
		gid_t gid() const  { return _node.gid; }
		time_t mtime() const { return _node.mtime[0] << 16 |  _node.mtime[1]; };
		time_t atime() const { return _node.atime[0] << 16 |  _node.atime[1]; };
		inode_v6() : inode() {}
		virtual ~inode_v6();
		static inode* iget(device* dev, ino_t inum);
		void iput() override;
		void itrunc() override;
	private:
	};

} /* namespace v6 */

#endif /* XV6CPP_INODEV6_H_ */
