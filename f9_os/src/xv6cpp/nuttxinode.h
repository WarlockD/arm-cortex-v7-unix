/*
 * nuttxinode.h
 *
 *  Created on: Apr 24, 2017
 *      Author: warlo
 */

#ifndef XV6CPP_NUTTXINODE_H_
#define XV6CPP_NUTTXINODE_H_

#include "os.h"
#include "list.h"
#include "bitmap.h"
namespace nuttx {
	class nami_cache {

	};
	constexpr static size_t  PATH_MAX = 254;
	enum class FSNODEFLAG_TYPE : uint16_t {
		TYPE_MASK 		= 0x00000007, /* Isolates type field        */
			DRIVER   	= 0x00000000, /*   Character driver         */
			BLOCK    	= 0x00000001, /*   Block driver             */
			MOUNTPT  	= 0x00000002, /*   Mount point              */
		SPECIAL  		= 0x00000004, /* Special OS type            */
			NAMEDSEM 	= 0x00000004 ,/*   Named semaphore          */
			MQUEUE   	= 0x00000005, /*   Message Queue            */
			SHM      	= 0x00000006, /*   Shared memory region     */
			SOFTLINK 	= 0x00000007, /*   Soft link                */
		DELETED   		= 0x00000008, /* Unlinked                   */
	};
	class inode {

	public:
		  /* Inode i_flag values:
		   *
		   *   Bit 0-3: Inode type (Bit 4 indicates internal OS types)
		   *   Bit 4:   Set if inode has been unlinked and is pending removal.
		   */
		bool is_type(FSNODEFLAG_TYPE t) const { return bitops::maskflag(i_flags,FSNODEFLAG_TYPE::TYPE_MASK) == t; }
		bool is_special(FSNODEFLAG_TYPE t) const { return bitops::maskflag(i_flags,FSNODEFLAG_TYPE::SPECIAL) == t; }
		bool is_driver() const { return is_type(FSNODEFLAG_TYPE::DRIVER); }
		bool is_block() const { return is_type(FSNODEFLAG_TYPE::BLOCK); }
		bool is_mountpt() const { return is_type(FSNODEFLAG_TYPE::MOUNTPT); }
		bool is_namedsem() const { return is_type(FSNODEFLAG_TYPE::NAMEDSEM); }
		bool is_mqueue() const { return is_type(FSNODEFLAG_TYPE::MQUEUE); }
		bool is_sgm() const { return is_type(FSNODEFLAG_TYPE::SHM); }
		bool is_softlink() const { return is_type(FSNODEFLAG_TYPE::SOFTLINK); }

		constexpr static inline bool dir_sep(char c) { return c == '\\' || c == '/'; }
		constexpr static inline bool dir_end(char c) { return dir_sep(c) || c == '\0'; }
		static int dir_compare(const char *fname, const char *nname);
		static const char *dir_basename(const char *name);
		static const char *dir_nextname(const char *name);
		  static inode *g_root_inode;
		  struct inode_search
		  {
			const char *path;      /* Path of inode to find */
			inode *node;    /* Pointer to the inode found */
			inode *peer;    /* Node to the "left" for the found inode */
			inode *parent;  /* Node "above" the found inode */
			char *relpath;   /* Relative path into the mountpoint */
#ifdef CONFIG_PSEUDOFS_SOFTLINKS
			const char *linktgt;   /* Target of symbolic link if linked to a directory */
			char *buffer;          /* Path expansion buffer */
			bool nofollow;             /* true: Don't follow terminal soft link */
			inode_search(const char* path, bool nofollow=false) : path(path), node(nullptr), peer(nullptr), parent(nullptr), relpath(nullptr) ,
					linktgt(nullptr), buffer(nullptr),nofollow(nofollow) {}
			~inode_search() { if(buffer) delete buffer; }
#else
			inode_search(const char* path, bool nofollow=false) : path(path), node(nullptr), peer(nullptr), parent(nullptr), relpath(nullptr) { (void)nofollow;}
			~inode_search() {}
#endif
		  };
		  typedef int (*foreach_inode_t)(  inode *node,
		                                  char dirpath[PATH_MAX],
		                                  void *arg);
		  static int search(inode_search &desc); // caller has to hold the g_root_inode lock
		  static int find(inode_search &desc);  // locks the node
		  static void semtake();
		  static void semgive();
		  static const char* nextname(const char* name);
		  static int reserve(const char *path, inode **n);
		  static inode *unlink(const char *path);
		  static int remove(const char *path);
		  void addref();
		  void release();
		  static  void initialize();
		  static int falloc(int offlags, off_t pos, int minfd);
		  static void fclose(int fd);
		  static void frelease(int fd);
		  int stat(struct stat* buf);
		  void free();
protected:
		  static int _search(struct inode_search *desc);
		  inode *i_peer;     /* Link to same level inode */
		  inode *i_child;    /* Link to lower level inode */
		  int16_t           i_crefs;    /* References to inode */
		  FSNODEFLAG_TYPE          i_flags;    /* Flags for inode */
		  mode_t            i_mode;     /* Access mode flags */
		  void        	 	*i_private;  /* Per inode driver private data */
		  const char*		i_name;		/* Name of inode (variable) */
	};
	class nuttx_inode {
	public:
		constexpr static inline bool dir_sep(char c) { return c == '\\' || c == '/'; }
		constexpr static inline bool dir_end(char c) { return dir_sep(c) || c == '\0'; }
		static int dir_compare(const char *fname, const char *nname);
		static const char *dir_basename(const char *name);
		static const char *dir_nextname(const char *name);
		/* Inode i_flag values */
		enum class type {
			deleted=0, /* Unlinked     */
			driver, /*   Character driver         */
			block, /*   Block driver             */
			mountpt, /*   Character driver         */
			special, /* Special OS type            */
			nsemaphore, /*   Named semaphore          */
			mqueue, /*   Character driver         */
			smr, /*   Shared memory region     */
		};
		class iterator {
			nuttx_inode* _node;
		public:
			iterator(nuttx_inode* node) : _node(node) {}
			bool operator==(const iterator& l) const { return _node == l._node; }
			bool operator!=(const iterator& l) const { return _node == l._node; }
			iterator operator++() {
				if(_node) _node = LIST_NEXT(_node,_peer);
				return iterator(_node);
			}
			iterator operator++(int){
				iterator t(_node);
				if(_node) _node = LIST_NEXT(_node,_peer);
				return t;
			}
			nuttx_inode* operator->() const { return _node; }
			operator nuttx_inode*() { return _node;}
		};
		nuttx_inode(const char* name, type t=type::special);
		iterator begin() { return iterator(LIST_FIRST(&_child)); }
		iterator end() { return iterator(nullptr); }

		void release(); // releases this inode
		~nuttx_inode();
		inline const char* name() const { return _name; }
		inline type flags() const { return _flags; }
		inline mode_t mode() const { return _mode; }
		nuttx_inode *find(const char *path, const char **relpath=nullptr,nuttx_inode** parent=nullptr);
		static nuttx_inode *root_find(const char *path, const char **relpath);
		nuttx_inode * link(const char* name, type t=type::special);
		void link(nuttx_inode* n);
		nuttx_inode * unlink(const char* path);
		void unlink();
		void addref() { bitops::atomic_add(1,&_crefs); }
		void* operator new(size_t size);
		void operator delete(void *ptr);
	protected:
		static void ifree(nuttx_inode* n); // releases the node, back to the pool
		int16_t			_crefs;    /* References to inode */
		type			_flags;    /* Flags for inode */
		mode_t			_mode;	 /* Access mode flags */
		char			_name[32];/* Name of inode (variable) */
		LIST_CLASS_HEAD(,nuttx_inode) _child;    /* Link to lower level inode */
		LIST_CLASS_ENTRY(nuttx_inode) _peer; /* Link to same level inode */

	};

} /* namespace xv6 */

#endif /* XV6CPP_NUTTXINODE_H_ */
