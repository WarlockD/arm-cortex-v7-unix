//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//
#include <sys\stat.h>
#include <fcntl.h>
#include "types.h"
#include "defs.h"
#include "param.h"

#include "proc.h"
#include "fs.h"
#include "file.h"
#include <errno.h>
#include <sys\unistd.h>
#include "sysfile.h"
#include "tty.h"

#undef errno
extern int errno;

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int getfd(int fd, int *pfd, struct file **pf)
{
	struct file* f= NULL;
    if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)  return -1;
    if(pfd) *pfd = fd;
    if(pf) *pf = f;
    return 0;
}
#if 0
static int argfd(int n, int *pfd, struct file **pf)
{
    int fd;
    if(argint(n, &fd) < 0)  return -1;
    return getfd(fd,pfd,pf);

}
#endif
// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int fdalloc(struct file *f)
{
    int fd;

    for(fd = 0; fd < NOFILE; fd++){
        if(proc->ofile[fd] == 0){
            proc->ofile[fd] = f;
            return fd;
        }
    }

    return -1;
}
// Is the directory dp empty except for "." and ".." ?
static int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
        if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
            panic("isdirempty: readi");
        }

        if(de.inum != 0) {
            return 0;
        }
    }
    return 1;
}
static struct inode* search(const char* path) {
    uint32_t off;
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if((dp = nameiparent(path, name)) == NULL)  return NULL;
    ilock(dp);
    if((ip = dirlookup(dp, name, &off)) != NULL)  {
    	iunlockput(dp);
    	ilock(ip);
    	return ip;
    }
    iunlockput(dp);
    return NULL;
}
static struct inode* create(const char *path, short type, dev_t dev)
{
    uint32_t off;
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if((dp = nameiparent(path, name)) == 0) {
        return 0;
    }

    ilock(dp);

    if((ip = dirlookup(dp, name, &off)) != 0){
        iunlockput(dp);
        ilock(ip);

        if(S_ISREG(type) && S_ISREG(ip->type) ) {
            return ip;
        }

        iunlockput(ip);

        return 0;
    }

    if((ip = ialloc(dp->dev, type)) == 0) {
        panic("create: ialloc");
    }

    ilock(ip);
    ip->dev = dp->dev;
    ip->nlink = 1;
    iupdate(ip);
    if(S_ISCHR(type) || S_ISBLK(type)) {
    	ip->addrs[0] = dev;
    	iupdate(ip);
    }
    else if(S_ISDIR(type)){  // Create . and .. entries.
        dp->nlink++;  // for ".."
        iupdate(dp);

        // No ip->nlink++ for ".": avoid cyclic ref count.
        if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0) {
            panic("create dots");
        }
    }

    if(dirlink(dp, name, ip->inum) < 0) {
        panic("create: dirlink");
    }

    iunlockput(dp);

    return ip;
}

int SYSFILE_FUNC(read) (int fd, char *p, int n)
{
    struct file *f;
    if(getfd(fd, 0, &f) < 0)return -1;
    return fileread(f, p, n);
}

int SYSFILE_FUNC(write)(int fd, char *p, int n)
{
    struct file *f;
    if(getfd(fd, 0, &f) < 0)return -1;
    return filewrite(f, p, n);
}
int SYSFILE_FUNC(lseek)(int fd, int offset, int rdir)
{
    struct file *f;
    if(getfd(fd, 0, &f) < 0)return -1;


	if( f->type != FD_INODE || S_ISFIFO(f->ip->type) ){
		errno = ESPIPE;
		return -1;
	}
	off_t noff = 0;
	switch(rdir){
	case SEEK_SET:	/* set file offset to offset */
		noff = offset;
		break;
	case SEEK_CUR:	/* set file offset to current plus offset */
		noff = f->off + offset;
		break;
	case SEEK_END:	/* set file offset to EOF plus offset */
		noff = offset + f->ip->size;
		break;
	}
	if(noff < 0)
		noff = 0;
	else
		if(noff > f->ip->size)
			noff = f->ip->size;
	f->off  = noff;
	return noff;
}

int SYSFILE_FUNC(close)(int fd)
{
    struct file *f;
    if(getfd(fd, 0, &f) < 0)return -1;

    proc->ofile[fd] = 0;
    fileclose(f);

    return 0;
}

int SYSFILE_FUNC(open)(char *path, int omode, ...)
{
	int fd;
	struct file *f;
	struct inode *ip;

	if(omode & O_CREAT){
		begin_trans();
		ip = create(path, S_IFREG, 0);
		commit_trans();

		if(ip == 0) {
			return -1;
		}

	} else {
		if((ip = namei(path)) == 0) {
			return -1;
		}

		ilock(ip);

		if(S_ISDIR(ip->type) && omode != O_RDONLY){
			iunlockput(ip);
			return -1;
		}
	}

	if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
		if(f) {
			fileclose(f);
		}

		iunlockput(ip);
		return -1;
	}

	iunlock(ip);

	f->type = FD_INODE;
	f->ip = ip;
	f->off = 0;
	f->readable = !(omode & O_WRONLY);
	f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

	return fd;
}
int SYSFILE_FUNC(stat)(const char *file, struct stat *st)
{
	struct inode* ip;
	if((ip=search(file))) {
		stati(ip, st);
		iunlock(ip);
		return 0;
	}
	return -1;
}
int SYSFILE_FUNC(fstat)(int fd, struct stat *st)
{
    struct file *f;
    if(getfd(fd, 0, &f) < 0)return -1;
    return filestat(f, st);
}

int SYSFILE_FUNC(dup)(int fd)
{
    struct file *f;
    if(getfd(fd, 0, &f) < 0)return -1;
    if((fd=fdalloc(f)) < 0) return -1;
    filedup(f);
    return fd;
}



// Create the path new as a link to the same inode as old.
int SYSFILE_FUNC(link)(const char* oldp, const char* newp)
{
    char name[DIRSIZ];
    struct inode *dp, *ip;


    if((ip = namei(oldp)) == 0) {
        return -1;
    }

    begin_trans();

    ilock(ip);

    if(S_ISDIR(ip->type)){
        iunlockput(ip);
        commit_trans();
        return -1;
    }

    ip->nlink++;
    iupdate(ip);
    iunlock(ip);

    if((dp = nameiparent(newp, name)) == 0) {
        goto bad;
    }

    ilock(dp);

    if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
        iunlockput(dp);
        goto bad;
    }

    iunlockput(dp);
    iput(ip);

    commit_trans();

    return 0;

    bad:
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    commit_trans();
    return -1;
}



//PAGEBREAK!
int SYSFILE_FUNC(unlink)(const char* path)
{
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ];
    uint32_t off;

    if((dp = nameiparent(path, name)) == 0) {
        return -1;
    }

    begin_trans();

    ilock(dp);

    // Cannot unlink "." or "..".
    if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0) {
        goto bad;
    }

    if((ip = dirlookup(dp, name, &off)) == 0) {
        goto bad;
    }

    ilock(ip);

    if(ip->nlink < 1) {
        panic("unlink: nlink < 1");
    }

    if(S_ISDIR(ip->type) && !isdirempty(ip)){
        iunlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));

    if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
        panic("unlink: writei");
    }

    if(S_ISDIR(ip->type)){
        dp->nlink--;
        iupdate(dp);
    }

    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    commit_trans();

    return 0;

    bad:
    iunlockput(dp);
    commit_trans();
    return -1;
}


int SYSFILE_FUNC(mkdir)(const char* path)
{
    struct inode *ip;

    begin_trans();

    if((ip = create(path, _IFDIR, 0)) == 0){
        commit_trans();
        return -1;
    }

    iunlockput(ip);
    commit_trans();

    return 0;
}

int SYSFILE_FUNC(mknod)(const char* path, int mode, dev_t dev)
{
    struct inode *ip;
    begin_trans();

    if((ip = create(path, mode, dev)) == 0){
        commit_trans();
        return -1;
    }

    iunlockput(ip);
    commit_trans();

    return 0;
}

int  SYSFILE_FUNC(chdir)(const char* path)
{
    struct inode *ip;
    if( (ip = namei(path)) == 0) return -1;

    ilock(ip);

    if(S_ISDIR(ip->type)){
        iunlockput(ip);
        return -1;
    }

    iunlock(ip);

    iput(proc->cwd);
    proc->cwd = ip;

    return 0;
}

int SYSFILE_FUNC(exec)(const char* path, const char* args)
{
	// fix this
   // return exec(path, argv);
	return -1;
}

int SYSFILE_FUNC(pipe)(int fd[2])
{
    struct file *rf, *wf;
    int fd0, fd1;

    if(pipealloc(&rf, &wf) < 0) return -1;
    fd0 = -1;

    if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
        if(fd0 >= 0) {
            proc->ofile[fd0] = 0;
        }
        fileclose(rf);
        fileclose(wf);
        return -1;
    }

    fd[0] = fd0;
    fd[1] = fd1;
    return 0;
}
/*
 * The routine implementing the gtty system call.
 * Just call lower level routine and pass back values.
 */
int SYSFILE_FUNC(gtty)(int fdesc, struct tc * parm)
{
	return ttyioctl(fdesc, TIOCGETP, parm);
}

/*
 * The routine implementing the stty system call.
 * Read in values and call lower level.
 */
int SYSFILE_FUNC(stty)(int fdesc, struct tc * parm)
{
	return ttyioctl(fdesc, TIOCSETP, parm);
}
