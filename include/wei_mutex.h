#ifndef __WMUTEX_H
#define __WMUTEX_H

#include "pthread.h"
#include "wei_type.h"

namespace WDB {
	enum MTXTYPE { M_DEFAULT = 0, M_ALLOCATED = 1, M_LOCKED = 2, M_LOGICAL_LOCK = 4, M_PROCESS_ONLY = 8, M_SELF_BLOCK = 16, M_SHARED = 32};

	class Mutex_int {
		union {
			struct {
				pthread_mutex_t mutex;
				pthread_cond_t cond;
			} m;
			pthread_rwlock_t rwlock;
		} u;
		uint flags;

		public:
		Mutex_int(uint __flags= M_DEFAULT) { init(__flags); }

		int init(uint __flags = M_DEFAULT);
		void lock();
		void unlock();
		int trylock();
		void rdlock();
		uint &get_flags() { return flags;}
	};

	struct Smtxpool {
		uint mtx_mgr;
		uint mtx_num;
		uint mtx_max;
		uint array_off;
	}

	class Mtxpool {
		Region *rg;
		void *init_smtxpool(uint num);
		public:
		Mutex_region(uint num);
		int init(uint num);
		int alloc_mutex(uint __flags = M_DEFAULT);
		void free_mutex(uint indx);

		Smtxpool *get_smtxpool() { return (Smtxpool *)(rg->primary);}

		Mutex_int *get_mtx(uint indx) {
			Smtxpool *mgr = get_smtxpool();
			return ((Mutex_int *)(R_OFFSET(mgr, mgr->array_off) + indx));
		}

		void lock(uint indx) {
			get_mtx(indx)->lock();
		}

		void unlock(uint indx) {
			get_mtx(indx)->unlock();
		}

		void rdlock(uint indx) {
			get_mtx(indx)->rdlock();
		}
	}
}
#endif
