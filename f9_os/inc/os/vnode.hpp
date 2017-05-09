/*
 * vnode.hpp
 *
 *  Created on: May 3, 2017
 *      Author: Paul
 */

#ifndef OS_VNODE_HPP_
#define OS_VNODE_HPP_

#include "types.hpp"
#include "list.hpp"

#include "driver.hpp"

namespace os {
	typedef char* caddr_t;
	class vnode;
	enum class vtype  { VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };
	enum class vtagtype   {
	    VT_NON, VT_UFS, VT_NFS, VT_MFS, VT_PC, VT_LFS, VT_LOFS, VT_FDESC,
	    VT_PORTAL, VT_NULL, VT_UMAP, VT_KERNFS, VT_PROCFS, VT_AFS, VT_ISOFS,
	    VT_UNION
	};
	enum class vflags {
		VROOT    =   0x0001,  /* root of its file system */
		VTEXT    =   0x0002,  /* vnode is a pure text prototype */
		VSYSTEM  =   0x0004,  /* vnode being used by kernel */
		VISTTY   =  0x0008,  /* vnode represents a tty */
		VXLOCK    =  0x0100,  /* vnode is locked to change underlying type */
		VXWANT    =  0x0200,  /* process is waiting for vnode */
		VBWAIT    =  0x0400,  /* waiting for output to complete */
		VALIASED  =  0x0800 , /* vnode has an alias */
		VDIROP    =  0x1000,  /* LFS: vnode is involved in a directory op */
	};


	/*
	 * Encapsulation of namei parameters.
	 */
	struct nameidata {
	    /*
	     * Arguments to namei/lookup.
	     */
	    caddr_t ni_dirp;            /* pathname pointer */
	     /* u_long  ni_nameiop;        namei operation */
	     /* u_long  ni_flags;          flags to namei */
	     /* struct  proc *ni_proc;     process requesting lookup */
	    /*
	     * Arguments to lookup.
	     */
	     /* struct  ucred *ni_cred;    credentials */
	     vnode *ni_startdir; /* starting directory */
	      vnode *ni_rootdir;  /* logical root directory */
	    /*
	     * Results: returned from/manipulated by lookup
	     */
	      vnode *ni_vp;       /* vnode of result */
	      vnode *ni_dvp;      /* vnode of intermediate directory */
	    /*
	     * Shared between namei and lookup/commit routines.
	     */
	    size_t   ni_pathlen;         /* remaining chars in path */
	    char    *ni_next;           /* next location in pathname */
	    size_t  ni_loopcnt;         /* count of symlinks encountered */
	    /*
	     * Lookup parameters: this structure describes the subset of
	     * information from the nameidata structure that is passed
	     * through the VOP interface.
	     */
	    struct componentname {
	        /*
	         * Arguments to lookup.
	         */
	    	size_t  cn_nameiop;     /* namei operation */
	    	size_t  cn_flags;       /* flags to namei */
	       // struct  proc *cn_proc;  /* process requesting lookup */
	       // struct  ucred *cn_cred; /* credentials */
	        /*
	         * Shared between lookup and commit routines.
	         */
	        char    *cn_pnbuf;      /* pathname buffer */
	        char    *cn_nameptr;    /* pointer to looked up name */
	        long    cn_namelen;     /* length of looked up component */
	        size_t  cn_hash;        /* hash value of looked up name */
	        long    cn_consume;     /* chars to consume in lookup() */
	    } ni_cnd;
	};
	class vnode {
		const char* _name; // vnode name, could be filename, etc
		vflags _flags;
		uint16_t _usecount;
		uint16_t _writecount;
		list::entry<vnode> _link;
		friend class mount_driver;
	public:


		//friend struct  list_head<vnode, vnode::_link>
		//friend class mount;

	};

	/*
	 * File identifier.
	 * These are unique per filesystem on a single machine.
	 */


	struct fid {
		constexpr static size_t MAXFIDSZ   = 16;
	    uint16_t     fid_len;            /* length of data in bytes */
	    uint16_t     fid_reserved;       /* force longword alignment */
	    char        fid_data[MAXFIDSZ]; /* data (variable length) */
	};

	class mount_driver {
		using vnodelist = list::head<vnode, &vnode::_link> ;
		friend vnode;
		friend list::entry<vnode>;
	  ///  CIRCLEQ_ENTRY(mount) mnt_list;      /* mount list */
	   // struct vfsops   *mnt_op;            /* operations on fs */

	   // vfsconf  *mnt_vfc;           /* configuration info */
	    vnode    *mnt_vnodecovered;  /* vnode we mounted on */
	    vnodelist mnt_vnodelist;     /* list of vnodes this mount */
	  //  struct lock     mnt_lock;           /* mount structure lock */
	    int             mnt_flag;           /* flags */
	    int             mnt_maxsymlinklen;  /* max size of short symlink */
	   // struct statfs   mnt_stat;           /* cache of filesystem stats */
	    void*         mnt_data;           /* private data */

	    int mount(char *path, caddr_t data,nameidata &ndp) { return -1; }
	    int start(int flags) { return -1; }
	    int root(int flags, vnode **vpp) { return -1; }
	    int quotactl(int cmds, uid_t uid, caddr_t arg) { return -1; }
	    int statfs(struct statfs *sbp) { return -1; }
	    int sync(int waitfor) { return -1; }
	    int vget(ino_t ino, vnode **vpp) { return -1; }
	   // int fhtovp(fid *fhp,
        //         mbuf *nam, struct vnode **vpp,
         //       int *exflagsp, struct ucred **credanonp));
	     int vptofh(vnode *vp,  fid *fhp);
	     int init(){ return -1; }
	     int sysct(int *, uint32_t, void *, size_t *, void *, size_t) { return -1; }
	};



	/* states */
	#define DK_CLOSED       0       /* drive is closed */
	#define DK_WANTOPEN     1       /* drive being opened */
	#define DK_WANTOPENRAW  2       /* drive being opened */
	#define DK_RDLABEL      3       /* label being read */
	#define DK_OPEN         4       /* label read, drive open */
	#define DK_OPENRAW      5       /* open without label */
	struct dkdriver {
	    void    (*d_strategy) __P((struct buf *));
	#ifdef notyet
	    int     (*d_open) __P((dev_t dev, int ifmt, int, struct proc *));
	    int     (*d_close) __P((dev_t dev, int, int ifmt, struct proc *));
	    int     (*d_ioctl) __P((dev_t dev, u_long cmd, caddr_t data, int fflag,
	                            struct proc *));
	    int     (*d_dump) __P((dev_t));
	    void    (*d_start) __P((struct buf *, daddr_t));
	    int     (*d_mklabel) __P((struct dkdevice *));
	#endif
	};
} /* namespace os */



#endif /* OS_VNODE_HPP_ */
