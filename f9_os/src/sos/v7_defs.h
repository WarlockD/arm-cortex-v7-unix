#ifndef _V7_DEFS_H_
#define _V7_DEFS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\time.h>
#include <sys\types.h>




time_t v7_gtime();
void  v7_stime(time_t time);



void* v7_sbrk (intptr_t n);
extern time_t  v7_time(time_t* t); // should be atomic, in arch



#endif
