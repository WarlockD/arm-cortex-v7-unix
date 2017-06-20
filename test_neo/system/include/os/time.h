#ifndef _OS_TIMER_H_
#define _OS_TIMER_H_

#define _POSIX_MONOTONIC_CLOCK
#define _POSIX_TIMERS
#include <stdint.h>
#include <time.h>
#include <sys\time.h>
#include <sys\times.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

	uint64_t sys_usec();
	void microtime(struct timeval* tv);
	int clock_settime(clockid_t clock_id, const struct timespec *tp);
	// helper functions
	__attribute__((always_inline)) static inline int timespec_snprintf(const struct timespec * tv,char* buffer, size_t len){
		return tv  && buffer && len > 15 ? snprintf(buffer, len, "%3d.%09ld", tv->tv_sec, tv->tv_nsec) : 0;
	}
	__attribute__((always_inline)) static inline int timeval_snprintf(const struct timeval * tv,char* buffer, size_t len){
		return tv  && buffer && len > 11 ? snprintf(buffer, len, "%3d.%06ld ", tv->tv_sec, tv->tv_usec) : 0;
	}
	// simple stopwatch
	typedef struct stopwatch_s
	{
	    struct timespec startTime;
	    struct timespec runningTime;
	    bool isRunning;
	}  stopwatch_t;
	// http://michael.dipperstein.com/stopwatch/

	__attribute__((always_inline)) static inline int PrintTimer(stopwatch_t *stopWatch,char* buffer, size_t len){
		// we device it by usec
		return timespec_snprintf(&stopWatch->runningTime,buffer,len);
	}

	__attribute__((always_inline)) static inline void StartTimer(stopwatch_t *stopWatch){
		clock_gettime(CLOCK_MONOTONIC, &stopWatch->startTime);
	    stopWatch->isRunning = true;
	    stopWatch->runningTime.tv_sec = 0;
	    stopWatch->runningTime.tv_nsec = 0;
	}


	__attribute__((always_inline)) static inline void StopTimer(stopwatch_t *stopWatch){
	    if (stopWatch->isRunning == false) return;
	    stopWatch->isRunning = false;
	    struct timespec now;
	    clock_gettime(CLOCK_MONOTONIC, &now);

	    /* update running time */
	    stopWatch->runningTime.tv_sec += (now.tv_sec - stopWatch->startTime.tv_sec);
	    if (now.tv_nsec < stopWatch->startTime.tv_nsec) 	        /* borrow second */
	    {
	        now.tv_nsec += 1000000000U;
	        stopWatch->runningTime.tv_sec -= 1;
	    }
	    stopWatch->runningTime.tv_nsec += (now.tv_nsec - stopWatch->startTime.tv_nsec);
	    stopWatch->startTime.tv_sec = 0;
	    stopWatch->startTime.tv_nsec = 0;
	}
	__attribute__((always_inline)) static inline void ResumeTimer(stopwatch_t *stopWatch)
	{
	    if (stopWatch->isRunning == false)
	    {
	        stopWatch->isRunning = true;
	        clock_gettime(CLOCK_MONOTONIC, &stopWatch->startTime);
	    }
	}

	__attribute__((always_inline)) static inline uint32_t ReadTimerMSEC(const stopwatch_t *stopWatch)
	{
	    if (stopWatch->isRunning==false)
	        return (stopWatch->runningTime.tv_sec * 1000) +  (stopWatch->runningTime.tv_nsec / 1000000);

		uint32_t delta;
	    struct timespec now;
	    clock_gettime(CLOCK_MONOTONIC, &now);

	    /* update running time */
	    if (now.tv_nsec < stopWatch->startTime.tv_nsec){
	        /* borrow second */
	        now.tv_nsec += 1000000000;
	        now.tv_sec -= 1;
	    }

	    delta = ((now.tv_sec - stopWatch->startTime.tv_sec) +
	        stopWatch->runningTime.tv_sec) * 1000;

	    delta += ((now.tv_nsec - stopWatch->startTime.tv_nsec) +
	        stopWatch->runningTime.tv_nsec) / 1000000;

	    return delta;
	}

#ifdef __cplusplus
}
#endif



#endif
