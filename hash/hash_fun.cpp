#include "wei_hash.h"

namespace WDB {

	uint hash5(const void *key, uint len) {
		const u1b *k, *e;
		uint h;

		k = key;
		e = k + len;
		for (h = 0; k < e; ++k) {
			h *= 16777619;
			h ^= *k;
		}
		return h;
	}












}
