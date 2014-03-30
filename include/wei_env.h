#ifndef __WENV_H
#define __WENV_H
namespace WDB {

	class ENV {
		Mutex_int env_mutex;
		public:
		Mtxpool *mtxp;
		Mpool *mp;



	};

}
#endif
