#ifndef _SIMPLE_THREAD_H_
#define _SIMPLE_THREAD_H_

#include <stddef.h>
#include <pthread.h>
typedef void* (*THREAD_CALLBACK)(void *arg);

#ifdef __cplusplus

namespace mw_util {
	class IThreadRunable
	{
	public:
	    IThreadRunable(){}
	    virtual ~IThreadRunable(){}
	    virtual void* OnRun(void* arg) = 0;
	};

	struct ThreadData {
	ThreadData() {
			runable_ = NULL;
			running_ = 0;
			thread_id_ = -1;
			user_data_ = NULL;
			cb_ = NULL;
			thread_name_[0] = '\0';
		}
		IThreadRunable *runable_;
		int running_;
		pthread_t thread_id_;
		void *user_data_;
		THREAD_CALLBACK cb_;
		char thread_name_[64];
	};

	class CThread
	{
	public:
	    CThread(void);
	    ~CThread(void);

	    int set_thread_name(const char *thread_name);
	    char *get_thread_name();

	    bool Start(IThreadRunable *runable, void *user_data = NULL);
	    bool Start(THREAD_CALLBACK cb, void *user_data = NULL);
	    bool Stop();
	    bool IsRunning();

	    IThreadRunable* GetRunable();
	    int GetId();
	private:
	    CThread(const CThread&);
	    const CThread& operator=(const CThread&);

	    static void* ThreadProc(void* param);
	    static void* ThreadProc(THREAD_CALLBACK cb, void* param = NULL);
	    struct ThreadData private_data_;
	};
}

#endif

#ifdef __cplusplus
extern "C" {
#endif

void* thread_start(THREAD_CALLBACK cb, void *arg);
void thread_stop(void* handle);

#if 0
example:

void* thread_run(void* arg) {
    int *param = (int *)arg;
    while(*param) {
        printf("thread_run, param:%d\n", *param);
        sleep(1);
    }
    return NULL;
}
int test_thread() {
	void *handle = thread_start(thread_run, (void*)&thread_running);
	while(1) {
	    sleep(10);
	    thread_running = 0;
	    thread_stop(handle);
	    printf("thread_exit\n");
	}
	return 0;
}


#endif

#ifdef __cplusplus
}
#endif

#endif // _SIMPLE_THREAD_H_