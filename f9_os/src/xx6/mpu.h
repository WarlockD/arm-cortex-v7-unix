#ifndef _MPU_H_
#define _MPU_H_


// Key addresses for address space layout (see kmap in vm.c for layout)
#define SRAMSTART 	  	0x20000000			/* start of free */
//#define EXTMEM
#define KERNBASE  		0x80000000         // First kernel virtual address
//#define KERNLINK  		(KERNBASE+EXTMEM)  // Address where kernel is linked

// we first map 1MB low memory containing kernel code.
#define INIT_KERNMAP 	0x100000


// align_up/down: al must be of power of 2
#define align_up(sz, al) (((uint32_t)(sz)+ (uint32_t)(al)-1) & ~((uint32_t)(al)-1))
#define align_dn(sz, al) ((uint32_t)(sz) & ~((uint32_t)(al)-1))

#define PE_TYPES    0x03    // mask for page type
#define KPDE_TYPE   0x02    // use "section" type for kernel page directory
#define UPDE_TYPE   0x01    // use "coarse page table" for user page directory
#define PTE_TYPE    0x02    // executable user page(subpage disable)

// 1st-level or large (1MB) page directory (always maps 1MB memory)
#define PDE_SHIFT   20                      // shift how many bits to get PDE index
#define PDE_SZ      (1 << PDE_SHIFT)
#define PDE_MASK    (PDE_SZ - 1)            // offset for page directory entries
#define PDE_IDX(v)  ((uint)(v) >> PDE_SHIFT) // index for page table entry

// 2nd-level page table
#define PTE_SHIFT   12                  // shift how many bits to get PTE index
#define PTE_IDX(v)  (((uint)(v) >> PTE_SHIFT) & (NUM_PTE - 1))
#define PTE_SZ      (1 << PTE_SHIFT)
#define PTE_ADDR(v) align_dn (v, PTE_SZ)
#define PTE_AP(pte) (((pte) >> 4) & 0x03)

// size of two-level page tables
#define UADDR_BITS  28                  // maximum user-application memory, 256MB
#define UADDR_SZ    (1 << UADDR_BITS)   // maximum user address space size

#endif
