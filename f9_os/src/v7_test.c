#include <assert.h>
#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>
#include <pin_macros.h>
#include <string.h>
#include <stdio.h>

#include <os\printk.h>
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
#include <os\printk.h>
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
void printk(const char*,...);
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

caddr_t _sbrk(int incr)
{
	extern char end __asm("end");
	static char* heap_end = NULL;
	char *prev_heap_end;
	//assert(cpu);
	if (heap_end== 0) { heap_end = &end; } // } ,sizeof(uint32_t)); }
	//assert(proc);
	prev_heap_end = heap_end;
	if ((prev_heap_end + incr) > (char *)__get_MSP())
	{
//		write(1, "Heap and stack collision\n", 25);
//		abort();
		errno = ENOMEM;
		return (caddr_t) -1;
	}
	heap_end += incr;
	return (caddr_t) prev_heap_end;
}
#if 0
int ls(int , const char*[]);


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
