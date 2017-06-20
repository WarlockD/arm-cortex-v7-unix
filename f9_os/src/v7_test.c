#include <assert.h>
#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>
#include <pin_macros.h>
#include <string.h>
#include <stdio.h>

#include <diag\Trace.h>
#if 0



#include <sys\time.h>
#include <errno.h>


#define __KERNEL__
#include "xx6\types.h"
#include "xx6\param.h"
#include "xx6\file.h"
#include "xx6\fs.h"
#include "xx6\proc.h"

#endif
#if 0
#include <diag\Trace.h>
#include "sos\v7_user.h"
#include "sos\v7_bio.h"
#include "sos\v7_inode.h"
#include "sos\dev\dev.h"
#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#else
#if __BSD_VISIBLE == 0
#undef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif
#endif
#include <sys\stat.h>

#endif
#include <errno.h>
#undef errno
extern int errno;

void _putchar(char c);



/* Functions */
void initialise_monitor_handles()
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{
	_kill(status, -1);
	while (1) {}		/* Make sure we hang here */
}
int _isatty(int fp){
	if(fp <3) return 1;
	return 0;
}
typedef char* caddr_t;
int _write(int fp, char* data, int size){ return -1; }
int _read(int fp, char* data, int size){ return -1; }
void _close(int fp) {}
int _fstat(int fp,void* meh) { return -1;}
int _lseek(int fp,int offset, int dir) { return -1; }

#if 0
void callit(const char* args, int(*func)(int argc, const char*[])){
	const char* temp[12] = { 0 };
	char buffer[128];
	//memcpy(buffer,args,strlen(args));
	char* cp = buffer;
	char* s= buffer;
	int argc= 0;
	while(*args) {
		int c = *args++;
		if(c == ' ') {
			*cp++ = 0;

			temp[argc++] = s;
			s = cp;

		}
		else *cp++ = c;
	}
	*cp = 0;
	 temp[argc++] = s;
	 func(argc,temp);

}
int _gettimeofday(struct timeval *tv, struct timezone *tz) {
	if(tv){
		tv->tv_sec =0;
		tv->tv_usec =0;
		return 0;
	}
	return -1;
}
void kmain ();
int make_root_fs();
void ideinit();
void binit();
#endif

void v7_test() {
	//kmain ();
//	_sbrk(0); // hard set psp for right now
	//ideinit();

	// ls("/dev");
	// install_blockdev(0,&nor_bdevsw);
	  // ok lets try

	//  pentry("/", CPROC->dir_root);
	// void ls(const char *path)
	//  callit("ls -l -r ", ls);
	  while(1);
}
