// helper macros
#ifndef _MY_MACROS_H_
#define _MY_MACROS_H_

#define	SET(t,f)	(t) |= (f)
#define	CLR(t,f)	(t) &= ~(f)
#define	ISSET(t,f)	(((t) & (f))==(f))
#define MIN(A,B)	((A)<((B) ? (A) : (B)))


#define NELM(X) (sizeof((X))/sizeof((X)[0]))
#define TELM(X)  __typeof__((X)[0])
#define PTELM(X)  __typeof__(&(X)[0])
#define for_each_array(NAME,ARRAY) \
	for( __typeof__(&ARRAY[0]) NAME = &ARRAY[0]; NAME != &ARRAY[NELM(ARRAY)]; NAME++)

#define __ALLIGNED(X) __attribute__((aligned(X)))
#define __ALLIGNED32 __ALLIGNED(sizeof(uint32_t))
#define BIT(x) (1<<(x))
#define __NAKED __attribute__((naked))

#define CAST(type, x) ((type)(x)) /* because there are just to many () in this thing */
#define CASTTO(FROM,TO) CAST(__typeof__(FROM), (TO))
#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))


#endif
