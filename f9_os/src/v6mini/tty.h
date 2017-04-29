#ifndef _MXSYS_TTY_H_
#define _MXSYS_TTY_H_

#include "param.h"
#include "types.h"
#include <sys\types.h>
#include <stdint.h>

// taken from v7
//extern char	canonb[CANBSIZ];	/* buffer for erase and kill (#@) */
/*
 * A clist structure is the head
 * of a linked list queue of characters.
 * The characters are stored in 4-word
 * blocks containing a link and several characters.
 * The routines getc and putc
 * manipulate these structures.
 */
struct clist
{
	int	c_cc;		/* character count */
	caddr_t c_cf;		/* pointer to first char */
	caddr_t	c_cl;		/* pointer to last char */
};
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
// tty settings, use termos latter
struct tty_parm {
	int t_speeds;
	int t_erase;
	int t_kill;
	int t_flags;
};
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
struct tty
{
	struct	clist t_rawq;	/* input chars right off device */
	struct	clist t_canq;	/* input chars after erase and kill */
	struct	clist t_outq;	/* output list to device */
	int	t_flags;	/* mode, settable by stty call */
	void (*t_oproc)(struct tty*); /* (register or startup fcn) */
	//int	*t_addr;	/* device address (register or startup fcn) */
	char	t_delct;	/* number of delimiters in raw q */
	char	t_col;		/* printing column of device */
	char	t_erase;	/* erase character */
	char	t_kill;		/* kill character */
	char	t_state;	/* internal state, not visible externally */
	char	t_char;		/* character temporary */
	int	t_speeds;	/* output+input line speed */
	int	t_pgrp;		/* process group name */
};


#define	TTIPRI	10
#define	TTOPRI	20

#define	CERASE	'#'		/* default special characters */
#define	CEOT	004
#define	CKILL	'@'
#define	CQUIT	034		/* FS, cntl shift L */
#define	CINTR	0177		/* DEL */

/* limits */
#define	TTHIWAT	90
#define	TTLOWAT	30
#define	TTYHOG	256

/* modes */
#define	HUPCL	01
#define	XTABS	02
#define	LCASE	04
#define	ECHO	010
#define	CRMOD	020
#define	RAW	040
#define	ODDP	0100
#define	EVENP	0200
#define	NLDELAY	001400
#define	TBDELAY	006000
#define	CRDELAY	030000
#define	VTDELAY	040000

/* Hardware bits */
#define	DONE	0200
#define	IENABLE	0100

/* Internal state bits */
#define	TIMEOUT	01		/* Delay timeout in progress */
#define	WOPEN	02		/* Waiting for open to complete */
#define	ISOPEN	04		/* Device is open */
#define	SSTART	010		/* Has special start routine at addr */
#define	CARR_ON	020		/* Software copy of carrier-present */
#define	BUSY	040		/* Output in progress */
#define	ASLEEP	0100		/* Wakeup when output done */
#define	TTSTOP	0400		/* Output stopped by ctl-s */

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


int v6_gtty(int fdesc, struct tty_parm * parm);
int v6_stty(int fdesc, struct tty_parm * parm);
void ttyopen(dev_t dev, struct tty *tp);
void ttychars (struct tty *tp);
void ttyclose (struct tty *tp);
void wflushtty(struct tty *tp);
void flushtty(struct tty *tp);
int canon(struct tty *tp);

void cinit();
int getclist(struct clist*l);
int putclist(int c, struct clist* l);
int getwlist(struct clist*l);
int putwlist(int c, struct clist* l);
int q_to_b (struct clist *q, caddr_t cp, int cc);
int ndqb (struct clist *q, int flag);
void ndflush (struct clist *q, int cc);
int b_to_q (caddr_t cp, int cc, struct clist *q);

int v6_ioctl(int fdes, int cmd, ...);
int v6_ioctlcom(struct tty* tp, dev_t dev, int cmd, va_list va);
int v6_ioctlv(int fdes, int cmd, va_list va);
void ttyinput(int ac, struct tty *tp);
void ttyoutput(int ac, struct tty *tp);
void ttstart(struct tty *tp);
void ttrstrt(struct tty *tp);
void ttread(struct tty *tp);

#endif
