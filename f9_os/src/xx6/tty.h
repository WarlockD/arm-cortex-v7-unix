#ifndef  _V7_TTY_
#define _V7_TTY_

#include "types.h"
#include "clist.h"
#include <stdarg.h>

/*
 * A tty structure is needed for
 * each UNIX character device that
 * is used for normal terminal IO.
 * The routines in tty.c handle the
 * common code associated with
 * these structures.
 * The definition and device dependent
 * code is in each driver. (kl.c dc.c dh.c)
 */

struct tc {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};

struct tty
{
	struct	clist t_rawq;	/* input chars right off device */
	struct	clist t_canq;	/* input chars after erase and kill */
	struct	clist t_outq;	/* output list to device */
	int	(* t_oproc)(struct tty*);	/* routine to start output */
	int	(* t_iproc)(struct tty*);	/* routine to start input */
	struct chan *t_chan;	/* destination channel */
	caddr_t	t_linep;	/* aux line discipline pointer */
	caddr_t	t_addr;		/* device address */
	dev_t	t_dev;		/* device number */
	int		t_flags;	/* mode, settable by ioctl call */
	short	t_state;	/* internal state, not visible externally */
	short	t_pgrp;		/* process group name */
	char	t_delct;	/* number of delimiters in raw q */
	char	t_line;		/* line discipline */
	char	t_col;		/* printing column of device */
	char	t_erase;	/* erase character */
	char	t_kill;		/* kill character */
	char	t_char;		/* character temporary */
	char	t_ispeed;	/* input speed */
	char	t_ospeed;	/* output speed */
	union {
		struct tc tc;
		struct clist t_ctlq;
	} t_un;
};
extern struct tty* tty_console;


/*
 * structure of arg for ioctl
 */
struct	ttiocb {
	char	ioc_ispeed;
	char	ioc_ospeed;
	char	ioc_erase;
	char	ioc_kill;
	int	ioc_flags;
};

#define	TTIPRI	28
#define	TTOPRI	29

#define	CERASE	'#'		/* default special characters */
#define	CEOT	004
#define	CKILL	'@'
#define	CQUIT	034		/* FS, cntl shift L */
#define	CINTR	0177		/* DEL */
#define	CSTOP	023		/* Stop output: ctl-s */
#define	CSTART	021		/* Start output: ctl-q */
#define	CBRK	0377

/* limits */
#define	TTHIWAT	100
#define	TTLOWAT	50
#define	TTYHOG	256

/* modes */
// tty modes

#define	TANDEM	BIT(0)
#define	CBREAK	BIT(1)
#define	LCASE	BIT(2)
#define	ECHO	BIT(3)
#define	CRMOD	BIT(4)
#define	RAW		BIT(5)
#define	ODDP	BIT(6)
#define	EVENP	BIT(7)
#
// delays?
#define	NLDELAY	001400
#define	TBDELAY	006000
#define	XTABS	006000
#define	CRDELAY	030000
#define	VTDELAY	040000

/* Hardware bits */
#define	DONE	BIT(7)
#define	IENABLE	BIT(6)

/* Internal state bits */
#define	TIMEOUT	BIT(0)		/* Delay timeout in progress */
#define	WOPEN	BIT(1)		/* Waiting for open to complete */
#define	ISOPEN	BIT(2)		/* Device is open */
#define	FLUSH	BIT(3)		/* outq has been flushed during DMA */
#define	CARR_ON	BIT(4)		/* Software copy of carrier-present */
#define	BUSY	BIT(5)		/* Output in progress */
#define	ASLEEP	BIT(6)		/* Wakeup when output done */
#define	XCLUDE	BIT(7)		/* exclusive-use flag against open */
#define	TTSTOP	BIT(8)		/* Output stopped by ctl-s */
#define	HUPCLS	BIT(9)		/* Hang up upon last close */
#define	TBLOCK	BIT(10)		/* tandem queue blocked */
#define	DKCMD	BIT(11)		/* datakit command channel */
#define	DKMPX	BIT(12)		/* datakit user-multiplexed mode */
#define	DKCALL	BIT(13)		/* datakit dial mode */
#define	DKLINGR	BIT(14)		/* datakit lingering close mode */
#define	CNTLQ	BIT(15)		/* interpret t_un as clist */

/*
 * tty ioctl commands
 */
#define	TIOCGETD	(('t'<<8)|0)
#define	TIOCSETD	(('t'<<8)|1)
#define	TIOCHPCL	(('t'<<8)|2)
#define	TIOCMODG	(('t'<<8)|3)
#define	TIOCMODS	(('t'<<8)|4)
#define	TIOCGETP	(('t'<<8)|8)
#define	TIOCSETP	(('t'<<8)|9)
#define	TIOCSETN	(('t'<<8)|10)
#define	TIOCEXCL	(('t'<<8)|13)
#define	TIOCNXCL	(('t'<<8)|14)
#define	TIOCFLUSH	(('t'<<8)|16)
#define	TIOCSETC	(('t'<<8)|17)
#define	TIOCGETC	(('t'<<8)|18)
#define	DIOCLSTN	(('d'<<8)|1)
#define	DIOCNTRL	(('d'<<8)|2)
#define	DIOCMPX		(('d'<<8)|3)
#define	DIOCNMPX	(('d'<<8)|4)
#define	DIOCSCALL	(('d'<<8)|5)
#define	DIOCRCALL	(('d'<<8)|6)
#define	DIOCPGRP	(('d'<<8)|7)
#define	DIOCGETP	(('d'<<8)|8)
#define	DIOCSETP	(('d'<<8)|9)
#define	DIOCLOSE	(('d'<<8)|10)
#define	DIOCTIME	(('d'<<8)|11)
#define	DIOCRESET	(('d'<<8)|12)
#define	FIOCLEX		(('f'<<8)|1)
#define	FIONCLEX	(('f'<<8)|2)
#define	MXLSTN		(('x'<<8)|1)
#define	MXNBLK		(('x'<<8)|2)

void ttyopen(dev_t dev, struct tty *tp);
void ttychars (struct tty *tp);
void ttyclose (struct tty *tp);
int  ttyioctl(int fdes, int cmd, ...);
int  ttyioctlv (int fdes, int cmd, va_list va);
void wflushtty (struct tty *tp);
void flushtty (struct tty *tp);
int canon (struct tty *tp, char* canonb, size_t csize);
void ttyrend (struct tty *tp, char *pb, char *pe);
int  ttyioctlcom (struct tty* tp, dev_t dev, int cmd, va_list va);
void  ttyblock (struct tty *tp);
void ttyoutput (int c, struct tty *tp);
void ttyinput (int c, struct tty *tp);
void ttrstrt (register struct tty *tp);
void ttstart (struct tty *tp);
#endif
