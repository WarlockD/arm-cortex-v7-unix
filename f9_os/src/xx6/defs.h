//
// a definition of data structure/methods
//
#ifndef INCLUDE_DEFS_H
#define INCLUDE_DEFS_H

#define UMAX(a, b) ((uint32_t)(a) > (uint32_t)(b) ? (uint32_t)(a):(uint32_t)(b))
#define UMIN(a, b) ((uint32_t)(a) > (uint32_t)(b) ? (uint32_t)(b):(uint32_t)(a))

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct stat;
struct superblock;
struct trapframe;

typedef uint32_t	pte_t;
typedef uint32_t  pde_t;
extern  uint32_t  _kernel_pgtbl;
typedef void (*ISR) (struct trapframe *tf, int n);

// arm.c
void            set_stk(uint32_t mode, uint32_t addr);
void            cli (void);
void            sti (void);
uint32_t            spsr_usr();
int             int_enabled();
void            pushcli(void);
void            popcli(void);
void            getcallerpcs(void *, uint32_t*);
void*           get_fp (void);
void            show_callstk (char *);



// buddy.c
void            kmem_init (void);
void            kmem_init2(void *vstart, void *vend);
void*           kmalloc (int order);
void            kfree (void *mem, int order);
void            free_page(void *v);
void*           alloc_page (void);
void            kmem_test_b (void);
int             get_order (uint32_t v);

// console.c
void            consoleinit(void);
void            cprintf(char*, ...);
void            consoleintr(int(*)(void));
void            panic(char*) __attribute__((noreturn));

// exec.c
int             exec(char*, char**);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, char*, int n);
int             filestat(struct file*, struct stat*);
int             filewrite(struct file*, char*, int n);

// fs.c
void            readsb(int dev, struct superblock *sb);
int             dirlink(struct inode*, const char*, uint32_t);
struct inode* 	iget (uint32_t dev, uint32_t inum);
struct inode*   dirlookup(struct inode*, const char*, uint32_t*);
struct inode*   ialloc(uint32_t, short);
struct inode*   idup(struct inode*);
void            iinit(void);
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(const char*);
struct inode*   nameiparent(const char*, char*);
int             readi(struct inode*, char*, uint32_t, uint32_t);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, char*, uint32_t, uint32_t);

// ide.c
void 			ideinit();
void            iderw(struct buf*);

// kalloc.c
/*char*           kalloc(void);
void            kfree(char*);
void            kinit1(void*, void*);
void            kinit2(void*, void*);
void            kmem_init (void);*/

// log.c
void            initlog(void);
void            log_write(struct buf*);
void            begin_trans();
void            commit_trans();

// picirq.c
void            pic_enable(int, ISR);
void            pic_init(void*);
void            pic_dispatch (struct trapframe *tp);

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, char*, int);
int             pipewrite(struct pipe*, char*, int);

//PAGEBREAK: 16
// proc.c
struct proc*    copyproc(struct proc*);
void            xv6_exit(void);
int             xv6_fork(void);
int             growproc(int);
int             xv6_kill(int);
void            pinit(void);
void            procdump(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            xv6_sleep(void*, struct spinlock*);
void            xv6_userinit(void);
int             xv6_wait(void);
void            xv6_wakeup(void*);
void            xv6_yield(void);

// swtch.S
void            swtch(struct context**, struct context*);



// string.c
#include <string.h>
#if 0
int             memcmp(const void*, const void*, uint32_t);
void*           memmove(void*, const void*, uint32_t);
void*           memset(void*, int, uint32_t);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint32_t);
char*           strncpy(char*, const char*, int);
#endif

char*           safestrcpy(char*, const char*, int);


// syscall.c
int             argint(int, int*);
int             argptr(int, char**, int);
int             argstr(int, char**);
int             fetchint(uint32_t, int*);
int             fetchstr(uint32_t, char**);
void            syscall(void);

// timer.c
void            timer_init(int hz);
extern struct   spinlock tickslock;

// trap.c
extern time_t     ticks;
void            trap_init(void);
void            dump_trapframe (struct context *tf);

// trap_asm.S
void            trap_reset(void);
void            trap_und(void);
void            trap_swi(void);
void            trap_iabort(void);
void            trap_dabort(void);
void            trap_na(void);
void            trap_irq(void);
void            trap_fiq(void);

// uart.c
void            uart_init(void*);
void            uartputc(int);
int             uartgetc(void);
void            micro_delay(int us);
void            uart_enable_rx();

// vm.c
int             allocuvm(pde_t*, uint32_t, uint32_t);
int             deallocuvm(pde_t*, uint32_t, uint32_t);
void            freevm(pde_t*);
void            inituvm(pde_t*, char*, uint32_t);
int             loaduvm(pde_t*, char*, struct inode*, uint32_t, uint32_t);
pde_t*          copyuvm(pde_t*, uint32_t);
void            switchuvm(struct proc*);
int             copyout(pde_t*, uint32_t, void*, uint32_t);
void            clearpteu(pde_t *pgdir, char *uva);
void*           kpt_alloc(void);
void            init_vmm (void);
void            kpt_freerange (uint32_t low, uint32_t hi);
void            paging_init (uint32_t phy_low, uint32_t phy_hi);
#endif
