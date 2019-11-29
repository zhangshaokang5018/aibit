#include "simple_thread.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
using namespace mw_util;

CThread::CThread()
{

}

CThread::~CThread()
{

}

void* CThread::ThreadProc(void* param)
{
	struct ThreadData *pri_data = reinterpret_cast<struct ThreadData*>(param);

	IThreadRunable *runbale = pri_data->runable_;
	THREAD_CALLBACK cb = pri_data->cb_;
	pri_data->running_ = 1;

	if (runbale) {
		runbale->OnRun(pri_data->user_data_);
	}
	else if (cb) {
		cb(pri_data->user_data_);
	}

	pri_data->running_ = 0;

	printf("thread [%s] exit\n", pri_data->thread_name_);

	return NULL;
}

int  CThread::set_thread_name(const char *thread_name)
{
	return snprintf(private_data_.thread_name_, sizeof(private_data_.thread_name_), "%s", thread_name);
}
char* CThread::get_thread_name()
{
	return private_data_.thread_name_;
}

bool CThread::Start(IThreadRunable *runable, void *user_data)
{
	int ret;
	pthread_t tid;
	pthread_attr_t attr;
	private_data_.runable_ = runable;
	private_data_.user_data_ = user_data;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&private_data_.thread_id_, &attr, ThreadProc, &private_data_);
	pthread_attr_destroy(&attr);

	return true;
}
bool CThread::Start(THREAD_CALLBACK cb, void *user_data)
{
	pthread_attr_t attr;
	private_data_.cb_ = cb;
	private_data_.user_data_ = user_data;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&private_data_.thread_id_, &attr, ThreadProc, &private_data_);
	pthread_attr_destroy(&attr);

	return true;
}
bool CThread::Stop()
{
	while (private_data_.running_) {
		// waiting for OnRun return
		usleep(100*1000);
	}
	return true;
}
bool CThread::IsRunning()
{
	return private_data_.running_;
}

IThreadRunable* CThread::GetRunable()
{
	return private_data_.runable_;
}
int CThread::GetId()
{
	return private_data_.thread_id_;
}

#ifdef __cplusplus
extern "C" {
#endif

void* thread_start(THREAD_CALLBACK cb, void *arg)
{
	CThread *thread = new CThread();
	if (thread) {
		thread->Start(cb, arg);
	}
	return reinterpret_cast<void *>(thread);
}
void thread_stop(void* handle)
{
	CThread *thread = reinterpret_cast<CThread *>(handle);
	if (thread) {
		thread->Stop();
		delete thread;
	}
}

#ifdef __cplusplus
}
#endif