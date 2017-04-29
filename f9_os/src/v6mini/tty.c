#include "param.h"
#include "systm.h"
#include "user.h"
#include "tty.h"
#include "proc.h"
#include "inode.h"
#include "file.h"
//#include "reg.h"
#include "conf.h"


static const char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0201,0005,0206,0201,0001,
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

static const char	maptab[] ={
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

static char canonb[CANBSIZ];	/* buffer for erase and kill (#@) */



/*
 * routine called on first teletype open.
 * establishes a process group for distribution
 * of quits and interrupts from the tty.
 */
void ttyopen(dev_t dev, struct tty *tp)
{
	struct proc *pp;

	pp = u.u_procp;
	if(pp->p_pgrp == 0) {
		pp->p_pgrp = pp->p_pid;
		u.u_ttyp = tp;
		u.u_ttyd = dev;
		tp->t_pgrp = pp->p_pid;
	}
	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;
}

/*
 * The routine implementing the gtty system call.
 * Just call lower level routine and pass back values.
 */
int v6_gtty(int fdesc, struct tty_parm * parm)
{
	return v6_ioctl(fdesc, TIOCGETP, parm);
}

/*
 * The routine implementing the stty system call.
 * Read in values and call lower level.
 */
int v6_stty(int fdesc, struct tty_parm * parm)
{
	return v6_ioctl(fdesc, TIOCSETP, parm);
}

int v6_ioctl(int fdes, int cmd, ...) {
	va_list ap;
	va_start(ap, cmd);
	int ret = v6_ioctlv(fdes,cmd,ap);
	va_end(ap);
	return ret;
}
/*
 * ioctl system call
 * Check legality, execute common code, and switch out to individual
 * device routine.
 */
int  v6_ioctlv (int fdes, int cmd, va_list va)
{
	struct file *fp;
	struct inode *ip;
	dev_t dev;

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
	ip = fp->f_inode;
	int fmt = ip->i_mode & IFMT;
	if (fmt != IFCHR && fmt != IFMPC) {
		u.u_error = ENOTTY;
		return -1;
	}
	dev = (dev_t)ip->i_addr[0];
	return cdevsw[major(dev)].d_ioctl(dev, cmd, fp->f_flag, va);
}

/*
 * Common code for several tty ioctl commands
 */
int  v6_ioctlcom (struct tty* tp, dev_t dev, int cmd, va_list va)
{
	//unsigned t;
	struct ttiocb* iocb;
	//extern int nldisp;

	switch(cmd) {

	/*
	 * get discipline number
	 */
	case TIOCGETD:
		u.u_error = ENOSYS;
#if 0
		*va_arg(va, int*)= tp->t_line;
#endif
		break;

	/*
	 * set line discipline
	 */
	case TIOCSETD:
		u.u_error = ENOSYS;
#if 0
		if (copyin(addr, (caddr_t)&t, sizeof(t))) {
			u.u_error = EFAULT;
			break;
		}
		if (t >= nldisp) {
			u.u_error = ENXIO;
			break;
		}
		if (tp->t_line)
			(*linesw[tp->t_line].l_close)(tp);
		if (t)
			(*linesw[t].l_open)(dev, tp, addr);
		if (u.u_error==0)
			tp->t_line = t;
#endif
		break;

	/*
	 * prevent more opens on channel
	 */
	case TIOCEXCL:
		u.u_error = ENOSYS;
#if 0
		tp->t_state |= XCLUDE;
#endif
		break;
	case TIOCNXCL:
		u.u_error = ENOSYS;
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
		tp->t_speeds = iocb->ioc_ispeed;
		tp->t_erase = iocb->ioc_erase;
		tp->t_kill = iocb->ioc_kill;
		tp->t_flags = iocb->ioc_flags;
		break;

	/*
	 * send current parameters to user
	 */
	case TIOCGETP:
		iocb = (struct ttiocb*)va_arg(va, struct ttiocb*);
		iocb->ioc_ispeed=tp->t_speeds;
		iocb->ioc_erase= tp->t_erase;
		iocb->ioc_kill=tp->t_kill;
		iocb->ioc_flags=tp->t_flags;
		break;

	/*
	 * Hang up line on last close
	 */

	case TIOCHPCL:
		u.u_error = ENOSYS;
#if 0
		tp->t_state |= HUPCLS;
#endif
		break;

	case TIOCFLUSH:
		flushtty(tp);
		break;

	/*
	 * ioctl entries to line discipline
	 */
	case DIOCSETP:
	case DIOCGETP:
		u.u_error = ENOSYS;
#if 0
		linesw[tp->t_line].l_ioctl(com, tp, addr);
#endif
		break;

	/*
	 * set and fetch special characters
	 */
	case TIOCSETC:
		u.u_error = ENOSYS;
#if 0
		if (copyin(addr, (caddr_t)&tun, sizeof(struct tc)))
			u.u_error = EFAULT;
#endif
		break;

	case TIOCGETC:
		u.u_error = ENOSYS;
#if 0
		if (copyout((caddr_t)&tun, addr, sizeof(struct tc)))
			u.u_error = EFAULT;
#endif
		break;

	default:
		return 1; // + is not handled
	}
	return u.u_error ? -1 : 0;
}
/*
 * Wait for output to drain, then flush input waiting.
 */
void wflushtty(struct tty *tp)
{
	spltty();
	while (tp->t_outq.c_cc) {
		tp->t_state |= ASLEEP;
		ksleep(&tp->t_outq, TTOPRI);
	}
	flushtty(tp);
	spl0();
}



/*
 * flush all TTY queues
 */
void flushtty(struct tty *tp)
{
	int sps;

	while (getclist(&tp->t_canq) >= 0);
	while (getclist(&tp->t_outq) >= 0);
	kwakeup(&tp->t_rawq);
	kwakeup(&tp->t_outq);
	sps = 	spltty();
	while (getclist(&tp->t_rawq) >= 0);
	tp->t_delct = 0;
	splx(sps);
}

/*
 * transfer raw input list to canonical list,
 * doing erase-kill processing and handling escapes.
 * It waits until a full line has been typed in cooked mode,
 * or until any character has been typed in raw mode.
 */
int canon(struct tty *tp)
{
	caddr_t bp;
	caddr_t bp1;
	int c;

	spltty();
	while (tp->t_delct==0) {
		if ((tp->t_state&CARR_ON)==0)
			return(0);
		ksleep(&tp->t_rawq, TTIPRI);
	}
	spl0();
loop:
	bp = &canonb[2];
	while ((c=getclist(&tp->t_rawq)) >= 0) {
		if (c==0377) {
			tp->t_delct--;
			break;
		}
		if ((tp->t_flags&RAW)==0) {
			if (bp[-1]!='\\') {
				if (c==tp->t_erase) {
					if (bp > &canonb[2])
						bp--;
					continue;
				}
				if (c==tp->t_kill)
					goto loop;
				if (c==CEOT)
					continue;
			} else
			if (maptab[c] && (maptab[c]==c || (tp->t_flags&LCASE))) {
				if (bp[-2] != '\\')
					c = maptab[c];
				bp--;
			}
		}
		*bp++ = c;
		if (bp>=canonb+CANBSIZ)
			break;
	}
	bp1 = bp;
	bp = &canonb[2];
	while (bp<bp1)
		putclist(*bp++, &tp->t_canq);
	return(1);
}

/*
 * Place a character on raw TTY input queue, putting in delimiters
 * and waking up top half as needed.
 * Also echo if required.
 * The arguments are the character and the appropriate
 * tty structure.
 */
void ttyinput(int c, struct tty *tp)
{
	int t_flags;

	t_flags = tp->t_flags;
	c &= 0177;
	if (c == '\r' && (t_flags&CRMOD)) c = '\n';
	if ((t_flags&RAW)==0 && (c==CQUIT || c==CINTR)) {
		kgsignal(tp->t_pgrp, c==CINTR? SIGINT:SIGQUIT);
		flushtty(tp);
		return;
	}
	if (tp->t_rawq.c_cc>=TTYHOG) {
		flushtty(tp);
		return;
	}
	if ((t_flags&LCASE) && c>='A' && c<='Z')
		c =+ 'a'-'A';
	putclist(c, &tp->t_rawq);
	if ((t_flags&RAW) || c=='\n' || c==004) {
		kwakeup(&tp->t_rawq);
		if (putclist(0377, &tp->t_rawq)==0)
			tp->t_delct++;
	}
	if (t_flags&ECHO) {
		ttyoutput(c, tp);
		ttstart(tp);
	}
}

/*
 * put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * It is called both from the top half for output, and from
 * interrupt level for echoing.
 * The arguments are the character and the tty structure.
 */
void ttyoutput(int c, struct tty *rtp)
{
	caddr_t colp;
	int ctype;

	c = c&0177;
	/*
	 * Ignore EOT in normal mode to avoid hanging up
	 * certain terminals.
	 */
	if (c==004 && (rtp->t_flags&RAW)==0) return;
	/*
	 * Turn tabs to spaces as required
	 */
	if (c=='\t' && (rtp->t_flags&XTABS)) {
		do
			ttyoutput(' ', rtp);
		while (rtp->t_col&07);
		return;
	}
	/*
	 * for upper-case-only terminals,
	 * generate escapes.
	 */
	if(rtp->t_flags&LCASE) {
		colp = "({)}!|^~'`";
		while(*colp++)
			if(c == *colp++) {
				ttyoutput('\\', rtp);
				c = colp[-2];
				break;
			}
		if ('a'<=c && c<='z') c =+ 'A' - 'a';
	}
	/*
	 * turn <nl> to <cr><lf> if desired.
	 */
	if (c=='\n' && (rtp->t_flags&CRMOD))
		ttyoutput('\r', rtp);
	if (putclist(c, &rtp->t_outq))
		return;
	/*
	 * Calculate delays.
	 * The numbers here represent clock ticks
	 * and are not necessarily optimal for all terminals.
	 * The delays are indicated by characters above 0200,
	 * thus (unfortunately) restricting the transmission
	 * path to 7 bits.
	 */
	colp = &rtp->t_col;
	ctype = partab[c];
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
		ctype = (rtp->t_flags >> 8) & 03;
		if(ctype == 1) { /* tty 37 */
			if (*colp)
				c = max((*colp>>4) + 3, 6);
		} else
		if(ctype == 2) { /* vt05 */
			c = 6;
		}
		*colp = 0;
		break;

	/* tab */
	case 4:
		ctype = (rtp->t_flags >> 10) & 03;
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
		if(rtp->t_flags & VTDELAY) /* tty 37 */
			c = 0177;
		break;

	/* carriage return */
	case 6:
		ctype = (rtp->t_flags >> 12) & 03;
		if(ctype == 1) { /* tn 300 */
			c = 5;
		} else
		if(ctype == 2) { /* ti 700 */
			c = 10;
		}
		*colp = 0;
		break;
	}
	if(c) putclist(c, &rtp->t_outq);
}

/*
 * Restart typewriter output following a delay
 * timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
void ttrstrt(struct tty *tp)
{
	tp->t_state &= ~TIMEOUT;
	ttstart(tp);
}

/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 * If the SSTART bit is off for the tty the work is done here,
 * using the protocol of the single-line interfaces (KL, DL, DC);
 * otherwise the address word of the tty structure is
 * taken to be the name of the device-dependent startup routine.
 */
void ttstart(struct tty *tp)
{
	int s = spl5();

	if((tp->t_state&(TIMEOUT|TTSTOP|BUSY)) == 0)
		(*tp->t_oproc)(tp);
	splx(s);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 * The pc is backed up for the duration of this call.
 * In case of a caught interrupt, an RTI will re-execute.
 */
void ttread(struct tty *tp)
{
	if ((tp->t_state&CARR_ON)==0)
		return;
	if (tp->t_canq.c_cc || canon(tp))
		while (tp->t_canq.c_cc && passc(getc(&tp->t_canq))>=0);
}

/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 */
void ttwrite(struct tty *tp)
{
	int c;

	if ((tp->t_state&CARR_ON)==0)
		return;
	while ((c=cpass())>=0) {
		spl5();
		while (tp->t_outq.c_cc > TTHIWAT) {
			ttstart(tp);
			tp->t_state |= ASLEEP;
			ksleep(&tp->t_outq, TTOPRI);
		}
		spl0();
		ttyoutput(c, tp);
	}
	ttstart(tp);
}

