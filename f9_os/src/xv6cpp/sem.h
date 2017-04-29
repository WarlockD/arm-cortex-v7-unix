/*
 * sem.h
 *
 *  Created on: Apr 21, 2017
 *      Author: Paul
 */

#ifndef XV6CPP_SEM_H_
#define XV6CPP_SEM_H_

#include "os.h"
#include "list.h"
#if 0
namespace xv6 {
	static constexpr size_t SEMHOLDERSIZE = 20;
	static constexpr size_t SEMSIZE = 20;

	struct semholder_t
	{
		pid_t pid;
		int16_t counts;
		slist_entry<semholder_t> link;
		semholder_t() : pid(0), counts(0) {}
	};
/* This is the generic semaphore structure. */

struct sem_t
{
  volatile int16_t semcount;     /* >0 -> Num counts available */
                                 /* <0 -> Num tasks waiting for semaphore */
  /* If priority inheritance is enabled, then we have to keep track of which
   * tasks hold references to the semaphore.
   */
  bool prio_inheritance_enabled;
  slist_head<semholder_t> hhead; /* List of holders of semaphore counts */
  sem_t() : semcount(0),prio_inheritance_enabled(false) {}
  static sem_t* create(int pshared, unsigned int value);
  static sem_t* destroy();
  void wait();
  void timedwait(struct timespec*);
  void trywait();
  void post();
  int getvalue();

  int        sem_init(FAR sem_t *sem, int pshared, unsigned int value);
  int        sem_destroy(FAR sem_t *sem);
  int        sem_wait(FAR sem_t *sem);
  int        sem_timedwait(FAR sem_t *sem, FAR const struct timespec *abstime);
  int        sem_trywait(FAR sem_t *sem);
  int        sem_post(FAR sem_t *sem);
  int        sem_getvalue(FAR sem_t *sem, FAR int *sval);

};


/* Initializers */

#ifdef CONFIG_PRIORITY_INHERITANCE
# if CONFIG_SEM_PREALLOCHOLDERS > 0
#  define SEM_INITIALIZER(c) {(c), 0, NULL}  /* semcount, flags, hhead */
# else
#  define SEM_INITIALIZER(c) {(c), 0, SEMHOLDER_INITIALIZER} /* semcount, flags, holder */
# endif
#else
#  define SEM_INITIALIZER(c) {(c)} /* semcount */
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/* Forward references needed by some prototypes */

struct timespec; /* Defined in time.h */

/* Counting Semaphore Interfaces (based on POSIX APIs) */

int        sem_init(FAR sem_t *sem, int pshared, unsigned int value);
int        sem_destroy(FAR sem_t *sem);
int        sem_wait(FAR sem_t *sem);
int        sem_timedwait(FAR sem_t *sem, FAR const struct timespec *abstime);
int        sem_trywait(FAR sem_t *sem);
int        sem_post(FAR sem_t *sem);
int        sem_getvalue(FAR sem_t *sem, FAR int *sval);

#ifdef CONFIG_FS_NAMED_SEMAPHORES
FAR sem_t *sem_open(FAR const char *name, int oflag, ...);
int        sem_close(FAR sem_t *sem);
int        sem_unlink(FAR const char *name);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif


class semholder {
	static semholder pre_alloc[SEMHOLDERSIZE];

	list_entry<semholder> link;
	pid_t pid;
	int16_t counts;
public:
};

class sem {
	volatile int16_t semcount;
	list_head<semholder> head;
public:



};

} /* namespace xv6 */
#endif
#endif /* XV6CPP_SEM_H_ */
