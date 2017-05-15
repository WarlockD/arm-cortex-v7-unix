#ifndef _FS_HPP_
#define _FS_HPP_

#include "types.hpp"
#include <os\atomic.h>
namespace fs {
	static constexpr bool is_sep_char(int c) { return c == '/' || c == '\\'; }
	static size_t dirlen(const char* str);
	static int dircmp(const char* a, const char* b);
	static const char* skip_sep(const char* str);
	class dir_string {
		const char* _str;
	public:
		dir_string() : _str("") {}
		dir_string(const char* str) : _str(skip_sep(str)) {}
		dir_string operator=(const char* str) { return dir_string(str); }
		operator const char*() const { return _str; }
		size_t size() const { return dirlen(_str); }
		const char& operator[](int i) const { return _str[i]; }
		const char* c_str() const { return _str; }
		bool empty() const { return *_str == '\0'; }
		bool operator==(const dir_string& r) const { return _str == r._str || dircmp(_str,r._str) == 0; }
		bool operator!=(const dir_string& r) const { return !(*this == r); }
		bool operator<(const dir_string& r) const { return dircmp(_str,r._str) < 0; }
		int compare(const char* r) const { return dircmp(_str,r); }
		int compare(const dir_string& r) const { return dircmp(_str,r._str); }
		dir_string nextname()
		{
			const char* name = _str;
			while (*name && !is_sep_char(*name)) name++;
			if(*name) name++; // skip over the sep
			return dir_string(name);
		}
		dir_string basename(){
			dir_string base;
			dir_string name = *this;
			do {
				name = name.nextname();
				if(name.empty()) break;
				base = name;
			} while(1);
			return base;
		}
	};
struct inode;
struct pollfd;
	/* This structure represents one inode in the Nuttx pseudo-file system */

	struct inode
	{
		static inode* g_root;
	  inode *i_peer;     /* Link to same level inode */
	  inode *i_child;    /* Link to lower level inode */
	  int16_t           i_crefs;    /* References to inode */
	  uint16_t          i_flags;    /* Flags for inode */
	//  union inode_ops_u u;          /* Inode operations */
	//#ifdef CONFIG_FILE_MODE
	  mode_t            i_mode;     /* Access mode flags */
	//#endif
	  void         *i_private;  /* Per inode driver private data */
	  char          i_name[32];  /* Name of inode (variable) */

	  inode *search(const char **path,inode **peer,inode **parent,const char **relpath);
	  void addref() { ARM::atomic_add(&i_crefs,1); }
	  void subref() { ARM::atomic_sub(&i_crefs,1); }
	 static void insert(inode *node,inode *peer,inode *parent);
	public:
	 inode(const char* name);
	  const char* name() const { return i_name; }
	  bool is_mountpt() const { return false; }
	  inode* find(const char *path, const char **relpath);

	  bool operator==(const inode& n) const { return this == &n || dircmp(name(), n.name()) == 0; }
	  bool operator!=(const inode& n) const { return !(*this == n); }
	  bool operator<(const inode& n) const { return dircmp(name(), n.name())<0; }
	  static void operator delete(void* ptr);
	  static void* operator new(std::size_t sz);
	};

#if 0
	struct file
	{
	  int               f_oflags;   /* Open mode flags */
	  off_t             f_pos;      /* File position */
	  inode *f_inode;    /* Driver interface */
	  void             *f_priv;     /* Per file driver private data */
	  /* The device driver open method differs from the mountpoint open method */

	  virtual int open()=0;

	  /* The following methods must be identical in signature and position because
	   * the struct file_operations and struct mountp_operations are treated like
	   * unions.
	   */

	  virtual int     close( struct file *filep)=0;
	  virtual ssize_t read(char *buffer, size_t buflen)=0;
	  virtual ssize_t write(   const char *buffer, size_t buflen)=0;
	  virtual off_t   seek( off_t offset, int whence)=0;
	  virtual int     ioctl(  int cmd, unsigned long arg)=0;

	  /* The two structures need not be common after this point */

	#ifndef CONFIG_DISABLE_POLL
	  virtual int     poll( pollfd *fds, bool setup)=0;
	#endif
	#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	  virtual int     unlink(struct inode *inode)=0;
	#endif
	  virtual ~file() {}
	};
#endif
};

#endif

