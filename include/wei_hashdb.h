#ifndef __WHASHDB_H
#define __WHASHDB_H

#include "wei_type.h"
#include "wei_mem.h"

namespace WDB {
	const int BUCKET_INVALID = 0xFFFFFFFF;

	struct Hashmeta {
#define	HASH_DUP	0x01	/*	  Duplicates. */
#define	HASH_SUBDB	0x02	/*	  Subdatabases. */
#define	HASH_DUPSORT	0x04	/*	  Duplicates are sorted. */
		uint pgno;		/* 08-11: Current page number. */
		uint encrypt_alg;	/*    24: Encryption algorithm. */
		uint last_pgno;	/* 32-35: Page number of last page in db. */
		uint nparts;	/* 36-39: Number of partitions. */
		u1b uid[MAX_FILENAME];

		uint max_bucket;	/* 72-75: ID of Maximum bucket in use */
		uint high_mask;	/* 76-79: Modulo mask into table */
		uint low_mask;	/* 80-83: Modulo mask into table lower half */
		uint ffactor;	/* 84-87: Fill factor */
		uint nelem;	/* 88-91: Number of keys in hash table */
		uint h_charkey;	/* 92-95: Value of hash(CHARKEY) */
#define	NCACHED	32		/* number of spare points */
		uint spares[NCACHED];	/* 96-223: Spare pages for overflow */
		uint crypto_magic;	/* 460-463: Crypto magic number */
		u1b iv[IV_BYTES];	/* 476-495: Crypto IV */
		u1b chksum[MAC_KEY];	/* 496-511: Page chksum */
	};

	struct Hashpage {
		uint pgno;
		uint prev_pgno;
		uint next_pgno;
		uint entries;
		uint hf_offset;
	};

	struct Hashcursor {
		uint mutex;
		Hashmeta *hm;
		Hashpage *page;
		Hashpage *split_buf;
		uint bucket;
		uint lbucket;
		uint dup_off;
		uint dup_len;
		uint dup_tlen;
		uint seek_size;
		uint seek_found_page;
		uint order;
	};

	class Hashdb {
		uint pgsize;		/* Database logical page size. */
		uint mutex;
		CACHE_PRI priority;	/* Database priority in cache. */
		Hashcursor *hc;

		Mpoolfile *mpf;
		Mtxpool *mtx;


		char *name;
		uint open_flags;
		public:
		Hashdb();
		int hc_init();
	};
}

#endif
