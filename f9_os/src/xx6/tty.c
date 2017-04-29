#define  __KERNEL__

#include "types.h"
#include "defs.h"
#include "param.h"

//#include "mpu.h"
#include "proc.h"
#include "spinlock.h"
#include "buf.h"
#include "fs.h"
#include "file.h"
#include "tty.h"
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include "device.h"
#include <ctype.h>

#undef errno
extern int errno;

#define	CONTROL		1
#define	BACKSPACE	2
#define	NEWLINE		3
#define	TAB		4
#define	VTAB		5
#define	RETURN		6
static const  char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201
};


/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */

static  const char	maptab[] ={
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,'|',000,000,000,000,000,'`',
	'{','}',000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,'~',000,
	000,'A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W',
	'X','Y','Z',000,000,000,000,000,
};


/*
 * shorthand
 */
#define	q1	tp->t_rawq
#define	q2	tp->t_canq
#define	q3	tp->t_outq
#define	q4	tp->t_un.t_ctlq

struct tty* tty_console=NULL;
/*
 * routine called on first teletype open.
 * establishes a process group for distribution
 * of quits and interrupts from the tty.
 */
void ttyopen(dev_t dev, struct tty *tp)
{
	tp->t_dev = dev;
	if(tty_console == 0) {
		tty_console = tp;
	}
	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;
}


/*
 * set default control characters.
 */
#define	tun	tp->t_un.tc
void ttychars (struct tty *tp)
{
	tun.t_intrc = CINTR;
	tun.t_quitc = CQUIT;
	tun.t_startc = CSTART;
	tun.t_stopc = CSTOP;
	tun.t_eofc = CEOT;
	tun.t_brkc = CBRK;
	tp->t_erase = CERASE;
	tp->t_kill = CKILL;
}

/*
 * clean tp on last close
 */
void  ttyclose (struct tty *tp)
{

	tp->t_pgrp = 0;
	wflushtty(tp);
	if(tp == tty_console) tty_console = NULL;
	tp->t_state = 0;
}

int ttyioctl(int fdes, int cmd, ...) {
	va_list ap;
	va_start(ap, cmd);
	int ret = ttyioctlv(fdes,cmd,ap);
	va_end(ap);
	return ret;
}
/*
 * ioctl system call
 * Check legality, execute common code, and switch out to individual
 * device routine.
 */
int  ttyioctlv (int fdes, int cmd, va_list va)
{
	struct file *fp;
	struct inode *ip;

	if ((fp = getf(fdes)) == NULL) return -1;
#if 0
	if (uap->cmd==FIOCLEX) {
		u.u_pofile[uap->fdes] |= EXCLOSE;
		return;
	}
	if (uap->cmd==FIONCLEX) {
		u.u_pofile[uap->fdes] &= ~EXCLOSE;
		return;
	}
#endif
	ip = fp->ip;
	if (S_ISCHR(ip->type)) {
		dev_t dev = (dev_t)ip->addrs[0];
		return cdevsw[major(dev)].d_ioctl(dev, cmd, ip->type, va);
	} else {
		errno = ENOTTY;
		return -1;
	}
}

/*
 * Common code for several tty ioctl commands
 */
int  v6_ioctlcom (struct tty* tp, dev_t dev, int cmd, va_list va)
{
	uint32_t t;
	struct ttiocb* iocb;
	int ret = 0;
	//extern int nldisp;

	switch(cmd) {

	/*
	 * get discipline number
	 */
	case TIOCGETD:
		*va_arg(va, uint32_t*)= tp->t_line;
		break;

	/*
	 * set line discipline
	 */
	case TIOCSETD:
		t = *va_arg(va, uint32_t*);
		if (t >= nldisp) {
			errno = ENXIO;
			break;
		}
		if (tp->t_line) linesw[(int)tp->t_line].l_close(tp);
		if (t &&(ret=linesw[t].l_open(dev, tp, va))==0)
			tp->t_line = t;
		break;

	/*
	 * prevent more opens on channel
	 */
	case TIOCEXCL:
		errno = ENOSYS;
#if 0
		tp->t_state |= XCLUDE;
#endif
		break;
	case TIOCNXCL:
		errno = ENOSYS;
#if 0
		tp->t_state &= ~XCLUDE;
#endif
		break;

	/*
	 * Set new parameters
	 */
	case TIOCSETP:
		wflushtty(tp);
		// no break
	case TIOCSETN:
		iocb = (struct ttiocb*)va_arg(va, struct ttiocb*);
		tp->t_ispeed = iocb->ioc_ispeed;
		tp->t_ospeed = iocb->ioc_ospeed;
		tp->t_erase = iocb->ioc_erase;
		tp->t_kill = iocb->ioc_kill;
		tp->t_flags = iocb->ioc_flags;
		break;

	/*
	 * send current parameters to user
	 */
	case TIOCGETP:
		iocb = (struct ttiocb*)va_arg(va, struct ttiocb*);
		iocb->ioc_ispeed = tp->t_ispeed;
		iocb->ioc_ospeed = tp->t_ospeed;
		iocb->ioc_erase= tp->t_erase;
		iocb->ioc_kill=tp->t_kill;
		iocb->ioc_flags=tp->t_flags;
		break;

	/*
	 * Hang up line on last close
	 */

	case TIOCHPCL:
		tp->t_state |= HUPCLS;
		break;

	case TIOCFLUSH:
		flushtty(tp);
		break;

	/*
	 * ioctl entries to line discipline
	 */
	case DIOCSETP:
	case DIOCGETP:
		ret=linesw[(int)tp->t_line].l_ioctl(cmd, tp, va);
		break;
	/*
	 * set and fetch special characters
	 */
	case TIOCSETC:
		tp->t_un.tc = *va_arg(va, struct tc*);
		break;

	case TIOCGETC:
		*va_arg(va, struct tc*)=tp->t_un.tc;
		break;

	default:
		ret= 1; // + is not handled
		break;
	}
	return ret;
}

/*
 * Wait for output to drain, then flush input waiting.
 */
void wflushtty (struct tty *tp)
{

	spl5();
	while (tp->t_outq.c_cc && (tp->t_state&CARR_ON)) {
		tp->t_oproc(tp);
		tp->t_state |= ASLEEP;
		//sleep((caddr_t)&tp->t_outq, TTOPRI);
		xv6_sleep(&tp->t_outq,NULL);
	}
	flushtty(tp);
	spl0();
}

/*
 * flush all TTY queues
 */
void flushtty (struct tty *tp)
{
	while (clget(&tp->t_canq) >= 0);
	xv6_wakeup(&tp->t_rawq);
	xv6_wakeup(&tp->t_outq);
	int s = spl6();
	tp->t_state &= ~TTSTOP;
	cdevsw[major(tp->t_dev)].d_stop(tp);
	while (clget(&tp->t_outq) >= 0);
	while (clget(&tp->t_rawq) >= 0);
	tp->t_delct = 0;
	splx(s);
}



/*
 * transfer raw input list to canonical list,
 * doing erase-kill processing and handling escapes.
 * It waits until a full line has been typed in cooked mode,
 * or until any character has been typed in raw mode.
 */
int canon (struct tty *tp, char* canonb, size_t csize)
{
	char *bp;
	char *bp1;
	int c;
	int mc;

	spl5();
	while (((tp->t_flags&(RAW|CBREAK))==0 && tp->t_delct==0)
	    || ((tp->t_flags&(RAW|CBREAK))!=0 && tp->t_rawq.c_cc==0)) {
		if ((tp->t_state&CARR_ON)==0 || tp->t_chan!=NULL) {
			return(0);
		}
		xv6_sleep(&tp->t_rawq,NULL);
		//sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	spl0();
loop:
	bp = &canonb[2];
	while ((c=clget(&tp->t_rawq)) >= 0) {
		if ((tp->t_flags&(RAW|CBREAK))==0) {
			if (c==0377) {
				tp->t_delct--;
				break;
			}
			if (bp[-1]!='\\') {
				if (c==tp->t_erase) {
					if (bp > &canonb[2])
						bp--;
					continue;
				}
				if (c==tp->t_kill)
					goto loop;
				if (c==tun.t_eofc)
					continue;
			} else {
				mc = maptab[c];
				if (c==tp->t_erase || c==tp->t_kill)
					mc = c;
				if (mc && (mc==c || (tp->t_flags&LCASE))) {
					if (bp[-2] != '\\')
						c = mc;
					bp--;
				}
			}
		}
		*bp++ = c;
		if (bp>=canonb+csize)
			break;
	}
	bp1 = &canonb[2];
	b_to_q(bp1, bp-bp1, &tp->t_canq);

	if ((tp->t_state&TBLOCK) && tp->t_rawq.c_cc < TTYHOG/5) {
		if (clput(tun.t_startc, &tp->t_outq)==0) {
			tp->t_state &= ~TBLOCK;
			ttstart(tp);
		}
		tp->t_char = 0;
	}

	return (int)(bp-bp1);
}


/*
 * block transfer input handler.
 */
void ttyrend (struct tty *tp, char *pb, char *pe)
{
	int	tandem;

	tandem = tp->t_flags&TANDEM;
	if (tp->t_flags&RAW) {
		b_to_q(pb, pe-pb, &tp->t_rawq);
	//	if (tp->t_chan)
	//		sdata(tp->t_chan);
	//	else
		xv6_wakeup(&tp->t_rawq);
	} else {
		tp->t_flags &= ~TANDEM;
		while (pb < pe)
			ttyinput(*pb++, tp);
		tp->t_flags |= tandem;
	}
	if (tandem)
		ttyblock(tp);
}

/*
 * Place a character on raw TTY input queue, putting in delimiters
 * and waking up top half as needed.
 * Also echo if required.
 * The arguments are the character and the appropriate
 * tty structure.
 */
static int tk_nin = 0;
void ttyinput (int c, struct tty *tp)
{
	int t_flags;
	//struct chan *cp;

	tk_nin += 1;
	c &= 0377;
	t_flags = tp->t_flags;
	if (t_flags&TANDEM)
		ttyblock(tp);
	if ((t_flags&RAW)==0) {
		c &= 0177;
		if (tp->t_state&TTSTOP) {
			if (c==tun.t_startc) {
				tp->t_state &= ~TTSTOP;
				ttstart(tp);
				return;
			}
			if (c==tun.t_stopc)
				return;
			tp->t_state &= ~TTSTOP;
			ttstart(tp);
		} else {
			if (c==tun.t_stopc) {
				tp->t_state |= TTSTOP;
				cdevsw[major(tp->t_dev)].d_stop(tp);
				return;
			}
			if (c==tun.t_startc)
				return;
		}
		if(c == tun.t_intrc){
			flushtty(tp);
			raise(SIGINT);
			return;
		}
		if(c == tun.t_quitc){
			flushtty(tp);
			raise(SIGQUIT);
			return;
		}
		if (c=='\r' && (t_flags&CRMOD))
			c = '\n';
	}
	if (tp->t_rawq.c_cc>TTYHOG) {
		flushtty(tp);
		return;
	}
	if ((t_flags&LCASE) && isupper(c)) c = tolower(c);

	clput(c, &tp->t_rawq);
	if ((t_flags&(RAW|CBREAK))||
			(c=='\n'||c==tun.t_eofc||c==tun.t_brkc)) {
		if ((t_flags&(RAW|CBREAK))==0 && clput(0377, &tp->t_rawq)==0)
			tp->t_delct++;
		//if ((cp=tp->t_chan)!=NULL)
		//	sdata(cp); else
			xv6_wakeup(&tp->t_rawq);
	}
	if (t_flags&ECHO) {
		ttyoutput(c, tp);
		if (c==tp->t_kill && (t_flags&(RAW|CBREAK))==0)
			ttyoutput('\n', tp);
		ttstart(tp);
	}
}


/*
 * Send stop character on input overflow.
 */
void  ttyblock (struct tty *tp)
{
	int x;
	x = q1.c_cc + q2.c_cc;
	if (q1.c_cc > TTYHOG) {
		flushtty(tp);
		tp->t_state &= ~TBLOCK;
	}
	if (x >= TTYHOG/2) {
		if (clput(tun.t_stopc, &tp->t_outq)==0) {
			tp->t_state |= TBLOCK;
			tp->t_char++;
			ttstart(tp);
		}
	}
}

/*
 * put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * It is called both from the top half for output, and from
 * interrupt level for echoing.
 * The arguments are the character and the tty structure.
 */
static int tk_nout = 0;
void ttyoutput (int c, struct tty *tp)
{
	tk_nout += 1;
	/*
	 * Ignore EOT in normal mode to avoid hanging up
	 * certain terminals.
	 * In raw mode dump the char unchanged.
	 */

	if ((tp->t_flags&RAW)==0) {
		c &= 0177;
		if (c==CEOT) return;
	} else {
		clput(c, &tp->t_outq);
		return;
	}

	/*
	 * Turn tabs to spaces as required
	 */
	if (c=='\t' && (tp->t_flags&TBDELAY)==XTABS) {
		for(int c=8; c >= 0 && tp->t_col&07;c--)
			ttyoutput(' ', tp);
		return;
	}
	/*
	 * for upper-case-only terminals,
	 * generate escapes.
	 */
	if (tp->t_flags&LCASE) {
		if (islower(c))
			c = toupper(c);
		else {
			const char* punc = "({)}!|^~'`";
			while(*punc++)
				if(c == *punc++) {
					ttyoutput('\\', tp);
					c = punc[-2];
					break;
			}
		}
	}
	/*
	 * turn <nl> to <cr><lf> if desired.
	 */
	if (c=='\n' && (tp->t_flags&CRMOD))
		ttyoutput('\r', tp);
	clput(c, &tp->t_outq);
	/*
	 * Calculate delays.
	 * The numbers here represent clock ticks
	 * and are not necessarily optimal for all terminals.
	 * The delays are indicated by characters above 0200.
	 * In raw mode there are no delays and the
	 * transmission path is 8 bits wide.
	 */
	char* colp = &tp->t_col;
	char ctype = partab[c];
	c = 0;
	switch (ctype&077) {

	/* ordinary */
	case 0:
		(*colp)++;

	/* non-printing */
		// no break
	case 1:
		break;

	/* backspace */
	case 2:
		if (*colp)
			(*colp)--;
		break;

	/* newline */
	case 3:
		ctype = (tp->t_flags >> 8) & 03;
		if(ctype == 1) { /* tty 37 */
			if (*colp)
				c = max(((unsigned)*colp>>4) + 3, (unsigned)6);
		} else
		if(ctype == 2) { /* vt05 */
			c = 6;
		}
		*colp = 0;
		break;

	/* tab */
	case 4:
		ctype = (tp->t_flags >> 10) & 03;
		if(ctype == 1) { /* tty 37 */
			c = 1 - (*colp | ~07);
			if(c < 5)
				c = 0;
		}
		*colp |= 07;
		(*colp)++;
		break;

	/* vertical motion */
	case 5:
		if(tp->t_flags & VTDELAY) /* tty 37 */
			c = 0177;
		break;

	/* carriage return */
	case 6:
		ctype = (tp->t_flags >> 12) & 03;
		if(ctype == 1) { /* tn 300 */
			c = 5;
		} else if(ctype == 2) { /* ti 700 */
			c = 10;
		}
		*colp = 0;
	}
	if(c) clput(c & 0xFF, &tp->t_outq);
}

/*
 * Restart typewriter output following a delay
 * timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
void ttrstrt (register struct tty *tp)
{

	tp->t_state &= ~TIMEOUT;
	ttstart(tp);
}

/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 */
void ttstart (struct tty *tp)
{
	int s = spl5();
	if((tp->t_state&(TIMEOUT|TTSTOP|BUSY)) == 0)
		tp->t_oproc(tp);
	splx(s);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
int ttread(struct tty *tp, char* data, size_t size)
{
	int c;
	if ((tp->t_state&CARR_ON)==0)
		return(0);
	return canon(tp,data,size);
#if 0
	if (tp->t_canq.c_cc || canon(tp,data,size)){
		while (size-- && tp->t_canq.c_cc &&  (c= clget(&tp->t_canq)) >=0)
			*data++ = c;
	}
	return(tp->t_rawq.c_cc + tp->t_canq.c_cc);
#endif
}

/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 */
int ttwrite(struct tty *tp, const char* data, size_t size)
{
	int c;
	if ((tp->t_state&CARR_ON)==0) return 0;
	while (size--) {
		spl5();
		// more than 100 charaters on this list, so wait
		while (tp->t_outq.c_cc > TTHIWAT) {
			ttstart(tp);
			tp->t_state |= ASLEEP;
			//if (tp->t_chan)
			//	return((caddr_t)&tp->t_outq);
			//sleep((caddr_t)&tp->t_outq, TTOPRI);
			xv6_sleep(&tp->t_outq, NULL);
		}
		spl0();
		c = *data++;
		ttyoutput(c, tp);
	}
	ttstart(tp);
	return 0;
}
