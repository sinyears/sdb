#ifndef __WCOMMON_H
#define __WCOMMON_H

#include "wei_type.h"

namespace WDB {
	int os_write(Fh *&fhp, void *addr, uint len);
	int os_read(Fh *&fhp, void *addr, uint len);
	void os_ioinfo(Fh *&fhp, uint *mb, uint *byte);
	int os_seek(Fh *&fhp, uint len);
	int os_open(Fh *&fhp, const char *name, uint flags, uint modes);
	int os_map(Fh *&fhp, uint len, bool is_rdonly, void *&addrp);
	int os_unmap(void *&addrp, uint len);
	int zero_fill(Fh *&fhp);
	int file_init(Fh *&fhp, uint len, int pattern);
	int file_extend(Fh *&fhp, uint size);
	int os_close(Fh *&fhp);
	int os_fileid(const char *name, uint *p);


	inline uint align(uint v, uint bound) {
		return (v + bound - 1) & ~ (bound - 1);
	}

	uint hash_tablesize(uint nbuckets);



#define OFFSET(T, F)	(&(((T*)0)->F))
#define OFFSET_ADDR(A1, A2)	(((u1b *)A2) - ((u1b *)A1))
#define R_OFFSET(A, offset) (((u1b *)A) + (int)offset)


}


#endif
