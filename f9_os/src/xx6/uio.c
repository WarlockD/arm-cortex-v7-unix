#include "uio.h"
#include <errno.h>
#include <string.h>
void panic(const char*);
#define SEG6 40000
// don't use uio yet
// hacks for now
int copyiout(void* from , void* to, size_t cnt) {
	memcpy(to, from, cnt);
	return 0;
}

int copyiin(void* from , void* to, size_t cnt){
	memcpy(to, from, cnt);
		return 0;
}
int copyout(void* from , void* to, size_t cnt) {
	memcpy(to, from, cnt);
	return 0;
}

int copyin(void* from , void* to, size_t cnt){
	memcpy(to, from, cnt);
		return 0;
}
int subyte(void* address, uint8_t data) { *((uint8_t*)address) = data; return 0; }
int suibyte(void* address, uint8_t data) { *((uint8_t*)address) = data; return 0; }

int fubyte(void* address) { return *((uint8_t*)address); }
int fuibyte(void* address) { return *((uint8_t*)address); }

/* copied, for supervisory networking, to sys_net.c */
int uiomove (caddr_t cp, uint32_t n, struct uio *uio)
{
	struct iovec *iov;
	int error = 0;
	uint32_t cnt;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		switch (uio->uio_segflg) {

		case UIO_USERSPACE:
#if 0
			if (cnt > 100 && cp + cnt < SEG6)
				error = uiofmove(cp, cnt, uio, iov);
			else if ((cnt | (int)cp | (int)iov->iov_base) & 1)
				if (uio->uio_rw == UIO_READ)
					error = vcopyout(cp,iov->iov_base, cnt);
				else
					error = vcopyin(iov->iov_base, cp, cnt);
			else {
#endif
				if (uio->uio_rw == UIO_READ)
					error = copyout(cp, iov->iov_base, cnt);
				else
					error = copyin(iov->iov_base, cp, cnt);
#if 0
			}
#endif
			if (error)
				return (error);
			break;

		case UIO_USERISPACE:
			if (cnt > 100 && cp + cnt < SEG6)
				error = uiofmove(cp, cnt, uio, iov);
			else if (uio->uio_rw == UIO_READ)
				error = copyiout(cp, iov->iov_base, cnt);
			else
				error = copyiin(iov->iov_base, cp, cnt);
			if (error)
				return (error);
			break;

		case UIO_SYSSPACE:
			if (uio->uio_rw == UIO_READ)
				memcpy(iov->iov_base, cp, cnt);
			else
				memcpy(cp, iov->iov_base, cnt);
			break;
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

/* copied, for supervisory networking, to sys_net.c */
/*
 * Give next character to user as result of read.
 */
int ureadc (int c, struct uio *uio)
{
	register struct iovec *iov;

again:
	if (uio->uio_iovcnt == 0)
		panic("ureadc");
	iov = uio->uio_iov;
	if (iov->iov_len == 0 || uio->uio_resid == 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	switch (uio->uio_segflg) {

	case UIO_USERSPACE:
		if (subyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;

	case UIO_SYSSPACE:
		*iov->iov_base = c;
		break;

	case UIO_USERISPACE:
		if (suibyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;
	}
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

/* copied, for supervisory networking, to sys_net.c */
/*
 * Get next character written in by user from uio.
 */
int uwritec (struct uio *uio)
{
	struct iovec *iov;
	int c;

	if (uio->uio_resid == 0)
		return (-1);
again:
	if (uio->uio_iovcnt <= 0)
		panic("uwritec");
	iov = uio->uio_iov;
	if (iov->iov_len == 0) {
		uio->uio_iov++;
		if (--uio->uio_iovcnt == 0)
			return (-1);
		goto again;
	}
	switch (uio->uio_segflg) {

	case UIO_USERSPACE:
		c = fubyte(iov->iov_base);
		break;

	case UIO_SYSSPACE:
		c = *iov->iov_base & 0377;
		break;

	case UIO_USERISPACE:
		c = fuibyte(iov->iov_base);
		break;
	}
	if (c < 0)
		return (-1);
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (c & 0377);
}

/*
 * Copy bytes to/from the kernel and the user.  Uiofmove assumes the kernel
 * area being copied to or from does not overlap segment 6 - the assembly
 * language helper routine, fmove, uses segment register 6 to map in the
 * user's memory.
 */
int uiofmove (void* cp, int n, struct uio *uio, struct iovec *iov)
{
#if 1
	// need ot
	if (uio->uio_rw == UIO_READ)
		memmove(cp, iov->iov_base, n);
	else
		memmove(iov->iov_base,cp,  n);
	return 0;
#else
	register short c;
	short on;
	register short segr;		/* seg register (0 - 7) */
	u_short *segd;			/* PDR map array */
	u_short *sega;			/* PAR map array */
	int error;

#ifdef NONSEPARATE
	segd = UISD;
	sega = UISA;
#else
	if (uio->uio_segflg == UIO_USERSPACE && u.u_sep) {
		segd = UDSD;
		sega = UDSA;
	}
	else {
		segd = UISD;
		sega = UISA;
	}
#endif

	segr = (short)iov->iov_base >> 13 & 07;
	on = (short)iov->iov_base & 017777;
	c = MIN(n, 8192-on);
	for (;;) {
		if (uio->uio_rw == UIO_READ)
			error = fmove(sega[segr], segd[segr], cp, SEG6+on, c);
		else
			error = fmove(sega[segr], segd[segr], SEG6+on, cp, c);
		if (error)
			return(error);
		n -= c;
		if (!n)
			return(0);
		cp += c;
		segr++;
		segr &= 07;
		on = 0;
		c = MIN(n, 8192);
	}
	/*NOTREACHED*/
#endif
}
