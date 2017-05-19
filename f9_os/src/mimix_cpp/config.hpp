#ifndef _CONFIG_H
#define _CONFIG_H


#include <cstdint>


namespace mimx {
	/* Length of program names stored in the process table. This is only used
	 * for the debugging dumps that can be generated with the IS server. The PM
	 * server keeps its own copy of the program name.
	 */
	constexpr static std::size_t P_NAME_LEN	   =8;

	/* Kernel diagnostics are written to a circular buffer. After each message,
	 * a system server is notified and a copy of the buffer can be retrieved to
	 * display the message. The buffers size can safely be reduced.
	 */
	constexpr static std::size_t KMESS_BUF_SIZE   =256;

	/* Buffer to gather randomness. This is used to generate a random stream by
	 * the MEMORY driver when reading from /dev/random.
	 */
	constexpr static std::size_t RANDOM_ELEMENTS   =32;

	/* This section contains defines for valuable system resources that are used
	 * by device drivers. The number of elements of the vectors is determined by
	 * the maximum needed by any given driver. The number of interrupt hooks may
	 * be incremented on systems with many device drivers.
	 */
	constexpr static std::size_t NR_IRQ_HOOKS	  =16;		/* number of interrupt hooks */
	constexpr static std::size_t VDEVIO_BUF_SIZE   =64;		/* max elements per VDEVIO request */
	constexpr static std::size_t VCOPY_VEC_SIZE    =16;		/* max elements per VCOPY request */

	/* How many bytes for the kernel stack. Space allocated in mpx.s. */
	constexpr static std::size_t K_STACK_BYTES   =1024;

	/* This section allows to enable kernel debugging and timing functionality.
	 * For normal operation all options should be disabled.
	 */
	constexpr static std::size_t DEBUG_SCHED_CHECK  =0;	/* sanity check of scheduling queues */
	constexpr static std::size_t DEBUG_LOCK_CHECK   =0;	/* kernel lock() sanity check */
	constexpr static std::size_t DEBUG_TIME_LOCKS   =0;	/* measure time spent in locks */

	constexpr static std::size_t NR_PROCS 	 = 64;
	constexpr static std::size_t NR_SYS_PROCS    =  32;
	constexpr static size_t NR_TASKS  = 4;

	constexpr static std::size_t NR_BUFS	=128;
	constexpr static std::size_t NR_BUF_HASH =128;

/* Number of controller tasks (/dev/cN device classes). */
	constexpr static std::size_t NR_CTRLRS        =  2;

/* Enable or disable the second level file system cache on the RAM disk. */
	constexpr static std::size_t ENABLE_CACHE2     = false;

/* Enable or disable swapping processes to disk. */
	constexpr static std::size_t ENABLE_SWAP	   = false;

/* Include or exclude an image of /dev/boot in the boot image.
 * Please update the makefile in /usr/src/tools/ as well.
 */
	constexpr static bool ENABLE_BOOTDEV	=   false;	/* load image of /dev/boot at boot time */

/* DMA_SECTORS may be increased to speed up DMA based drivers. */
	constexpr static std::size_t DMA_SECTORS     =   1;	/* DMA buffer size (must be >= 1) */

/* Include or exclude backwards compatibility code. */
	constexpr static bool ENABLE_BINCOMPAT  = false; 	/* for binaries using obsolete calls */
	constexpr static bool ENABLE_SRCCOMPAT  = false;	/* for sources using obsolete calls */

/* Which process should receive diagnostics from the kernel and system?
 * Directly sending it to TTY only displays the output. Sending it to the
 * log driver will cause the diagnostics to be buffered and displayed.
 */
	//constexpr static std::size_t OUTPUT_PROC_NR	= LOG_PROC_NR;	/* TTY_PROC_NR or LOG_PROC_NR */

/* NR_CONS, NR_RS_LINES, and NR_PTYS determine the number of terminals the
 * system can handle.
 */
	constexpr static std::size_t  NR_CONS       =   4;	/* # system consoles (1 to 8) */
	constexpr static std::size_t NR_RS_LINES	=   0;	/* # rs232 terminals (0 to 4) */
	constexpr static std::size_t NR_PTYS		=   0;	/* # pseudo terminals (0 to 64) */

	// clicks, not sure if needed

	constexpr static std::size_t CLICK_SIZE      =1024;	/* unit in which memory is allocated */
	constexpr static std::size_t CLICK_SHIFT     =10;	/* log2 of CLICK_SIZE */
};
















#endif
