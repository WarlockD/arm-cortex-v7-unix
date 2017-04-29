#ifndef _V7_CLIST_H_
#define _V7_CLIST_H_

#include "types.h"
#include <sys\queue.h>
// clist parms

#define	NCLIST	50		/* max total clist size */
#define	CBSIZE	16		/* dosn't HAVE to be ^2 */

/*
 * A clist structure is the head
 * of a linked list queue of characters.
 * The characters are stored in 4-word
 * blocks containing a link and several characters.
 * The routines getc and putc
 * manipulate these structures.
 */

struct cblock;
struct clist
{
	int		c_cc;		/* character count */
	STAILQ_HEAD(, cblock) queue;
	//struct cblock	*c_cf;		/* pointer to first char */
	//struct cblock	*c_cl;		/* pointer to last char */
};

void cinit (void);
int clget (struct clist *p);
int clput (int c, struct clist *p);
int q_to_b (struct clist *q, char *cp, int cc);
int b_to_q (const char *cp, int cc, struct clist *q);
int ndqb (struct clist *q, int flag);
void ndflush (struct clist *q, int cc);
int clputw (int c, struct clist *p);
int clgetw (struct clist *p);





#endif
