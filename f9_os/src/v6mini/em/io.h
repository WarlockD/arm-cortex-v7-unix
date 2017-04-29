#ifndef __EM_ARM_IO_H
#define __EM_ARM_IO_H

#include <stdint.h>
#ifdef __CHECKER__
# define __user		__attribute__((noderef, address_space(1)))
# define __kernel	/* default address space */
# define __safe		__attribute__((safe))
# define __force	__attribute__((force))
# define __nocast	__attribute__((nocast))
# define __iomem	__attribute__((noderef, address_space(2)))
#else
# define __user
# define __kernel	/* default address space */
# define __safe
# define __force
# define __nocast
# define __iomem
#define  __chk_io_ptr(x) (void)0

#endif

#define IO_SPACE_LIMIT 0xffffffff

#define __io(a) 	__typesafe_io(a)
#define __mem_pci(a)	(a)
#define __mem_isa(a)	(a)

extern void __raw_writesb(void __iomem *addr, const void *data, int bytelen);
extern void __raw_writesw(void __iomem *addr, const void *data, int wordlen);
extern void __raw_writesl(void __iomem *addr, const void *data, int longlen);

extern void __raw_readsb(const void __iomem *addr, void *data, int bytelen);
extern void __raw_readsw(const void __iomem *addr, void *data, int wordlen);
extern void __raw_readsl(const void __iomem *addr, void *data, int longlen);

#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a) = (v))
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile unsigned short __force *)(a) = (v))
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a) = (v))

#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a))
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile unsigned short __force *)(a))
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a))
#define le32_to_cpu(a)


/*
 * A typesafe __io() helper
 */
static inline void __iomem *__typesafe_io(unsigned long addr)
{
	return (void __iomem *)addr;
}
#ifdef __mem_pci
#define readb(c) ({ unsigned char  	__v = __raw_readb(c); __v; })
#define readw(c) ({ unsigned short 	__v = __raw_readw(c); __v; })
#define readl(c) ({ unsigned int 	__v = __raw_readl(c); __v; })
#define readb_relaxed(addr) readb(addr)
#define readw_relaxed(addr) readw(addr)
#define readl_relaxed(addr) readl(addr)

#define readsb(p,d,l)		__raw_readsb(__mem_pci(p),d,l)
#define readsw(p,d,l)		__raw_readsw(__mem_pci(p),d,l)
#define readsl(p,d,l)		__raw_readsl(__mem_pci(p),d,l)

#define writeb(v,c)		__raw_writeb(v,c)
#define writew(v,c)		__raw_writew(v,c)
#define writel(v,c)		__raw_writel(v,c)

#define writesb(p,d,l)		__raw_writesb(__mem_pci(p),d,l)
#define writesw(p,d,l)		__raw_writesw(__mem_pci(p),d,l)
#define writesl(p,d,l)		__raw_writesl(__mem_pci(p),d,l)

#define memset_io(c,v,l)	_memset_io(__mem_pci(c),(v),(l))
#define memcpy_fromio(a,c,l)	_memcpy_fromio((a),__mem_pci(c),(l))
#define memcpy_fromiow(a,c,l)	_memcpy_fromiow((a),__mem_pci(c),(l))
#define memcpy_fromiol(a,c,l)	_memcpy_fromiol((a),__mem_pci(c),(l))
#define memcpy_toio(c,a,l)	_memcpy_toio(__mem_pci(c),(a),(l))
#define memcpy_toiow(c,a,l)	_memcpy_toiow(__mem_pci(c),(a),(l))
#define memcpy_toiol(c,a,l)	_memcpy_toiol(__mem_pci(c),(a),(l))

#elif !defined(readb)
/*
 * Bad read/write accesses...
 */
extern void __readwrite_bug(const char *fn);
#define readb(c)			(__readwrite_bug("readb"),0)
#define readw(c)			(__readwrite_bug("readw"),0)
#define readl(c)			(__readwrite_bug("readl"),0)
#define writeb(v,c)			__readwrite_bug("writeb")
#define writew(v,c)			__readwrite_bug("writew")
#define writel(v,c)			__readwrite_bug("writel")

#define check_signature(io,sig,len)	(0)

#endif	/* __mem_pci */


#endif
