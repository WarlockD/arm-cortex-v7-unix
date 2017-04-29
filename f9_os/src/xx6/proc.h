#ifndef PROC_INCLUDE_
#define PROC_INCLUDE_

#include "arm.h"


// Per-CPU state, now we only support one CPU
struct cpu {
    uint8_t           id;             // index into cpus[] below
    struct context*   scheduler;    // swtch() here to enter scheduler
    volatile uint32_t   started;        // Has the CPU started?

    int             ncli;           // Depth of pushcli nesting.
    int             intena;         // Were interrupts enabled before pushcli?

    // Cpu-local storage variables; see below
    struct cpu*     cpu;
    struct proc*    proc;           // The currently-running process.
    char*			user_stack;		// start of the user stack
    char*			user_heap;		// start of the heap

};

extern struct cpu cpus[NCPU];
extern int ncpu;


extern struct cpu* cpu;
extern struct proc* proc;

//PAGEBREAK: 17
// Saved registers for kernel context switches. The context switcher
// needs to save the callee save register, as usually. For ARM, it is
// also necessary to save the banked sp (r13) and lr (r14) registers.
// There is, however, no need to save the user space pc (r15) because
// pc has been saved on the stack somewhere. We only include it here
// for debugging purpose. It will not be restored for the next process.
// According to ARM calling convension, r0-r3 is caller saved. We do
// not need to save sp_svc, as it will be saved in the pcb, neither
// pc_svc, as it will be always be the same value.
//
// Keep it in sync with swtch.S
//
// context?  how is that diffrent from trap?
#if 0
struct context {
    // svc mode registers
    uint32_t    r4;
    uint32_t    r5;
    uint32_t    r6;
    uint32_t    r7;
    uint32_t    r8;
    uint32_t    r9;
    uint32_t    r10;
    uint32_t    r11;
    uint32_t    r12;
    uint32_t    lr;
};
#endif

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, SWAPPED };

// Per-process state
struct proc {
	int 			errno_save;			// save of the error no
    size_t          sz;             // Size of process memory (bytes)
   // pde_t*          pgdir;          // Page table
    char*           kstack;         // Bottom of kernel stack for this process
    enum procstate  state;          // Process state
    volatile int    pid;            // Process ID
    struct proc*    parent;         // Parent process
    struct trapframe*   tf;         // Trap frame for current syscall
    struct context* context;        // swtch() here to run process
    void*           chan;           // If non-zero, sleeping on chan
    int             killed;         // If non-zero, have been killed
    struct file*    ofile[NOFILE];  // Open files
    struct inode*   cwd;            // Current directory
    char*			heap_end;		// end of the heap
    char*			stack_end;		// end of the current stack
    char            name[16];       // Process name (debugging)
    ino_t			swap;			// node that we swap this process on
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
#endif
