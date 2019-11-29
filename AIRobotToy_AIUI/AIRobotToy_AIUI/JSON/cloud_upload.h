#ifndef _CLOUD_UPLOAD_H_
#define _CLOUD_UPLOAD_H_

#ifdef __cplusplus

#include <auto_lock.h>
#include <simple_thread.h>
#include "uthash.h"
#include "cloud_upload_thread.h"

namespace mw_util {

	class CloudUploadFileInfo 
	{
	public:
		CloudUploadFileInfo()
		{
			size_ = 0;
			path_name_[0] = '\0';
			modify_time_[0] = '\0';
		}
		~CloudUploadFileInfo(){}

		unsigned long size_;
		char path_name_[1024];
		char modify_time_[64];

		UT_hash_handle hh;
	};

	class CloudUpload {
	public:
		static CloudUpload& GetInstance() {
			static CloudUpload instance;
			return instance;
		}
		~CloudUpload(){}

		void set_running_status(int running_status);
		int get_running_status();

		void set_base_dir(const char *base_dir);
		char* get_base_dir();

		int AddFiles();
		int AddFile(const char *filename);
		int RemoveFile(const char *filename);
		int RemoveAll();
		int StartUpload();
		void StopUpload();
		// int GetFile(std::string &pathname, CloudUploadFileInfo &info);
		int GetFile(CloudUploadFileInfo *pInfo);
	protected:
		int GetFileInfo(const char *pathname, CloudUploadFileInfo *pInfo);
		int GetDirInfo(const char *dir);
	private:
		CloudUpload()
			:running_status_(0)
			,file_list_(NULL)
			,file_normal_list_(NULL)
			,file_uploaded_list_(NULL)
			,runable_(NULL){}
		CloudUpload(const CloudUpload &);
		CloudUpload & operator = (const CloudUpload &);

		int running_status_;
		char base_dir_[256];

		CMutex lock_;
		CMutex normal_lock_;
		CMutex uploaded_lock_;

		CloudUploadFileInfo *file_list_;
		CloudUploadFileInfo *file_normal_list_;
		CloudUploadFileInfo *file_uploaded_list_;

		CThread upload_thread_;
		IThreadRunable *runable_;
	};
}

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * start auto upload from \p directory
 *
 * @param dir upload base directory
 * @return \p 0 when successed, otherwise failed
 */
int cloud_upload_start(const char *dir);
/**
 * add single upload file
 *
 * @param filename upload file name
 * @return \p 0 when successed, otherwise failed
 */
int cloud_upload_add_file(const char* filename);
/**
 * remove single upload file
 *
 * @param filename remove file name
 * @return \p 0 when successed, otherwise failed, ex:not exsit
 */
int cloud_upload_remove_file(const char* filename);
/**
 * stop upload
 */
void cloud_upload_stop();

#ifdef __cplusplus
}
#endif

#endif //_CLOUD_UPLOAD_H_
