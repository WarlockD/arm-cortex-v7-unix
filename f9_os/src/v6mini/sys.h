#ifndef _MXSYS_SYS_H_
#define _MXSYS_SYS_H_

// all the sys calls
#include "types.h"

// sys1
void v6_exec(); // DOSN'T WORK
void v6_rexit();
void v6_exit();
void r6_wait();
int v6_fork();
int v6_sbreak(int size);
// sys2
int  v6_read(int fdesc, void* data, size_t size);
int  v6_write(int fdesc, void* data, size_t size);
int  v6_open(const char* filename, mode_t mode);
int  creat(const char* filename, mode_t mode);
void v6_close(int fdesc);
off_t v6_lseek(int fdesc,off_t offset, int rdir);
int v6_link(const char* fname, const char* flink);
int v6_mknod(const char* fname, int mode, dev_t dev);
void v6_sleep(time_t ts);
//sys3
int v6_fstat(int fdesc, struct stat* st);
int v6_stat(const char* fname, struct stat* st);
int v6_dup(int fdesc);
int v6_mount(const char* root_dir, const char* device_name, int ro);
int v6_umount(const char* root_dir);
#endif
