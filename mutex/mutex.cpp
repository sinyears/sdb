#include "wei_mutex.h"

namespace WDB {

	int Mutex_int::init(uint __flags = M_DEFAULT) {
		flags = __flags;
		flags |= M_ALLOCATED;

		int ret;
		if (flags & M_SHARED) {
			pthread_rwlockattr_t rwlockattr;
			ret = pthread_rwlockattr_init(&rwlockattr);
			if (ret != 0)
				return ret;
			if (!(flags & M_PROCESS_ONLY))
				ret = pthread_rwlockattr_setpshared(&rwlockattr, PTHREAD_PROCESS_SHARED);
			if (ret == 0)
				ret = pthread_rwlock_init(&(u.rwlock), &rwlockattr);
			return ret;
		}

		pthread_mutexattr_t mutexattr;

		ret = pthread_mutexattr_init(&mutexattr);
		if (ret != 0) return ret;

		if (!(flags & M_PROCESS_ONLY)) {
			ret = pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
		}
		if (ret == 0)
			ret = pthread_mutex_init(&(u.m.mutex), &mutexattr);
		if (ret != 0) return ret;

		if ((flags & M_SELF_BLOCK)) {
			pthread_condattr_t condattr;

			ret = pthread_condattr_init(&condattr);
			if (ret != 0) return ret;

			if (!(flags & M_PROCESS_ONLY)) {
				ret = pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);

				if (ret == 0)
					pthread_cond_init(&(u.m.cond), &condattr);
			}
		}

		return ret;
	}

	void Mutex_int::lock() {
		if (flags & M_SHARED)
			pthread_rwlock_wrlock(&(u.rwlock));
		else 
			pthread_mutex_lock(&(u.m.mutex));
	}

	int Mutex_int::trylock() {
		if (flags & M_SHARED)
			return pthread_rwlock_trywrlock(&(u.rwlock));
		else 
			return pthread_mutex_trylock(&(u.m.mutex));
	}

	void Mutex_int::unlock() {
		if (flags & M_SHARED)
			pthread_rwlock_unlock(&(u.rwlock));
		else 
			pthread_mutex_unlock(&(u.m.mutex));
	}

	void Mutex_int::rdlock() {
		if (!(flags & M_SHARED))
			return;
		pthread_rwlock_rdlock(u.rwlock);
	}

	void Mtxpool::init_smtxpool(uint num) {
		Smtxpool *mgr = rp->primary;
		mgr = rg->alloc(sizeof(Mtxpool));
		mgr->mtx_max = num;
		mgr->mtx_num = 0;
		mgr->mtx_mgr = 0;
		void *p = rg->alloc(num * sizeof(Mutex_int));
		mgr->array_off = (uint)OFFSET_ADDR(mgr, p);
		Mutex_int *mtx = (Mutex_int*)p;
		mtx->init();
	}

	int Mtxpool::init(uint num) {
		num += 1;
		uint total_size = alloc_size(num * sizeof(Mutex_int));
		total_size += alloc_size(sizeof(Mtxpool));
		total_size += DEFAULT_PAGESIZE;
		rg = new Region(RG_MUTEX, total_size);
		init_smtxpool(num);
	}

	int Mtxpool::alloc_mutex(uint __flags = M_DEFAULT) {
		if (mtx_num == mtx_max) return -1;
		int i;
		Smtxpool *mgr = get_primary();
		(mgr->mtx_mgr).lock();
		Mutex_int *mtx_array = (Mutex_int *)R_OFFSET(mgr, mgr->array_off);

		for (i = 1; i < mtx_max; ++i) {
			mtx_array += i;
			if (!(mtx_array->get_flags() & M_ALLOCATED)) {
				mtx_array->init(__flags);
				break;
			}
			else if (!(mtx_array->get_flags() & M_LOCKED)) {
				if (mtx_array->get_flags() !=___flags)
					mtx_array->init(__flags);
				break;
			} 
		}

		if (i <= mtx_max) {
			mtx_num++;
			(mgr->mtx_mgr).unlock();
			return i;
		} else {
			(mgr->mtx_mgr).unlock();
			return -1;
		}
	}

	void Mtxpool::free_mutex(uint indx) {
		Smtxpool *mgr = get_primary();
		(mgr->mtx_mgr).lock();
		Mutex_int *mtx = ((Mutex_int *)R_OFFSET(mgr, mgr->array_off)) + indx;
		(mtx->get_flags()) &= ~M_ALLOCATED;
		(mgr->mtx_mgr).unlock();
	}
}
