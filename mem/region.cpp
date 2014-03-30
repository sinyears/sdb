#include "wei_mem.h"

namespace WDB {

	void Layout::insert_sizeq(Element *elp) {
		Shlist_entry *entry = &elp->sizeq;
		int i = get_queue(elp->len);
		Shlist_entry *tmp;
		Shlist_head *q = &sizeq[i];

		if (!q->get_first()) {
			q->insert_head(entry);
			return;
		}

		for(tmp = q->get_first(); tmp != NULL; tmp = tmp->get_next()){
			Element *eltmp = se2e(entry);
		if (elp->len >= eltmp->len) {
			tmp->insert_before(entry);
			return;
		}
		}
		q->insert_tail(entry);
	}

	void Layout::insert_addrq(Element *elp) {
		Shlist_entry *entry = &elp->addrq;
		addrq.insert_tail(entry);
	}

	uint Region::rgnum = 0;

	Region::Region() : type(RG_INVALID), id(0), fhp(NULL), addr(NULL), head(NULL), primary(NULL), max_alloc(0), allocated(0), mtx_alloc(0) {
	}

	int Region::init(REGTYPE __rt, uint size) {
		type = __rt;
		max_alloc = size;
		rgnum++;
		id = rgnum;
		char buf[32] = {0};

		snprintf(buf, sizeof(buf), "%d_%d", rgnum, type);
		int ret = os_open(fhp, buf, O_CREAT | O_RDWR , S_IRUSR | S_IWUSR);
		ret = file_init(fhp, size, 0x00);
		ret = os_map(fhp, size, false, addr);
		primary = head = addr;

		if (ret == 0) 
			init_head(size);
		ret = mtx_rg.init(M_SHARED);

		return ret;
	}

	Region::Region(REGTYPE __rt, uint size) {
		init(__rt, size);
	}

	void Region::init_head(uint size) {
		Layout *head = (Layout *)this->head;	
		memset(head, 0, sizeof(head));
		Element *elp = (Element *)(head + 1);
		elp->len = size - sizeof(Layout);
		elp->ulen = 0;

		head->addrq.insert_head(&(elp->addrq));
		(head->sizeq[SIZE_Q_COUNT - 1]).insert_head(&(elp->sizeq));
	}

	void* Region::alloc(uint size) {
		mtx_rg.lock();
		Layout *head = (Layout *)this->head;
		uint total_len = alloc_size(size);
		int i = get_queue(total_len);
		Shlist_head *q;
		Element *elp = NULL, *elptmp;
		Shlist_entry *lp;

		while (i < SIZE_Q_COUNT) {
			q = &(head->sizeq[i]);
			if (q->is_empty()) {
				i++;
				continue;
			}

			for (lp = q->get_first(); lp != NULL; lp = lp->get_next()) {
				elptmp = se2e(lp);
				if (elptmp->len < total_len)
					break;
				elp = elptmp;
				if (elptmp->len - total_len <= SHALLOC_FRAGMENT)
					break;
			}

			i++;
			if (elp != NULL )
				break;
		}

		if (elp == NULL)
			return NULL;

		shremove(q, &elp->sizeq);

		Element *frag;
		if (elp->len - total_len > SHALLOC_FRAGMENT) {
			frag = (Element *)((u1b *)elp + total_len);
			frag->len = elp->len - total_len;
			frag->ulen = 0;
			elp->len = total_len;
			head->insert_addrq(frag);
			head->insert_sizeq(frag);
		}
		elp->ulen = elp->len;
		mtx_rg.unlock();
		return (void *)((u1b *)elp + sizeof(Element));
	}

	void Region::free(void *p) {
		Element *elp = (Element *)((u1b *)p - sizeof(Element));
		memset(p, 0, elp->len - sizeof(Element));
		Shlist_entry *scur_entry = &(elp->sizeq);
		Shlist_entry *entry = scur_entry->get_pre();

		if (entry != NULL) {
			Element *elp_tmp = se2e(entry);

			if (elp_tmp ->ulen == 0 && ((u1b *)elp_tmp + elp_tmp->len == (u1b *)elp)) {
				(elp->addrq).remove();
				(elp_tmp->sizeq).remove();
				elp_tmp->len += elp->len;
				elp = elp_tmp;
			}
		}

		entry = scur_entry->get_next();

		if (entry != NULL) {
			Element *elp_tmp = se2e(entry);

			if (elp_tmp ->ulen == 0 && ((u1b *)elp + elp->len == (u1b *)elp_tmp)) {
				(elp_tmp->addrq).remove();
				(elp_tmp->sizeq).remove();
				elp->len += elp_tmp->len;
			}
		}
		insert_sizeq(elp);
	}

	Region::free_all() {
		os_unmap(addr, max_alloc);
	}

	Region::~Region() {
		os_close(fhp);
		delete fhp;
	}
}
