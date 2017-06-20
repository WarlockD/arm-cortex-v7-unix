// BSP support routine
#define __KERNEL__
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "mpu.h"
#include "buf.h"
#include "fs.h"
#include "file.h"
#include <assert.h>
#include <string.h>
#include <stm32746g_discovery.h>
#include "sysfile.h"

void trace_printf(const char*,...);

struct cpu	cpus[NCPU];
struct cpu	*cpu;

#define MB (1024*1024)

uint8_t temp_kernel[1024*64];

static void  mkfs_balloc(struct superblock* sb, uint32_t used)
{
  trace_printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE);
  uint32_t bn = sb->ninodes / IPB + 3;
  struct buf * bp = bread(0,bn);
  uint8_t* buf = (uint8_t*)bp->data;
  for(int i = 0; i < used; i++) {
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  trace_printf("balloc: write bitmap block at sector %lu\n", sb->ninodes/IPB + 3);
	bwrite(bp);
	brelse(bp);
}
static uint32_t i2b(uint32_t inum){ return (inum / IPB) + 2; }
struct inode* iget (uint32_t dev, uint32_t inum);
void ideinit();
void write_root_sb(int dev) {
	//ideinit();
	struct buf *bp;
	struct superblock sb;
	struct inode *ip;
	sb.size = 1024;
	sb.nblocks = 995; // so whole disk is size sectors
	sb.ninodes = 200;
	sb.nlog = LOGSIZE;
	uint32_t bitblocks = sb.size/(BSIZE*8) + 1;
	uint32_t  usedblocks = sb.ninodes / IPB + 3 + bitblocks;
	uint32_t freeblock = usedblocks;
	// format and make file system
	trace_printf("used %d (bit %d ninode %lu) free %u total %d\n", usedblocks,
	         bitblocks, sb.ninodes/IPB + 1, freeblock, sb.nblocks+usedblocks);
	bp = bread(dev, 1);
	memmove(bp->data, &sb, sizeof(sb));
	bwrite(bp);
	brelse(bp);
	mkfs_balloc(&sb,usedblocks);

	initlog(); // init the log here so the standard os stuff works
	begin_trans();
	trace_printf("Staring root dir creationg\r\n");
	ip= ialloc (ROOTDEV, _IFDIR);
	assert(ip->inum == ROOTINO);

	ilock(ip);
	ip->dev = ROOTDEV;
	ip->nlink = 2;
	iupdate(ip);
	trace_printf("root inode updated creationg\r\n");
	// No ip->nlink++ for ".": avoid cyclic ref count.
	assert(dirlink(ip, ".", ip->inum)==0); // panic("create dots");
	trace_printf("dirlink .\r\n");
	//iupdate(ip); // force an update?
	assert(dirlink(ip, "..", ip->inum)==0); // panic("create dots");
	trace_printf("dirlink ..\r\n");
    iunlockput(ip);
    trace_printf("iunlockput ..\r\n");
    commit_trans();

    struct proc _p;
    proc = &_p;
    proc->cwd = namei("/");
    SYSFILE_FUNC(mkdir)("/dev");
    SYSFILE_FUNC(mknod)("/dev/console", 0,0);
    SYSFILE_FUNC(mknod)("/dev/mem0", 0,0);
    iput(proc->cwd);
    proc =  NULL;
}

void kmain (void)
{
   // uint32_t vectbl;

    cpu = &cpus[0];

   // uart_init (P2V(UART0));

    // interrrupt vector table is in the middle of first 1MB. We use the left
    // over for page tables
  //  vectbl = P2V_WO (VEC_TBL & PDE_MASK);

   // init_vmm ();
   // kpt_freerange (align_up(&end, PT_SZ), vectbl);
   // kpt_freerange (vectbl + PT_SZ, P2V_WO(INIT_KERNMAP));
  //  paging_init (INIT_KERNMAP, PHYSTOP);

    kmem_init ();
   kmem_init2(temp_kernel, temp_kernel + sizeof(temp_kernel));
	//extern char end __asm("end");
	//char *prev_heap_end;
	//assert(cpu); temp_kernel
	if (cpu->user_heap == 0) {
		// userstack starts 4096 after the kernel stack
		cpu->user_heap = temp_kernel;
		cpu->user_stack = (char*)ALIGN(__get_MSP(),4096)-4096;
#if 0
		__set_PSP(ALIGN(__get_MSP(),4096)-4096);
		cpu->user_stack = (char*)__get_PSP();
		cpu->user_heap = ALIGNP(&end,sizeof(uint32_t));
#endif
	}
   // trap_init ();				// vector table and stacks for models
   // pic_init (P2V(VIC_BASE));	// interrupt controller
   // uart_enable_rx ();			// interrupt for uart
    consoleinit ();				// console
    pinit ();					// process (locks)

    binit ();					// buffer cache
    fileinit ();				// file table
    iinit ();					// inode cache

    timer_init (HZ);			// the timer (ticker)
    // create the root dirrectory
    write_root_sb(0); // creates an empty root directory


    sti ();
    trace_printf("Ok, starting user init!\r\n");
    // hard set it so we can test the file system ugh
    proc=userinit();					// first user process

    scheduler();				// start running processes
}
