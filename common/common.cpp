#include "wei_common.h"

namespace WDB {
	const static uint LARGE_WRITE = (64 * 1024);

	int os_open(Fh *&fhp, const char *name, uint flags, uint modes) {
		fhp = new Fh;
		int len = strlen(name);
		strncpy(fhp->name, name, len);
		fhp->name[len] = '\0';
		fhp->fd = open(name, flags, modes);

		if (fhp->fd != -1)
			return 0;
		return -1;
	}

	int os_map(Fh *&fhp, uint len, bool is_rdonly, void *&addrp) {
		int flags = is_rdonly ? MAP_PRIVATE : MAP_SHARED;
		int prot = PROT_READ | (is_rdonly ? 0 : PROT_WRITE);	
		addrp = mmap(NULL, len, prot, flags, fhp->fd, 0);
		if (addrp == (void *)-1)
			return -1;
		return 0;
	}

	int os_unmap(void *&addrp, uint len) {
		return munmap(addrp, len);
	}

	int os_seek(Fh *&fhp, uint len) {
		int ret = lseek(fhp->fd, len, SEEK_SET);
		fhp->lastseek = len;
		return ret;
	}

	void os_ioinfo(Fh *&fhp, uint *mb, uint *byte) {
		struct stat sb;

		fstat(fhp->fd, &sb);
		if (mb) 
			*mb = (sb.st_size) / MEGABYTE;

		if (byte)
			*byte = (sb.st_size) % MEGABYTE;
	}

	int os_write(Fh *&fhp, void *addr, uint len) {
		int nw = 0;
		u1b *taddr = (u1b *)addr;
		uint offset = 0;

		while (offset < len) {
			nw = write(fhp->fd, taddr, len - offset);
			offset += nw;
			taddr += nw;
		}
		return offset;
	}

	int os_read(Fh *&fhp, void *addr, uint len) {
		u1b *taddr = (u1b *)addr;
		uint offset = 0;
		int nr = 0;

		while (offset < len) {
			nw = read(fhp->fd, taddr, len - offset);
			offset += nr;
			taddr += nr;
		}
		return offset;
	}

	int zero_fill(Fh *&fhp) {
		uint write_offset = fhp->lastseek;
		uint mb, byte;
		int ret;

		os_ioinfo(fhp, &mb, &byte);
		uint stat_offset = mb * MEGABYTE + byte;

		if (stat_offset > write_offset)
			return (0);

		char *bp = new char[LARGE_WRITE];
		uint blen = LARGE_WRITE;

		ret = os_seek(fhp, stat_offset);
		while (stat_offset < write_offset) {
			if (write_offset - stat_offset <= blen) {
				blen = write_offset -stat_offset;
			}
			ret = os_write(fhp, bp, blen);
			stat_offset += blen;
		}
		ret = fsync(fhp->fd);
		delete[] bp;
		ret = os_seek(fhp, mb * MEGABYTE + byte);
		return ret;
	}

	int file_init(Fh *&fhp, uint len, int pattern) {
		char *bp = (char *)malloc(LARGE_WRITE);
		memset(bp, pattern, LARGE_WRITE);
		uint blen = LARGE_WRITE;
		uint startw = 0;

		int ret = os_seek(fhp, 0);
		while (startw < len) {
			if ((len - startw) <= blen) {
				blen = len - startw;
			}
			ret = os_write(fhp, bp, blen);
			startw += blen;
		}
		ret = fsync(fhp->fd);
		free(bp);
		return ret;
	}

	int file_extend(Fh *&fhp, uint size) {
		int ret = os_seek(fhp, size);
		ret = zero_fill(fhp);
		return ret;
	}

	int os_close(Fh *&fhp) {
		int ret = close(fhp->fd);
		delete fhp;
		return ret;
	}

	void os_fileid(const char *name, uint *p) {
		struct stat sb;
		stat(name, &sb);
		memset(p, 0, MAX_FILENAME);
		uint tmp = (uint)sb.st_ino;

		for (int i = sizeof(uint), uint *pt = &tmp; i > 0; i--) 
			*p++ = *pt++;

		tmp = (uint)sb.st_dev;
		for (int i = sizeof(uint), uint *pt = &tmp; i > 0; i--) 
			*p++ = *pt++;
	}

	uint hash_tablesize(uint nbuckets) {

#define  hash_size(power, prime) {		\
			if ((power) >= (nbuckets)) 	\
				return (prime);			\
		}	

		hash_size(32,37);			/*2^5*/
		hash_size(64,67);			/*2^6*/
		hash_size(128,131);			/*2^7*/
		hash_size(256,257);			/*2^8*/
		hash_size(512,521);			/*2^9*/
		hash_size(32,37);			/*2^5*/
		hash_size(64,67);			/*2^6*/
		hash_size(128,131);			/*2^7*/
		hash_size(256,257);			/*2^8*/
		hash_size(512,521);			/*2^9*/
		hash_size(1024,1031);			/*2^10*/
		hash_size(2048,2053);			/*2^11*/
		hash_size(4096,4099);			/*2^12*/
		hash_size(8192,8191);			/*2^13*/
		hash_size(16384,16381);		/*2^14*/
		hash_size(32768,32771);		/*2^15*/
		hash_size(65536,65537);		/*2^16*/
		hash_size(131072,131071);		/*2^17*/
		hash_size(262144,262147);		/*2^18*/
		hash_size(393216,393209);		/*2^18+2^18/2*/
		hash_size(524288,524287);		/*2^19*/
		hash_size(786432,786431);		/*2^19+2^19/2*/
		hash_size(1048576,1048573);		/*2^20*/
		hash_size(1572864,1572869);		/*2^20+2^20/2*/
		hash_size(2097152,2097169);		/*2^21*/
		hash_size(3145728,3145721);		/*2^21+2^21/2*/
		hash_size(4194304,4194301);		/*2^22*/
		hash_size(6291456,6291449);		/*2^22+2^22/2*/
		hash_size(8388608,8388617);		/*2^23*/
		hash_size(12582912,12582917);		/*2^23+2^23/2*/
		hash_size(16777216,16777213);		/*2^24*/
		hash_size(25165824,25165813);		/*2^24+2^24/2*/
		hash_size(33554432,33554393);		/*2^25*/
		hash_size(50331648,50331653);		/*2^25+2^25/2*/
		hash_size(67108864,67108859);		/*2^26*/
		hash_size(100663296,100663291);	/*2^26+2^26/2*/
		hash_size(134217728,134217757);	/*2^27*/
		hash_size(201326592,201326611);	/*2^27+2^27/2*/
		hash_size(268435456,268435459);	/*2^28*/
		hash_size(402653184,402653189);	/*2^28+2^28/2*/
		hash_size(536870912,536870909);	/*2^29*/
		hash_size(805306368,805306357);	/*2^29+2^29/2*/
		hash_size(1073741824,1073741827);	/*2^30*/
		return(1073741827);
	}
}
