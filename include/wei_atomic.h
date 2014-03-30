#ifndef _WATOMIC_H_
#define _WATOMIC_H_

#if defined(__cplusplus)
extern "C"{
#endif

#define atomic_read(p) ((p)->value)
#define atomic_init(p,val) ((p)->value = (val))

typedef struct {
	volatile int value;
} atomic_t;

static inline void __atomic_inc(atomic_t *p)
{
	__asm__ __volatile__("lock; xadd %0,(%1)" :: "r"(1), "r"(p));
}

static inline void __atomic_dec(atomic_t *p)
{
	__asm__ __volatile__("lock; xadd %0,(%1)" :: "r"(-1), "r"(p));
}

static inline void __atomic_compare_exchange(atomic_t *p, int newval)
{
	__asm__ __volatile__("lock; cmpxchgl %0, (%1)" :: "r"(newval), "r"(p) ,"a"(p->value) : "memory", "cc");

}

#define atomic_inc(p) __atomic_inc(p)
#define atomic_dec(p) __atomic_dec(p)


#if defined(__cplusplus)
}
#endif

#endif
