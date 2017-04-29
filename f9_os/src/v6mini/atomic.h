#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

#define _SUPER_INLINE __attribute__( ( always_inline ) ) __STATIC_INLINE

#ifndef __CMSIS_GCC_H

// simple atomic file I keep around for cortex stuff
_SUPER_INLINE void __enable_irq(void)
{
  __asm volatile ("cpsie i" : : : "memory");
}
_SUPER_INLINE void __disable_irq(void)
{
	__asm volatile ("cpsid i" : : : "memory");
}
_SUPER_INLINE  bool __irq_disabled(void)
{
	  uint32_t result;
	  __asm volatile ("MRS %0, primask" : "=r" (result) );
	  return result == 1 ? true : false;
}
// instruction/data sync
_SUPER_INLINE void __ISB(void) {__asm volatile ("isb 0xF":::"memory");}
_SUPER_INLINE void __DSB(void) {__asm volatile ("dsb 0xF":::"memory");}
// memory barrier
_SUPER_INLINE void __DMB(void) {__asm volatile ("dmb 0xF":::"memory");}
_SUPER_INLINE void __WFI(void) {__asm volatile ("wfi":::"memory");}
#define __BKPT(value)                       __ASM volatile ("bkpt "#value)
#define __CLZ             __builtin_clz
// exclusive locks
_SUPER_INLINE uint8_t __LDREXB(volatile uint8_t *addr)
{
    uint32_t result;
   __asm volatile ("ldrexb %0, %1" : "=r" (result) : "Q" (*addr) );
   return ((uint8_t) result);    /* Add explicit type cast here */
}
_SUPER_INLINE uint16_t __LDREXH(volatile uint16_t *addr)
{
    uint32_t result;
   __asm volatile ("ldrexh %0, %1" : "=r" (result) : "Q" (*addr) );
   return ((uint16_t) result);    /* Add explicit type cast here */
}
_SUPER_INLINE uint32_t __LDREXW(volatile uint32_t *addr)
{
    uint32_t result;
   __asm volatile ("ldrex %0, %1" : "=r" (result) : "Q" (*addr) );
   return (result);
}
_SUPER_INLINE uint32_t __STREXB(uint8_t value, volatile uint8_t *addr)
{
   uint32_t result;
   __asm  volatile ("strexb %0, %2, %1" : "=&r" (result), "=Q" (*addr) : "r" ((uint32_t)value) );
   return(result);
}
_SUPER_INLINE uint32_t __STREXH(uint16_t value, volatile uint16_t *addr)
{
   uint32_t result;
   __asm volatile ("strexh %0, %2, %1" : "=&r" (result), "=Q" (*addr) : "r" ((uint32_t)value) );
   return(result);
}
_SUPER_INLINE uint32_t __STREXW(uint32_t value, volatile uint32_t *addr)
{
   uint32_t result;
   __asm volatile ("strex %0, %2, %1" : "=&r" (result), "=Q" (*addr) : "r" (value) );
   return(result);
}
_SUPER_INLINE void __CLREX(void) { __asm volatile ("clrex" ::: "memory"); }
// loading unprivilaged sutff
_SUPER_INLINE uint8_t __LDRBT(volatile uint8_t *addr)
{
    uint32_t result;
   __asm volatile ("ldrbt %0, %1" : "=r" (result) : "Q" (*addr) );
   return ((uint8_t) result);    /* Add explicit type cast here */
}
_SUPER_INLINE uint16_t __LDRHT(volatile uint16_t *addr)
{
	   uint32_t result;
    __asm volatile ("ldrht %0, %1" : "=r" (result) : "Q" (*addr) );
   return ((uint16_t) result);    /* Add explicit type cast here */
}
_SUPER_INLINE  uint32_t __LDRT(volatile uint32_t *addr)
{
    uint32_t result;
    __asm volatile ("ldrt %0, %1" : "=r" (result) : "Q" (*addr) );
   return(result);
}
_SUPER_INLINE void __STRBT(uint8_t value, volatile uint8_t *addr)
{
	__asm volatile ("strbt %1, %0" : "=Q" (*addr) : "r" ((uint32_t)value) );
}
_SUPER_INLINE void __STRHT(uint16_t value, volatile uint16_t *addr)
{
	__asm volatile ("strht %1, %0" : "=Q" (*addr) : "r" ((uint32_t)value) );
}
_SUPER_INLINE void __STRT(uint32_t value, volatile uint32_t *addr)
{
	__asm volatile ("strt %1, %0" : "=Q" (*addr) : "r" (value) );
}

#endif //__CMSIS_GCC_H

// get user, get memory from user routines
#define get_user(x,p)							\
	({								\		\
		register __typeof__(*(p)) __e;		\
		switch (sizeof(*(__p))) {				\
		case 1:	__e = __LDRBT(p); break;					\
		case 2:	__e = __LDRHT(p); break;					\
		case 4:	__e = __LDRT(p); break;					\
		default: __e = __get_user_bad(); break;			\
		}							\
		__e;							\
	})
// helpful combines
// from mbeb
__STATIC_INLINE bool atomic_cas_u8(uint8_t *ptr, uint8_t *expectedCurrentValue, uint8_t desiredValue)
{
    uint8_t currentValue = __LDREXB((volatile uint8_t*)ptr);
    if (currentValue != *expectedCurrentValue) {
        *expectedCurrentValue = currentValue;
        __CLREX();
        return false;
    }
    return !__STREXB(desiredValue, (volatile uint8_t*)ptr);
}
__STATIC_INLINE bool atomic_cas_u16(uint16_t *ptr, uint16_t *expectedCurrentValue, uint16_t desiredValue)
{
    uint16_t currentValue = __LDREXH((volatile uint16_t*)ptr);
    if (currentValue != *expectedCurrentValue) {
        *expectedCurrentValue = currentValue;
        __CLREX();
        return false;
    }
    return !__STREXH(desiredValue, (volatile uint16_t*)ptr);
}
__STATIC_INLINE bool atomic_cas_u32(uint32_t *ptr, uint32_t *expectedCurrentValue, uint32_t desiredValue)
{
    uint32_t currentValue = __LDREXW((volatile uint32_t*)ptr);
    if (currentValue != *expectedCurrentValue) {
        *expectedCurrentValue = currentValue;
        __CLREX();
        return false;
    }
    return !__STREXW(desiredValue, (volatile uint32_t*)ptr);
}
__STATIC_INLINE uint8_t atomic_incr_u8(uint8_t *valuePtr, uint8_t delta)
{
    uint8_t newValue;
    do {
        newValue = __LDREXB((volatile uint8_t*)valuePtr) + delta;
    } while (__STREXB(newValue, (volatile uint8_t*)valuePtr));
    return newValue;
}
__STATIC_INLINE uint16_t atomic_incr_u16(uint16_t *valuePtr, uint16_t delta)
{
    uint16_t newValue;
    do {
        newValue = __LDREXH((volatile uint16_t*)valuePtr) + delta;
    } while (__STREXH(newValue, (volatile uint16_t*)valuePtr));
    return newValue;
}
__STATIC_INLINE uint32_t atomic_incr_u32(uint32_t *valuePtr, uint32_t delta)
{
    uint32_t newValue;
    do {
        newValue = __LDREXW((volatile uint32_t*)valuePtr) + delta;
    } while (__STREXW(newValue, (volatile uint32_t*)valuePtr));
    return newValue;
}
__STATIC_INLINE uint8_t atomic_decr_u8(uint8_t *valuePtr, uint8_t delta)
{
    uint8_t newValue;
    do {
        newValue = __LDREXB((volatile uint8_t*)valuePtr) - delta;
    } while (__STREXB(newValue, (volatile uint8_t*)valuePtr));
    return newValue;
}

__STATIC_INLINE uint16_t atomic_decr_u16(uint16_t *valuePtr, uint16_t delta)
{
    uint16_t newValue;
    do {
        newValue = __LDREXH((volatile uint16_t*)valuePtr) - delta;
    } while (__STREXH(newValue, (volatile uint16_t*)valuePtr));
    return newValue;
}
__STATIC_INLINE uint32_t atomic_decr_u32(uint32_t *valuePtr, uint32_t delta)
{
    uint32_t newValue;
    do {
        newValue = __LDREXW((volatile uint32_t*)valuePtr) - delta;
    } while (__STREXW(newValue, (volatile uint32_t*)valuePtr));
    return newValue;
}
__STATIC_INLINE bool atomic_cas_ptr(void **ptr, void **expectedCurrentValue, void *desiredValue) {
    return atomic_cas_u32(
            (uint32_t *)ptr,
            (uint32_t *)expectedCurrentValue,
            (uint32_t)desiredValue);
}

#define atomic_cas_obj(PTR,EXPECTED,DESIRED) \
	atomic_cas_u32((uint32_t *)(PTR),  (uint32_t *)(EXPECTED), (uint32_t)(DESIRED))

__STATIC_INLINE void *atomic_incr_ptr(void **valuePtr, ptrdiff_t delta) {
    return (void *)atomic_incr_u32((uint32_t *)valuePtr, (uint32_t)delta);
}

__STATIC_INLINE void *atomic_decr_ptr(void **valuePtr, ptrdiff_t delta) {
    return (void *)atomic_decr_u32((uint32_t *)valuePtr, (uint32_t)delta);
}




#endif
