#ifndef _SYSFILE_H_
#define _SYSFILE_H_

#if 0
#define SYSFILE_FUNC(NAME) xv6_##NAME
#else
#define SYSFILE_FUNC(NAME) _##NAME
#endif

#define SYSFILE_FUNC(NAME) _##NAME

int SYSFILE_FUNC(read) (int fd, char *p, int n);
int SYSFILE_FUNC(write)(int fd, char *p, int n);
int SYSFILE_FUNC(lseek)(int fd, int offset, int rdir);
int SYSFILE_FUNC(close)(int fd);
int SYSFILE_FUNC(open)(char *path, int omode, ...);
int SYSFILE_FUNC(stat)(const char *file, struct stat *st);
int SYSFILE_FUNC(fstat)(int fd, struct stat *st);
int SYSFILE_FUNC(dup)(int fd);
int SYSFILE_FUNC(link)(const char* oldp, const char* newp);
int SYSFILE_FUNC(unlink)(const char* path);
int SYSFILE_FUNC(mkdir)(const char* path);
int SYSFILE_FUNC(mknod)(const char* path, int mode, dev_t dev);
int SYSFILE_FUNC(chdir)(const char* path);
int SYSFILE_FUNC(exec)(const char* path, const char* args);
int SYSFILE_FUNC(pipe)(int fd[2]);



#endif
