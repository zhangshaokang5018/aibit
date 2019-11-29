#ifndef _CLOUD_UPLOAD_THREAD_H_
#define _CLOUD_UPLOAD_THREAD_H_

#include "simple_thread.h"

#ifdef __cplusplus

namespace mw_util {

	class CUploadRunable: public IThreadRunable
	{
	public:
	    CUploadRunable(){}
	    virtual ~CUploadRunable(){}
	    virtual void* OnRun(void* arg);
	};
}

#endif

#endif //_CLOUD_UPLOAD_THREAD_H_
