#include "wei_hashdb.h"

namespace WDB {
	Hashdb::hc_init() {
		hc->bucket = BUCKET_INVALID;
		hc->lbucket = BUCKET_INVALID;
		hc->mutex = -1;
		return 0;
	}

	Hashdb::Hashdb(char *__name, Mtxpool *__mtx, Mpoolfile *__mpf, uint __open_flags, __priority, uint __pgsize) : pgsize(__pgsize), priority(__priority), mpf(__mpf), mtx(__mtx) {
		hc = new Hashcursor;
		hc->split_buf = (Hashpage *)malloc(pgsize);
		hc_init();
	}
	

}
