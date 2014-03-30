#ifndef __WTYPE_H
#define __WTYPE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include "wei_list.h"
#include "wei_atomic.h"
#include "wei_hashfun.h"

namespace WDB {
	typedef char s1b;
	typedef unsigned char u1b;
	typedef short s2b;
	typedef unsigned short u2b;
	typedef unsigned int uint;

const uint DEF_IOSIZE = (8 * 1024);
const uint MEGABYTE = 1048576;
const uint MAX_UINT32= 4294967295U;
const uint DEFAULT_PAGESIZE = (4 * 1024);
const uint MAX_FILENAME = 20;
const uint IV_BYTES = 16;
const uint MAC_KEY = 20;

	struct Fh {
		List_entry<Fh> fileq;
		uint mtx_fh;
		int fd;
		uint lastseek;
		u1b name[MAX_FILENAME];
	};

	struct Kv {
		void *data;
		uint size;
	};
}

#endif
