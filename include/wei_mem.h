#ifndef __WMEM_H
#define __WMEM_H

#include <iostream>
#include "wei_type.h"
#include "wei_common.h"

namespace WDB {

	struct Element {
		Shlist_entry addrq;
		Shlist_entry sizeq;
		uint len;
		uint ulen;
	};

	static const int SIZE_Q_COUNT = 11;
	static const int SHALLOC_FRAGMENT = (sizeof(Element) + 64);
	static const int FILEID_LEN = MAX_FILENAME;
	static const int MPOOL_DEFAULT_SIZE = (4 * 1024);
	static const int MPOOL_FILE_BUCKETS = 17;
	static uint alloc_size(uint size) { return align(size + sizeof(Element), sizeof(uint));}

	static inline Element *ae2e(Shlist_entry *entry) {
		uint offset = (uint)OFFSET(Element, addrq);
		return (Element *)((u1b *)entry - offset);
	}

	static inline Element *se2e(Shlist_entry *entry) {
		uint offset = (uint)OFFSET(Element, sizeq); 
		return (Element *)((u1b *)entry - offset);
	}

	static inline Bh *e2bh(Shlist_entry *entry) {
		uint offset = (uint)OFFSET(Bh, hq);
		return (Bh *)((u1b *)entry - offset);
	}

	static inline Smpoolfile *e2mh(Shlist_entry *entry) {
		uint offset = (uint) OFFSET(Smpoolfile, shmfq);
		return (Smpoolfile *)((u1b *)entry - offset);
	}

	static inline int alloc_size(uint len) {
		return (len + sizeof(Element));
	}

	struct Layout {
		Shlist_head addrq;
		Shlist_head sizeq[SIZE_Q_COUNT];
		void insert_sizeq(Element *elp);
		void insert_addrq(Element *elp);
	};

	static inline int get_queue(uint size) {
		int i;
		for (i = 0; i < SIZE_Q_COUNT; i++) {
			if (size < ((uint)1024 << i))
				break;
		}
		return i;
	}

	enum REGTYPE {RG_INVALID = 0, RG_ENV, RG_LOCK, RG_LOG, RG_MPOOL, RG_MUTEX, RG_TXN};

	class Region {
		REGTYPE type;	
		Mutex_int mtx_rg;
		uint id;
		Fh *fhp;
		void *addr;
		void *head;

		uint max_alloc;
		uint allocated;

		static uint rgnum;
		void init_head(uint size);
		public:
		void *primary;
		Region(REGTYPE __rt, uint size);
		Region();
		void lock() { mtx_rg.lock(); }
		void unlock() {mtx_rg.unlock(); }
		int trylock() { return mtx_rg.trylock(); }
		void rdlock() { mtx_rg.rdlock(); }
		void init(REGTYPE __rt, uint size);
		void *alloc(uint size);
		void free(void *p);
		void free_all();
		~Region();
	};

	struct Mpoolhash {
		uint mtx_hash;
		Shlist_head hash_bucket;
		atomic_t hash_page_dirty;
		uint flags;
	};

	
	struct Smpool {
		uint mtx_region;

		uint gbytes;
		uint bytes;
		uint maxopenfd;
		uint maxwrite;
		uint mmapsize;

		uint ftab;
		uint htab;
		uint lru_priority;
		uint last_checked;
		uint lru_generation;

		uint pages;//number of pages in cache;

		Shlist_head free_frozen;
		Shlist_head alloc_frozen;
	};

	enum MPOOL_LRU{ MPOOL_LRU_MAX = MAX_UINT32, MPOOL_LRU_REDZONE = (MPOOL_LRU_MAX - 128), MPOOL_LRU_BASE = (MPOOL_LRU_MAX / 4), MPOOL_LRU_DECREMENT = (MPOOL_LRU_MAX - MPOOL_LRU_BASE)};

	enum MPOOL_PRI { MPOOL_PRI_VERY_LOW = -1, MPOOL_PRI_LOW = -2, MPOOL_PRI_DEFAULT = 0, MPOOL_PRI_HIGH = 10, MPOOL_PRI_DIRTY = 10, MPOOL_PRI_VERY_HIGH = 1};

	enum CACHE_PRI {
		C_PRIORITY_UNCHANGED=0,
		C_PRIORITY_VERY_LOW=1,
		C_PRIORITY_LOW=2,
		C_PRIORITY_DEFAULT=3,
		C_PRIORITY_HIGH=4,
		C_PRIORITY_VERY_HIGH=5
	};

	class Mpool {
		Mtxpool *mtxp;
		uint mutex;
		uint mtx_resize;
		uint max_nreg;
		uint nreg;
		uint htab_buckets;
		uint nbuckets;
		uint pagesize;
		uint htab_mutexes;
		uint reg_size;

		List_head<Mpoolfile> mfq;
		Region *rg;
		int merge_bucket(uint new_nbuckets, uint old_buckets, uint new_bucket);
		int init(uint __cache_size, uint __nreg, uint __pagesize, Env *__env);
		int add_region();
		int add_bucket();
		int remove_region();
		int remove_bucket();
		void init_smpool(uint __rg_off);

		public:
		Mpool(uint __cache_size, uint __nreg, uint __pagesize, Mtxpool *__mtxp) { init(__cache_size, __nreg, __pagesize, __env);}

		int set_lru(uint rg_indx);

		Smpool *get_smpool(uint sm_indx) { 
			if (rg)
				return (Smpool *)(rg[sm_indx].primary);
			return NULL;
		}

		Region *get_region(int indx) { return &rg[indx];}

		int resize(uint total_size);

		void *alloc(uint rg_indx, uint len);

		void *alloc_page(uint rg_indx);

		void *get_htb(uint pgno, uint mf_offset);

		void *get_htb(uint __rg_indx);
	};

	enum BHFLAG { CALLPGIN = 1, DIRTY = 2, DIRTY_CREATE = 4, DISCARD = 8, EXCLUSIVE = 16, FREED = 32, FROZEN = 64, TRASH = 128, THAWED = 256};

	struct Bh {
		uint mtx_buf;
		atomic_t ref;

		u2b flags;
		uint priority;

		Shlist_entry hq;

		uint pgno;
		uint mf_offset;
		uint bucket;
		int region;

		u1b buf[0];
	};

	struct Bh_frozen_p {
		Bh header;
		uint spgno;
	};

	struct Bh_frozen_a {
		Shlist_entry links;
	};

	static inline int nregion(uint __htab_buckets, uint __bucket) {
		return (__bucket / __htab_buckets);
	}

	static inline uint mp_hash(uint __mf_offset, uint __pgno) {
		return ((__pgno << 8) ^ __pgno) ^ (__mf_offset * 509);
	}

	static inline uint mp_mask(uint __nbuckets) {
		for (uint __mask = 1; __mask < __nbuckets; __mask = (__mask << 1) | 1);
		return __mask;
	}

	static inline uint mp_hash_bucket(uint __hash, uint __nbuckets, uint __mask) {
		uint __bucket = (__hash & __mask);
		if (__bucket >= __nbuckets)
			__bucket &= (__mask >> 1);
		return __bucket;
	}

	static inline uint mp_bucket(uint __mf_offset, uint __pgno, uint __nbuckets) {
		uint __hash = mp_hash(__mf_offset, __pgno);
		uint __mask = mp_mask(__nbuckets);
		return mp_hash_bucket(__hash, __nbuckets, __mask);
	}

	enum {MF_CREATE = 0x1, MF_LAST = 0x10, MF_NEW = 0x20}

	struct Smpoolfile {
		uint mutex;

		uint backup_in_progress;
		pid_t pid;
		uint tid;
		atomic_t writers;
		uint mtx_write;
		uint low_pgno, high_pgno;

		uint block_cnt;
		uint last_pgno;
		uint last_flushed_pgno;
		uint orig_last_pgno;
		uint maxpgno;

		Shlist_entry shmfq;


		uint bucket;
		u1b fileid[FILEID_LEN];
		int deadfile;
		int ftype;
		int priority;//unpinning buffer

		int file_written;
		int no_backing_file;
		int unlink_on_close;
	};

	class Mpoolfile {
		Fh *fhp;
		Mtxpool *mtxp;
		Mpool *mp;
		Smpoolfile *smf;
		uint ref;
		uint pinref;

		List_entry<Mpoolfile> mfq;

		u1b fileid[FILEID_LEN];
		uint max_size;

		void *addr;//mmap region;
		uint len;//length of mmap'd region;
		uint flags;

		Smpoolfile * smpf_find();
		int init_smpoolfile();
		int page_read(Bh *bhp);
		public:
		Mpoolfile(const char *__name, uint __size, Mtxpool *__mtxp, Mpool *__mp, uint __flags, uint __modes) { init(__name, __size, __mp, __flags, __modes);}

		int init(const char *__name, uint __size, Mtxpool *__mtxp, Mpool *__mp, uint __flags, uint __modes);

		Bh *get_page(uint __pgno);

		int put_page(Bh *__bhp, int __priority);

	}



}
#endif
