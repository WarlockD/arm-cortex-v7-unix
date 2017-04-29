#include "v7_internal.h"
#include "v7_user.h"
#include "asm.h"
#include <assert.h>


struct user u;
#define PROCHASHSIZE (NPROC>>2)
#define PROCHASH(X)	 ((X) % PROCHASHSIZE)
#define MAX_PID 9999
struct proc proc[NPROC];
static struct proc_tq proc_hash[PROCHASHSIZE];
static struct proc_tq proc_free;
static pid_t pid_num = 0;
struct map coremap[CMAPSIZ]; /* space for core allocation */
struct map swapmap[SMAPSIZ]; /* space for swap allocation */
// schedualer
int runin = 0;

static struct proc* lookupproc(pid_t pid) {
	// assuming we are already locked
	size_t hash = PROCHASH(pid);
	struct proc* p;
	TAILQ_FOREACH(p, &proc_hash[hash],p_hash){
		if(p->p_pid == pid) return p;
	}
	return NULL;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and return it.
// Otherwise return 0.
static struct proc* allocproc(void)
{
	int s;
	struct proc *p;

	s = spl6(); // hard stop
	if(TAILQ_EMPTY(&proc_free)){
		while(TAILQ_EMPTY(&proc_free)) {
			spl0();
			ksleep(&proc_free, PZERO);
		}
	}
	spl6();
    p = TAILQ_FIRST(&proc_free);
    TAILQ_REMOVE(&proc_free, p , p_link);
    do {
    	 if(++pid_num > MAX_PID) pid_num=1;
    } while(lookupproc(pid_num));
    p->p_pid = pid_num;
    // init the rest of the process here
    TAILQ_INSERT_HEAD(&proc_hash[PROCHASH(p->p_pid)], p, p_hash);
    splx(s);
    return p;
}
static void freeproc(struct proc* p) {
	int s = spl6(); // hard stop
	p->p_pid = 0;
	TAILQ_REMOVE(&proc_hash[PROCHASH(p->p_pid)], p, p_hash);
	TAILQ_INSERT_TAIL(&proc_free, p, p_hash);
	splx(s);
}
extern void v7_arch_setup();
void fsinit();
void init_proc() {
	memset(&u, 0, sizeof(u));
	memset(&proc, 0, sizeof(proc));
	pid_num = 0;
	// attach the first proc to it
	TAILQ_INIT(&u.procs);
	TAILQ_INIT(&proc_free);
	//struct proc* p;
	foreach_array(p,proc) {
		p->state = UNUSED;
		TAILQ_INSERT_HEAD(&proc_free, p, p_link);
	}
	foreach_array(pq,proc_hash) TAILQ_INIT(pq);
	struct proc* temp = allocproc();
	TAILQ_INSERT_HEAD(&u.procs, temp, p_link);
	// set up memory
	extern char end __asm("end");
	u.heap_start = u.heap_end = &end;
	 fsinit();
	v7_arch_setup();
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated
 * space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 * The core map unit is 64 bytes; the swap map unit
 * is 512 bytes.
 * Algorithm is first-fit.
 */
void* kmalloc (struct map *mp, size_t size)
{
	caddr_t a;
	struct map *bp;

	for (bp=mp; bp->m_size; bp++) {
		if (bp->m_size >= size) {
			a = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while (((bp-1)->m_size = bp->m_size)!=0);
			}
			return(a);
		}
	}
	return(0);
}

/*
 * Free the previously allocated space aa
 * of size units into the specified map.
 * Sort aa into map and combine on
 * one or both ends if possible.
 */
void kmfree (struct map *mp, size_t size, void* a)
{
	struct map *bp;
	unsigned int t;

	if ((bp = mp)==coremap && runin) {
		runin = 0;
		kwakeup(&runin);	/* Wake scheduler when freeing core */
	}
	for (; bp->m_addr<=a && bp->m_size!=0; bp++);
	if (bp>mp && (bp-1)->m_addr+(bp-1)->m_size == a) {
		(bp-1)->m_size += size;
		if (a+size == bp->m_addr) {
			(bp-1)->m_size += bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
		}
	} else {
		if (a+size == bp->m_addr && bp->m_size) {
			bp->m_addr -= size;
			bp->m_size += size;
		} else if (size) {
			do {
				t = bp->m_addr;
				bp->m_addr = a;
				a = t;
				t = bp->m_size;
				bp->m_size = size;
				bp++;
			} while (size = t);
		}
	}
}



/*
 * break system call.
 *  -- bad planning: "break" is a dirty word in C.
 */
void* v7_sbrk (intptr_t n)
{
	caddr_t current = u.heap_end;
	caddr_t heap_end = u.heap_start + n;
	caddr_t sp =  (caddr_t)get_psp();
	if(heap_end >= sp) {
		u.errno = ENOMEM;
		return -1;
	}
	u.heap_end = heap_end;
	return current;
}

void ksleep(void* chan, int prio){
	assert(0);
}
void kwakeup(void* chan){
	//assert(0);
}
