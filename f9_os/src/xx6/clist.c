#include "clist.h"
#include "arm.h"
#include <sys\queue.h>
#include <assert.h>
#include <string.h>


struct cblock {
	STAILQ_ENTRY(cblock) queue;
	char	c_info[CBSIZE];
};

static struct cblock		 cfree[NCLIST] ;
static STAILQ_HEAD(, cblock) cfreelist;

static inline struct cblock* clalloc() {
	if(STAILQ_EMPTY(&cfreelist)) return NULL;
	struct cblock* cl = STAILQ_FIRST(&cfreelist);
	STAILQ_REMOVE_HEAD(&cfreelist,queue);
	return cl;
}
static inline void clfree(struct cblock* cl) {
	STAILQ_INSERT_HEAD(&cfreelist, cl, queue);
}

/*
 * So how clists works is its just a link list of pointers to the next block
 * now why they don't do this in the clist structure? humm
 */
/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
void cinit (void)
{
	STAILQ_INIT(&cfreelist);
	for_each_array(cp,cfree) {
		memset(cp,0,sizeof(*cp));
		clfree(cp);
	}
#if 0
 //we don't count charater devices here
	ccp = 0;
	for(cdp = cdevsw; cdp->d_open; cdp++)
		ccp++;
	nchrdev = ccp;
#endif
}
static int _clget (struct clist *p) {
	int c;
	if (p->c_cc <= 0) {
		c = -1;
		p->queue.stqh_first = NULL;
		p->queue.stqh_last = &p->queue.stqh_first;
		p->c_cc = 0;
	} else {
		struct cblock* cl = STAILQ_FIRST(&p->queue);
		--p->c_cc;
		c = cl->c_info[p->c_cc / CBSIZE];
		cl->c_info[p->c_cc / CBSIZE] = 0; // clear it
		if((p->c_cc % CBSIZE)==0) {
			STAILQ_REMOVE_HEAD(&p->queue,queue);
			clfree(cl);
		}
	}
	return(c);
}
static int _clput (int c, struct clist *p) {
	struct cblock* cb;
	if((p->c_cc % CBSIZE)==0) {
		cb = clalloc();
		if(cb == NULL) return -1;
		STAILQ_INSERT_TAIL(&p->queue, cb, queue);
	} else cb = STAILQ_LAST(&p->queue, cblock, queue);
	cb->c_info[p->c_cc / CBSIZE] = c;
	++p->c_cc;
	return(c);
}

static int _ndqb (struct clist *q, int flag) {
	int cc=0;
	if (q->c_cc <= 0) cc=-q->c_cc;
	else if(flag==0) cc=q->c_cc;
	else {
		struct cblock* cb;
		STAILQ_FOREACH(cb, &q->queue,queue) {
			for_each_array(c, cb->c_info) {
				if(*c&flag) return cc;
				cc++;
			}
		}
	}
	return cc;
}
/*
 * Character list get/put
 */
int clget (struct clist *p)
{
	int s = spl6();
	int c = _clget(p);
	splx(s);
	return c;
}

/*
 * copy clist to buffer.
 * return number of bytes moved.
 */
int q_to_b (struct clist *p, char *cp, int cc)
{
	char* acp=cp; // save
	int s = spl6();
	while(cc && p->c_cc)*cp++ = _clget(p);
	splx(s);
	return (int)(cp-acp);
}


/*
 * Return count of contiguous characters
 * in clist starting at q->c_cf.
 * Stop counting if flag&character is non-null.
 */
int ndqb (struct clist *q, int flag)
{
	int s = spl6();
	int cc = _ndqb(q,flag);
	splx(s);
	return cc;
}



/*
 * Update clist to show that cc characters
 * were removed.  It is assumed that cc < CBSIZE.
 */
void ndflush (struct clist *q, int cc)
{
	assert(cc < CBSIZE);
	int s = spl6();
	// eh its fast enough, work on it better latter
	while(cc--) _clget(q);
	splx(s);
}
int clput (int c, struct clist *p)
{
	int s = spl6();
	int r = _clput(c,p);
	splx(s);
	return r;
}


/*
 * copy buffer to clist.
 * return number of bytes not transfered.
 */
int b_to_q (const char *cp, int cc, struct clist *q)
{
	const char* acp=cp; // save
	int s = spl6();
	while(cc-- && _clput(*cp++,q)!=-1);
	splx(s);
	return (int)(cp-acp);
}



/*
 * integer (2-byte) get/put
 * using clists
 */
int clgetw (struct clist *p)
{
	if (p->c_cc <= 1) return(-1);
	 int s = spl6();
	 int r = (_clget(p) | (_clget(p)<<8));
	 splx(s);
	 return r;
}

int clputw (int c, struct clist *p)
{
	if(STAILQ_EMPTY(&cfreelist)) return -1;
	int s = spl6();
	_clput(c, p);
	_clput(c>>8, p);
	splx(s);
	return(0);
}
