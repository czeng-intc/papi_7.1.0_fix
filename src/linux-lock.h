#ifndef _LINUX_LOCK_H
#define _LINUX_LOCK_H

/* Locking functions */

extern volatile unsigned int _papi_hwd_lock_data[PAPI_MAX_LOCK];
#define MUTEX_OPEN 0
#define MUTEX_CLOSED 1

/********/
/* ia64 */
/********/

#if defined(__ia64__)
#ifdef __INTEL_COMPILER
#define _papi_hwd_lock(lck) { while(_InterlockedCompareExchange_acq(&_papi_hwd_lock_data[lck],MUTEX_CLOSED,MUTEX_OPEN) != MUTEX_OPEN) { ; } }
#define _papi_hwd_unlock(lck) { _InterlockedExchange((volatile int *)&_papi_hwd_lock_data[lck], MUTEX_OPEN); }
#else  /* GCC */
#define _papi_hwd_lock(lck)			 			      \
   { int res = 0;							      \
    do {								      \
      __asm__ __volatile__ ("mov ar.ccv=%0;;" :: "r"(MUTEX_OPEN));            \
      __asm__ __volatile__ ("cmpxchg4.acq %0=[%1],%2,ar.ccv" : "=r"(res) : "r"(&_papi_hwd_lock_data[lck]), "r"(MUTEX_CLOSED) : "memory");				      \
    } while (res != MUTEX_OPEN); }

#define _papi_hwd_unlock(lck) {  __asm__ __volatile__ ("st4.rel [%0]=%1" : : "r"(&_papi_hwd_lock_data[lck]), "r"(MUTEX_OPEN) : "memory"); }
#endif

/***********/
/* x86     */
/***********/

#elif defined(__i386__)||defined(__x86_64__)
#define  _papi_hwd_lock(lck)                    \
do                                              \
{                                               \
   unsigned int res = 0;                        \
   do {                                         \
      __asm__ __volatile__ ("lock ; " "cmpxchg %1,%2" : "=a"(res) : "q"(MUTEX_CLOSED), "m"(_papi_hwd_lock_data[lck]), "0"(MUTEX_OPEN) : "memory");  \
   } while(res != (unsigned int)MUTEX_OPEN);   \
} while(0)
#define  _papi_hwd_unlock(lck)                  \
do                                              \
{                                               \
   unsigned int res = 0;                       \
   __asm__ __volatile__ ("xchg %0,%1" : "=r"(res) : "m"(_papi_hwd_lock_data[lck]), "0"(MUTEX_OPEN) : "memory");                                \
} while(0)

/***************/
/* power       */
/***************/

#elif defined(__powerpc__)

/*
 * These functions are slight modifications of the functions in
 * /usr/include/asm-ppc/system.h.
 *
 *  We can't use the ones in system.h directly because they are defined
 *  only when __KERNEL__ is defined.
 */

static __inline__ unsigned long
papi_xchg_u32( volatile void *p, unsigned long val )
{
	unsigned long prev;

	__asm__ __volatile__( "\n\
        sync \n\
1:      lwarx   %0,0,%2 \n\
        stwcx.  %3,0,%2 \n\
        bne-    1b \n\
        isync":"=&r"( prev ), "=m"( *( volatile unsigned long * ) p )
						  :"r"( p ), "r"( val ),
						  "m"( *( volatile unsigned long * ) p )
						  :"cc", "memory" );

	return prev;
}

#define  _papi_hwd_lock(lck)                          \
do {                                                    \
  unsigned int retval;                                 \
  do {                                                  \
  retval = papi_xchg_u32(&_papi_hwd_lock_data[lck],MUTEX_CLOSED);  \
  } while(retval != (unsigned int)MUTEX_OPEN);	        \
} while(0)
#define  _papi_hwd_unlock(lck)                          \
do {                                                    \
  unsigned int retval;                                 \
  retval = papi_xchg_u32(&_papi_hwd_lock_data[lck],MUTEX_OPEN); \
} while(0)

/*****************/
/* SPARC         */
/*****************/

#elif defined(__sparc__)
static inline void
__raw_spin_lock( volatile unsigned int *lock )
{
	__asm__ __volatile__( "\n1:\n\t" "ldstub	[%0], %%g2\n\t" "orcc	%%g2, 0x0, %%g0\n\t" "bne,a	2f\n\t" " ldub	[%0], %%g2\n\t" ".subsection	2\n" "2:\n\t" "orcc	%%g2, 0x0, %%g0\n\t" "bne,a	2b\n\t" " ldub	[%0], %%g2\n\t" "b,a	1b\n\t" ".previous\n":	/* no outputs */
						  :"r"( lock )
						  :"g2", "memory", "cc" );
}
static inline void
__raw_spin_unlock( volatile unsigned int *lock )
{
	__asm__ __volatile__( "stb %%g0, [%0]"::"r"( lock ):"memory" );
}

#define  _papi_hwd_lock(lck) __raw_spin_lock(&_papi_hwd_lock_data[lck]);
#define  _papi_hwd_unlock(lck) __raw_spin_unlock(&_papi_hwd_lock_data[lck])

/*******************/
/* ARM             */
/*******************/

#elif defined(__arm__)

/* FIXME */
/* not sure if this even works            */
/* also the various flavors of ARM        */
/* have differing levels of atomic        */
/* instruction support.  A proper         */
/* implementation needs to handle this :( */

#warning "WARNING!  Verify mutexes work on ARM!"

/*
 * For arm/gcc, 0 is clear, 1 is set.
 */
#define MUTEX_SET(tsl) ({      \
  int __r;                     \
  asm volatile(                \
  "swpb   %0, %1, [%2]\n\t"    \
  "eor    %0, %0, #1\n\t"      \
  : "=&r" (__r)                \
  : "r" (1), "r" (tsl)         \
  );                           \
  __r & 1;                     \
    })

#define  _papi_hwd_lock(lck) MUTEX_SET(lck)
#define  _papi_hwd_unlock(lck) (*(volatile int *)(lck) = 0)

#else

#error "_papi_hwd_lock/unlock undefined!"
#endif

#endif