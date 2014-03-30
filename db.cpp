#include <iostream>
#include "wei_type.h"
#include "wei_common.h"
#include "wei_mem.h"
#include "wei_mutex.h"
using std::cout;
using std::endl;

using namespace WDB;


int main() {
	// mutex test
	/*
	Mutex_int ms(M_SHARED);
	Mutex_int mp(M_PROCESS_ONLY);

	ms.lock();
	ms.unlock();

	mp.lock();
	mp.unlock();
	*/
	// region test
	Region *rg = new Region(RG_ENV, 60 * 1024 * 1024);

	return 0;
	

}
