#ifndef _AutoLock_H
#define _AutoLock_H   

#include <pthread.h>

#ifdef __cplusplus

namespace mw_util {
class ILock
{
	public:  
		virtual ~ILock() {}  

		virtual void Lock() const = 0;  
		virtual void Unlock() const = 0;  
};  

class CMutex : public ILock  
{  
	public:  
		CMutex() {
			pthread_mutex_init(&mutex_, NULL);  
		} 
		~CMutex(){
			pthread_mutex_destroy(&mutex_);  
		}

		virtual void Lock() const {
			pthread_mutex_lock(&mutex_);  
		}
		virtual void Unlock() const {
			pthread_mutex_unlock(&mutex_);  
		}

	private:  
		mutable pthread_mutex_t mutex_;  
};  

class AutoLock  
{
	public:  
		AutoLock(const ILock& lock):lock_(lock) {
			lock_.Lock();  
		}
		~AutoLock(){
			lock_.Unlock();  
		} 

	private:  
		const ILock& lock_;  
};

}

#endif

#endif  
