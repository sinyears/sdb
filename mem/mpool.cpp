#include "wei_mem.h"
#include "wei_hash.h"

namespace WDB {

	void Mpool::init_smpool(uint __rg_off) {
		Region *trg = &rg[__rg_off];	
		trg->primary = trg->alloc(sizeof(Smpool));
		Smpool *sm = (Smpool *)(trg->primary);
		sm->mtx_region = mtxp->alloc_mutex(M_SHARED);

		Mpoolhash *htp = (Mpoolhash *)(trg->alloc(MPOOL_FILE_BUCKETS * sizeof(Mpoolhash)));
		sm->ftab = OFFSET_ADDR(sm, htp);

		for (int i = 0; i < MPOOL_FILE_BUCKETS; ++i) {
			htp += i;
			htp->mtx_hash = (env->mtxp)->alloc_mutex();
			(htp->hash_bucket).init();
			htp->hash_page_dirty = 0;
		}

		htp = (Mpoolhash *)(trg->alloc(htab_buckets * sizeof(Mpoolhash)));
		sm->htab = OFFSET_ADDR(htp, sm);

		for (int i = 0; i < htab_buckets; ++i) {
			htp += i;
			htp->mtx_hash = (env->mtxp)->alloc_mutex(M_SHARED);
			(htp->hash_bucket).init();
			htp->hash_page_dirty = 0;
		}

		Bh_frozen_a  *frozen = (Bh_frozen_a *)trg->alloc(sizeof(Bh_frozen_p) + sizeof(Bh_frozen_a));
		sm->alloc_frozen.insert_tail(&(frozen->links));
		Bh *frozen_bhp = (Bh *)(frozen + 1);
		sm->free_frozen.insert_tail(&(frozen_bhp->hq));
	}

	int Mpool::init(uint __cache_size, uint __max_nreg, uint __pagesize, Mtxpool *__mtxp) : mtxp(__mtxp) {
		uint __max_size = __cache_size / __max_nreg;

		if (__pagesize == 0)
			pagesize = MPOOL_DEFAULT_SIZE;
		else
			pagesize = align(__pagesize, MPOOL_DEFAULT_SIZE);

		htab_buckets = hash_tablesize((__max_size / (3 * pagesize)));
		nbuckets = htab_buckets * __max_nreg;

		reg_size = sizeof(Smpool);
		reg_size += MPOOL_FILE_BUCKETS * sizeof(Mpoolhash);
		reg_size += htab_buckets * sizeof(Mpoolhash);
		reg_size += pagesize * 10;

		if (reg_size > __max_size) { return EINVAL;}

		reg_size = __max_size;
		max_nreg = __max_nreg;
		mutex = mtxp->alloc_mutex(M_SHARED);
		mutex_resize = mtxp->alloc_mutex(M_SHARED);

		Region *trg = new Region(RG_MPOOL, __max_size);
		rg = new Region[max_nreg];

		if (rg->init(RG_MPOOL, __max_size) == 0)
			init_smpool(0);
	}

	void *Mpool::alloc(uint rg_indx, uint len) {
		Region *trg = &rg[rg_indx];
		return trg->alloc(len);
	}

	void *Mpool::alloc_page(uint rg_indx) {
		Smpool *smp = get_smpool(rg_indx);
		uint len = sizeof(Bh) + pagesize;
		Region *trg = &rg[rg_indx];
		void *result = trg->alloc(len);
		Bh *bhp;
		if (result) {
			bhp = (Bh *)result;
			bhp->mtx_buf = mtxp->alloc_mutex(M_SHARED);
		} else {
			uint priority = MAX_UINT32;

			for (int loop = 0; loop < htab_buckets; i++) {
				Mpoolhash *htb = (Mpoolhash *)(smp->htab + (u1b *)(smp));
				Mpoolhash *hp = htb[smp->last_checked++];

				if (smp->last_checked > htab_buckets)
					smp->last_checked = 0;

				if ((hp->hash_bucket).is_empty())
					continue;

				Shlist_entry *entry;
				mtxp->rdlock(hp->mtx_hash);
				for (entry = (hp->hash_bucket).get_first(); entry != NULL, entry = entry->get_next()) {
					Bh *tbh = (Bh *)e2bh(entry);
					mtxp->rdlock(tbh->mtx_buf);

					if (tbh->ref != 0)
						continue;

					if (priority > tbh->priority) {
						priority = tbh->priority;
						bhp = tbh;
					}

					mtxp->unlock(tbh->mtx_buf);
				}
			}
		}

		if (bhp)
			return (void *)bhp;

		return NULL;
	}

	int Mpool::merge_buckets(uint new_nbuckets, uint old_bucket, uint new_bucket) {
		uint old_region = nregion(htab_buckets, old_bucket);
		Region *org = get_region(old_region);
		Smpool *osm = get_smpool(old_region);
		Mpoolhash *old_hp = (Mpoolhash *)R_OFFSET(org, osm->htab);
		old_hp = &old_hp[old_bucket - old_region * htab_buckets];

		uint new_region = nregion(htab_buckets, new_bucket);
		Region *nrg = get_region(new_region);
		Smpool *nsm = get_smpool(new_region);
		Mpoolhash *new_hp = (Mpoolhash *)R_OFFSET(nrg, nsm->htab);
		new_hp = &new_hp[new_bucket - new_region * htab_buckets];

		Shlist_entry *entry;
		Bh *bh;
		uint bucket;
		Smpoolfile *smf;

		mtx->lock(old_hp->mtx_hash);
		for (entry = (old_hp->hash_bucket).get_first(); entry->next != 0; entry = entry->get_next()) {
			bh = e2bh(entry);
			mtxp->rdlock(bh->mtx_buf);
			bucket = mp_bucket(bh->mf_offset, bh->pgno, new_nbuckets);

			if (bucket != new_bucket) {
				mtxp->unlock(bh->mtx_buf);
				continue;
			}

			mtxp->unlock(bh->mtx_buf);
			mtxp->lock(new_hp->mtx_hash);
			(bh->hq).remove();
			new_hp->hash_bucket.insert_tail(&(bh->hq));
			mtxp->unlock(new_hp->mtx_hash);
		}
		mtx->unlock(old_hp->mtx_hash);
	}

	int Mpool::add_bucket() {
		uint new_bucket = nbuckets;
		uint new_mask = mp_mask(new_buckets);
		uint old_bucket = new_bucket & (new_mask >> 1);
		return merge_bucket(nbuckets + 1, old_bucket, new_bucket);
	}

	void *Mpool::get_htb(uint __mf_offset, uint __pgno) {
		uint __bucket = mp_bucket(__mf_offset, __pgno, nbuckets);
		uint __region = nregion(htab_buckets, __bucket);
		Smpool *__sm = get_smpool(__region);
		Mpoolhash *ht = R_OFFSET(__sm, __sm->htab);
		return &ht[__bucket - __region * htab_buckets];
	}

	void *Mpool::get_htb(uint __rg_indx) {
		Smpool *__smp = get_smpool(__rg_indx);
		void *__hb = (void *)R_OFFSET(__smp, __smp->htab);
		return __hb;
	}

	void *Mpool::get_ftb(u1b *__fileid) {
		uint __bucket = hash5(__fileid, FILEID_LEN) % MPOOL_FILE_BUCKETS;
		Smpool *__sm = get_smpool(0);
		Mpoolhash *fht = (Mpoolhash *)R_OFFSET(__sm, __sm->ftab);
		return fht;
	}

	int Mpool::add_region() {
		if (rg[nreg].init(RG_MPOOL, reg_size) != 0)
			return -1;

		init_smpool(nreg);
		mtxp->lock(mutex);
		nreg++;
		mtxp->unlock(mutex);

		for (int i = 0; i < htab_buckets; i++) {
			if (add_bucket() != 0)
				return -1;
		}

		return 0;
	}

	int Mpool::remove_bucket() {
		uint old_bucket = nbuckets - 1;
		uint new_mask = mp_mask(old_bucket);
		uint new_bucket = old_bucket & (new_mask >> 1);
		return merge_bucket(nbuckets -1, old_bucket, new_bucket);
	}

	int Mpool::remove_region() {
		if (nreg == 1)
			return EINVAL;

		for (int i = 0; i < htab_buckets; ++i) {
			if (remove_bucket() != 0)
				return -1;
		}

		mtxp->lock(mutex);
		nreg--;
		mtxp->unlock(mutex);

		Smpool *sm = get_smpool(nreg);
		Mpoolhash *htb = (Mpoolhash *)R_OFFSET(sm, sm->htab);
		for (int i = 0; i < htab_buckets; ++i) {
			mtxp->free_mutex((htb + i)->mtx_hash);
		}
		rg[nreg].free_all();
		memset(&rg[nreg], 0, sizeof(Region));
		return 0;
	}

	int Mpool::resize(uint total_size) {
		uint __nreg = (total_size + reg_size / 2) / reg_size;
		int ret;

		if (__nreg < 1)
			__nreg = 1;
		else if (__nreg > max_nreg)
			return EINVAL;

		mtxp->lock(mtx_resize);

		while (nreg != __nreg) {
			if (__nreg < nreg) {
				add_region();
			}
			else if (__nreg > nreg) 
				remove_region();
		}
		mtxp->unlock(mtx_resize);
	}

	int Mpool::set_lru(uint __rg_indx) {
		Smpool *__smp = get_smpool(rg_indx);
		Mpoolhash *__htb = get_htb(__rg_indx);
		rg[__rg_indx].lock();
		if (__smp->lru_priority >= MPOOL_LRU_DECREMENT) {
			__smp->lru_priority -= MPOOL_LRU_DECREMENT;
			__smp->lru_generation++;
		}
		rg[__rg_indx].unlock();

		uint __bucket = 0;
		Shlist_entry *entry = NULL;
		for (; __bucket < htab_buckets; ++_bucket, ++__htb) {
			for (entry = __htb->get_first(); entry != NULL; entry = entry->get_next()) {
				mtxp->lock(__htb->mtx_hash);
				Bh *__bhp = e2bh(entry);
				if (__bhp->priority > MPOOL_LRU_DECREMENT)
					__bhp->priority -= MPOOL_LRU_DECREMENT;
				mtxp->unlock(__htb->mtx_hash);
			}
		}
		return 0;
	}

	int Mpoolfile::init_smpoolfile(int __ftype) {
		smf->ftype = __ftype;
		smf->maxpgno = max_size / mp->pagesize;
		smf->mutex = mtxp->alloc_mutex(M_SHARED);
		smf->bucket = hash5(fileid, FILEID_LEN) % MPOOL_FILE_BUCKETS; 
		Smpool *smp = mp->get_smpool(0);
		Mpoolhash *hf = (Mpoolhash *)R_OFFSET(smp, smp->ftab);
		hf += smf->bucket;
		(hf->hash_bucket).insert_tail(&smf->shmfq);
		return 0;
	}

	int Mpoolfile::init(const char *__name, uint __size, Mtxpool *__mtxp, Mpool *__mp, uint __flags, uint __modes, int __ftype) : mtxp(__mtxp), mp(__mp) {
		max_size = align(__size, mp->pagesize);
		int ret = os_open(fhp, __name, __flags, __modes);
		os_fileid(__name, fileid);
		smf = smpf_find(0);

		if (!smf) {
			smf = (mp->rg)->alloc(sizeof(Smpoolfile));
			init_smpoolfile(__ftype);
		}
	}

	Smpoolfile *Mpoolfile::smpf_find(uint indx) {
		Smpool *smp = mp->get_smpool(indx);	
		Mpoolhash *h = R_OFFSET(smp, smp->ftab);
		uint bucket = hash5(fileid, FILEID_LEN) % MPOOL_FILE_BUCKETS;
		h += bucket;
		Shlist_entry *entry;

		for (entry = (h->hash_bucket).get_first(); entry != NULL; entry = entry->get_next()) {
			Smpoolfile *smf = e2mh(entry);
			if (!memcmp(smf->fileid, fileid, FILEID_LEN))
				return smf;
		}
		return NULL;
	}

	int Mpoolfile::page_read(Bh *bhp) {
		mtxp->lock(fhp->mtx_fh);
		int ret = os_seek(fhp, bhp->pgno * mp->pagesize);
		ret = os_read(fhp, bhp->buf, mp->pagesize);
		mtxp->unlock(fhp->mtx_fh);
		return ret;
	}

	Bh *Mpoolfile::get_page(uint __pgno) {
		uint __mf_offset = OFFSET_ADDR(smp, smf);
		uint __bucket = mp_bucket(__mf_offset, __pgno, mp->nbuckets);
		Mpoolhash *hb = mp->get_smpool(0);
		mtxp->rdlock(hb->mtx_hash);
		Shlist_entry *entry;
		Bh *bhp = NULL;
		for (entry = (hb->hash_bucket).get_first(); entry != NULL; entry = entry->get_next()) {
			Bh *tbhp = e2bh(entry);
			if ((tbhp->__pgno == __pgno) && (tbhp->__mf_offset == __mf_offset)){
				bhp = tbhp;
				break;
			}
		}
		mtxp->unlock(hb->mtx_hash);

		if (bhp)
			return bhp;

		mtxp->rdlock(mp->mtx_resize);
		for (int i = 0; i < mp->nreg; i++) {
			bhp = (Bh *)mp->alloc_page(i);
			if (bhp)
				break;
		}
		mtxp->unlock(mp->mtx_resize);

		if (bhp == NULL)
			return NULL;

		atomic_inc(&(bhp->ref));
		bhp->mf_offset = __mf_offset;
		bhp->pgno = __pgno;
		bhp->bucket = __bucket;
		return bhp;
	}


	int Mpoolfile::put_page(Bh *__bhp, int __priority) {
		Smpool *__smp = mp->get_smpool(__bhp->region);
		int pfactor;
		int adjust = 0;
		if (__priority == MPOOL_PRI_VERY_LOW || smf->priority == MPOOL_PRI_VERY_LOW)	
			__bhp->priority = 0;
		else {
			__bhp->priority = __smp->lru_priority;
			switch (__priority) {
				default:
				case C_PRIORITY_UNCHANGED:
					pfactor = smf->priority;
					break;
				case C_PRIORITY_VERY_LOW:
					pfactor = MPOOL_PRI_VERY_LOW;
					break;
				case C_PRIORITY_LOW:
					pfactor = MPOOL_PRI_LOW;
					break;
				case C_PRIORITY_DEFAULT:
					pfactor = MPOOL_PRI_DEFAULT;
					break;
				case C_PRIORITY_HIGH:
					pfactor = MPOOL_PRI_HIGH;
					break;
				case C_PRIORITY_VERY_HIGH:
					pfactor = MPOOL_PRI_VERY_HIGH;
					break;
			}
			if (pfactor != 0) 
				adjust = __smp->pages / pfactor;

			if (__bhp->flags & DIRTY)
				adjust += __smp->pages / MPOOL_PRI_DIRTY;

			if ((uint)adjust + __bhp->priority <= MPOOL_LRU_REDZONE)
				__bhp->priority += adjust;

		}

		__smp->lru_priority++;
		mp->set_lru(__bhp->region);
	}
}
