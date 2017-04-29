#include <stm32f7xx.h>
#include <stm32746g_discovery.h>
#include <pin_macros.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <os\printk.h>
#include <unistd.h>
#include <assert.h>
#include <sys\stat.h>
#include "..\v6mini\inode.h"

#define DIRSIZ 14

#define	NFILES	1024
FILE*		pwdf, dirf;
char	stdbuf[BUFSIZ];

struct lbuf {
	union {
		char	lname[15];
		char	*namep;
	} ln;
	char	ltype;
	short	lnum;
	short	lflags;
	short	lnl;
	short	luid;
	short	lgid;
	long	lsize;
	long	lmtime;
};

int	aflg, dflg, lflg, sflg, tflg, uflg, iflg, fflg, gflg, cflg;
int	rflg	= 1;
long	year;
int	flags;
int	lastuid	= -1;
char	tbuf[16];
long	tblocks;
int	statreq;
struct	lbuf	*flist[NFILES];
struct	lbuf	**lastp = flist;
struct	lbuf	**firstp = flist;
char	*dotp	= ".";

static void readdir(char *dir);
static char * makename(char *dir,char *file);
static int getname(uid_t uid, char *buf);
static int compar(struct lbuf **pp1,struct lbuf **pp2);
static struct lbuf * gstat(char *file, int  argfl);

#define	ISARG	0100000

off_t nblock(off_t size) { return((size+511)>>9); }

static const char* pmode(int flags)
{
	static const int	m1[] = { 1, IREAD, 	'r', '-' };
	static const int	m2[] = { 1, IWRITE>>0, 	'w', '-' };
	static const int	m3[] = { 2, S_ISUID, 		's', IEXEC>>0, 'x', '-' };
	static const int	m4[] = { 1, IREAD>>3, 	'r', '-' };
	static const int	m5[] = { 1, IWRITE>>3, 	'w', '-' };
	static const int	m6[] = { 2, S_ISGID, 		's', IEXEC>>3, 'x', '-' };
	static const int	m7[] = { 1, IREAD>>6, 	'r', '-' };
	static const int	m8[] = { 1, IWRITE>>6, 	'w', '-' };
	static const int	m9[] = { 2, S_ISVTX, 		't', IEXEC>>6, 'x', '-' };
	static const int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};
	int i;
	static char pmodebuf[(sizeof(m)/sizeof(m[0]))+1];
	for(i = 0; i < (sizeof(m)/sizeof(m[0])) ;i++) {
		const int* pairp = m[i];
		int n = *pairp++;
		while (--n>=0 && (flags&*pairp++)==0)
			pairp++;
		pmodebuf[i] = *pairp;
	}
	pmodebuf[i] = 0;
	return pmodebuf;
}

static void pentry(struct lbuf *p)
{
	int  t;
	char *cp;

	if (p->lnum == -1)
		return;
	if (iflg)
		printk("%5u ", p->lnum);
	if (sflg)
	printk("%4D ", nblock(p->lsize));
	if (lflg) {
		printk("%c%s%2d", p->ltype,pmode(p->lflags), p->lnl);
		t = p->luid;
		if(gflg)
			t = p->lgid;
		if (getname(t, tbuf)==0)
			printk("%-6.6s", tbuf);
		else
			printk("%-6d", t);
		if (p->ltype=='b' || p->ltype=='c')
			printk("%3d,%3d", major((int)p->lsize), minor((int)p->lsize));
		else
			printk("%7ld", p->lsize);
		cp = ctime(&p->lmtime);
		if(p->lmtime < year)
			printk(" %-7.7s %-4.4s ", cp+4, cp+20); else
			printk(" %-12.12s ", cp+4);
	}
	if (p->lflags&ISARG)
		printk("%s\n", p->ln.namep);
	else
		printk("%.14s\n", p->ln.lname);
}
static int getname(uid_t uid, char *buf)
{
	int j, c, n, i;

	if (uid==lastuid)
		return(0);
	if(pwdf == NULL)
		return (-1);
	rewind(pwdf);
	lastuid = -1;
	do {
		i = 0;
		j = 0;
		n = 0;
		while((c=fgetc(pwdf)) != '\n') {
			if (c==EOF)
				return (-1);
			if (c==':') {
				j++;
				c = '0';
			}
			if (j==0)
				buf[i++] = c;
			if (j==2)
				n = n*10 + c - '0';
		}
	} while (n != uid);
	buf[i++] = '\0';
	lastuid = uid;
	return(0);
}

static char * makename(char *dir, char *file)
{
	static char dfile[100];
	register char *dp, *fp;
	register int i;

	dp = dfile;
	fp = dir;
	while (*fp)
		*dp++ = *fp++;
	*dp++ = '/';
	fp = file;
	for (i=0; i<DIRSIZ; i++)
		*dp++ = *fp++;
	*dp = 0;
	return(dfile);
}
#define MATCHDOT (PTR) (PTR[0] == '.' && PTR[1] == '\0')
#define MATCHDOTDOT(PTR) (PTR[0] == '.' && PTR[1] == '.' && PTR[2] == '\0')
#define MATCHANYDOT(PTR) (PTR[0] == '.' && (PTR[1] == '\0' || (PTR[1] == '.' && PTR[2] == '\0')))
struct	dirent {
	uint16_t d_ino;
	char d_name[14];
};
static void readdir(char *dir)
{
	static struct	dirent dentry;
	register int j;
	register struct lbuf *ep;
	int fdes=0;
	if ((fdes = open(dir, O_RDONLY))< 0) {
		printk("%s unreadable\n", dir);
		return;
	}
	tblocks = 0;
	for(;;) {
		int ret = read(fdes, (char *)&dentry, sizeof(dentry));
		if(ret <= 0) break;
		assert(ret == sizeof(dentry));
		//if (v7_read(fdes, (char *)&dentry, sizeof(dentry)) > 0)
		//	break;
		if (dentry.d_ino==0
		 || (aflg==0 && MATCHANYDOT(dentry.d_name)))
			continue;
		ep = gstat(makename(dir, dentry.d_name), 0);
		if (ep==NULL)
			continue;
		if (ep->lnum != -1)
			ep->lnum = dentry.d_ino;
		for (j=0; j<DIRSIZ; j++)
			ep->ln.lname[j] = dentry.d_name[j];
	}
	close(fdes);
}
void ls(int argc, const char *argv[])
{
	int i;
	struct lbuf *ep, **ep1;
	struct lbuf **slastp;
	struct lbuf **epp;
	struct lbuf lb;
	char *t;
	int compar();

//	setbuf(stdout, stdbuf);
	time(&lb.lmtime);
	year = lb.lmtime - 6L*30L*24L*60L*60L; /* 6 months ago */
	if (--argc > 0 && *argv[1] == '-') {
		argv++;
		while (*++*argv) switch (**argv) {

		case 'a':
			aflg++;
			continue;

		case 's':
			sflg++;
			statreq++;
			continue;

		case 'd':
			dflg++;
			continue;

		case 'g':
			gflg++;
			continue;

		case 'l':
			lflg++;
			statreq++;
			continue;

		case 'r':
			rflg = -1;
			continue;

		case 't':
			tflg++;
			statreq++;
			continue;

		case 'u':
			uflg++;
			continue;

		case 'c':
			cflg++;
			continue;

		case 'i':
			iflg++;
			continue;

		case 'f':
			fflg++;
			continue;

		default:
			continue;
		}
		argc--;
	}
	if (fflg) {
		aflg++;
		lflg = 0;
		sflg = 0;
		tflg = 0;
		statreq = 0;
	}
	if(lflg) {
		t = "/etc/passwd";
		if(gflg)
			t = "/etc/group";
		pwdf = fopen(t, "r");
	}


#if 1
	struct stat st;
	//v7_fstat("/dev", & st);
	//readdir("/dev");
	if ((ep = gstat("/dev", 1))==NULL){
		printk("error: %d\n", argc);
		while(1);
	}

	ep->ln.namep = *argv;
	ep->lflags |= ISARG;

#else
	if (argc==0) {
		argc++;
		argv = &dotp - 1;
	}
	for (i=0; i < argc; i++) {
		if ((ep = gstat(*++argv, 1))==NULL)
			continue;
		ep->ln.namep = *argv;
		ep->lflags |= ISARG;
	}
#endif
	qsort(firstp, lastp - firstp, sizeof (*lastp), compar);
	slastp = lastp;
	for (epp=firstp; epp<slastp; epp++) {
		ep = *epp;
		if (ep->ltype=='d' && dflg==0 || fflg) {
			if (argc>1)
				printk("\n%s:\n", ep->ln.namep);
			lastp = slastp;
			readdir(ep->ln.namep);
			if (fflg==0)
				qsort(slastp,lastp - slastp,sizeof *lastp,compar);
			if (lflg || sflg)
				printk("total %D\n", tblocks);
			for (ep1=slastp; ep1<lastp; ep1++)
				pentry(*ep1);
		} else
			pentry(ep);
	}
	exit(0);
}



static struct lbuf * gstat(char *file, int  argfl)
{
	struct stat statb;
	register struct lbuf *rep;
	static int nomocore;

	if (nomocore)
		return(NULL);
	rep = (struct lbuf *)malloc(sizeof(struct lbuf));
	if (rep==NULL) {
		fprintf(stderr, "ls: out of memory\n");
		nomocore = 1;
		return(NULL);
	}
	if (lastp >= &flist[NFILES]) {
		static int msg;
		lastp--;
		if (msg==0) {
			fprintf(stderr, "ls: too many files\n");
			msg++;
		}
	}
	*lastp++ = rep;
	rep->lflags = 0;
	rep->lnum = 0;
	rep->ltype = '-';
	if (argfl || statreq) {
		if (stat(file, &statb)<0) {
			printk("%s not found\n", file);
			statb.st_ino = -1;
			statb.st_size = 0;
			statb.st_mode = 0;
			if (argfl) {
				lastp--;
				return(0);
			}
		}
		rep->lnum = statb.st_ino;
		rep->lsize = statb.st_size;
		switch(statb.st_mode&S_IFMT) {

		case S_IFDIR:
			rep->ltype = 'd';
			break;

		case S_IFBLK:
			rep->ltype = 'b';
			rep->lsize = statb.st_rdev;
			break;

		case S_IFCHR:
			rep->ltype = 'c';
			rep->lsize = statb.st_rdev;
			break;
		}
		rep->lflags = statb.st_mode & ~S_IFMT;
		rep->luid = statb.st_uid;
		rep->lgid = statb.st_gid;
		rep->lnl = statb.st_nlink;
		if(uflg)
			rep->lmtime = statb.st_atime;
		else if (cflg)
			rep->lmtime = statb.st_ctime;
		else
			rep->lmtime = statb.st_mtime;
		tblocks += nblock(statb.st_size);
	}
	return(rep);
}

static int compar(struct lbuf **pp1, struct lbuf **pp2)
{
	struct lbuf *p1, *p2;

	p1 = *pp1;
	p2 = *pp2;
	if (dflg==0) {
		if ((p1->lflags&ISARG) && p1->ltype=='d') {
			if (!(p2->lflags&ISARG && p2->ltype=='d'))
				return(1);
		} else {
			if ((p2->lflags&ISARG) && p2->ltype=='d')
				return(-1);
		}
	}
	if (tflg) {
		if(p2->lmtime == p1->lmtime)
			return(0);
		if(p2->lmtime > p1->lmtime)
			return(rflg);
		return(-rflg);
	}
	return(rflg * strcmp(p1->lflags&ISARG? p1->ln.namep: p1->ln.lname,
				p2->lflags&ISARG? p2->ln.namep: p2->ln.lname));
}


