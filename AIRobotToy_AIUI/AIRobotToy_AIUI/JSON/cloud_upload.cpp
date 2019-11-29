#include "cloud_upload.h"
#include "cJSON.h"
#include "http_client.h"
#include <https.h>
#include <stdio.h>  
#include <string.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

using namespace mw_util;

void CloudUpload::set_running_status(int running_status)
{
	running_status_ = running_status;
}
int CloudUpload::get_running_status()
{
	return running_status_;
}

void CloudUpload::set_base_dir(const char *base_dir)
{
	strcpy(base_dir_, base_dir);
}
char * CloudUpload::get_base_dir()
{
	return base_dir_;
}

int CloudUpload::AddFile(const char *filename)
{
	CloudUploadFileInfo *pInfo = new CloudUploadFileInfo();
	if (pInfo) {
		GetFileInfo(filename, pInfo);
		AutoLock lock(lock_);
		HASH_ADD_STR(file_list_, path_name_, pInfo);
		return 0;
	}
	return -1;
}

int CloudUpload::RemoveFile(const char *filename)
{
	CloudUploadFileInfo *pInfo = NULL;
	{
		AutoLock lock(lock_);
		HASH_FIND_STR(file_list_, filename, pInfo);
		if (pInfo) {
			delete pInfo;
			HASH_DEL(file_list_, pInfo);
			return 0;
		}
	}
	{
		AutoLock lock(lock_);
		HASH_FIND_STR(file_normal_list_, filename, pInfo);
		if (pInfo) {
			delete pInfo;
			HASH_DEL(file_normal_list_, pInfo);
			return 0;
		}
	}
	return -1;
}

int CloudUpload::RemoveAll()
{
	CloudUploadFileInfo *iter = NULL;
	CloudUploadFileInfo *out = NULL;
	{
		CloudUploadFileInfo *iter = NULL;
		AutoLock lock(lock_);
	    HASH_ITER(hh, file_list_, iter, out) {
	        if (iter) {
	            HASH_DEL(file_list_, iter);
	            delete iter;
	        }
	    }
	}
	{
		CloudUploadFileInfo *iter = NULL;
		AutoLock lock(normal_lock_);
	    HASH_ITER(hh, file_normal_list_, iter, out) {
	        if (iter) {
	            HASH_DEL(file_normal_list_, iter);
	            delete iter;
	        }
	    }
	}
	{
		CloudUploadFileInfo *iter = NULL;
		AutoLock lock(uploaded_lock_);
	    HASH_ITER(hh, file_uploaded_list_, iter, out) {
	        if (iter) {
	            HASH_DEL(file_uploaded_list_, iter);
	            delete iter;
	        }
	    }
	}
	return 0;
}

int CloudUpload::AddFiles()
{
	return GetDirInfo(base_dir_);
}

int CloudUpload::StartUpload()
{
	if (get_running_status()) {
		return 0;
	}
	runable_ = dynamic_cast<IThreadRunable *>(new CUploadRunable());
	set_running_status(1);
	upload_thread_.set_thread_name("cloud_upload_thread");
	if (upload_thread_.Start(runable_, this)) {
		return 0;
	}
	else return -1;
}

void CloudUpload::StopUpload()
{
	if (get_running_status()) {
		set_running_status(0);
		upload_thread_.Stop();
	}
}

int CloudUpload::GetFile(CloudUploadFileInfo *pInfo)
{
	CloudUploadFileInfo *iter = NULL;
	CloudUploadFileInfo *out = NULL;
	if (NULL == pInfo) return -1;
	{
		AutoLock lock(lock_);
		HASH_ITER(hh, file_list_, iter, out) {
	        if (iter) {
	        	memcpy(pInfo, iter, sizeof(CloudUploadFileInfo));
	            HASH_DEL(file_list_, iter);
	            delete iter;
	            iter = out;
	            return 0;
	        }
	    }
	}
	{
		AutoLock lock(normal_lock_);
		HASH_ITER(hh, file_normal_list_, iter, out) {
	        if (iter) {
	        	memcpy(pInfo, iter, sizeof(CloudUploadFileInfo));
	            HASH_DEL(file_normal_list_, iter);
	            delete iter;
	            iter = out;
	            return 0;
	        }
	    }
	}
	return -1;
}

int CloudUpload::GetFileInfo(const char *pathname, CloudUploadFileInfo *pInfo)
{
	struct stat s;
	char *mtime = NULL;
	char dir_name[1024];
	time_t itime;
	if (NULL == pathname || NULL == pInfo) return -1;

	memset(&s,0,sizeof(struct stat));
	if (0 == stat(pathname, &s)) {
		itime = (time_t)s.st_mtime;
		mtime = ctime(&itime);
		if (mtime) {
			strcpy(pInfo->modify_time_, mtime);
		}
		pInfo->size_ = s.st_size;
		strcpy(pInfo->path_name_, pathname);
		return 0;
	}
	return -1;
}

int CloudUpload::GetDirInfo(const char *dir)
{
	DIR *pDir ;
    struct dirent *ent  ;  
    int i = 0  ;  
    char childpath[512];  

    pDir = opendir(dir);
    memset(childpath, 0, sizeof(childpath));

    while((ent = readdir(pDir)) != NULL) {  
        if(ent->d_type & DT_DIR) {
			if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)  
			        continue;  
			sprintf(childpath,"%s/%s",dir, ent->d_name);
			GetDirInfo(childpath);
        }
		else {
			memset(childpath, 0, sizeof(childpath));
			sprintf(childpath,"%s/%s",dir, ent->d_name);
			
			CloudUploadFileInfo *pInfo = new CloudUploadFileInfo();
			if (pInfo) {
				if (GetFileInfo(childpath, pInfo)) {
					delete pInfo;
				}
				else {
					AutoLock lock(normal_lock_);
					HASH_ADD_STR(file_normal_list_, path_name_, pInfo);
					printf("getfileinfo:%s, size:%d\n", pInfo->path_name_, HASH_COUNT(file_normal_list_));
				}
			}
		}
	} 
	return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int cloud_upload_start(const char *dir)
{
	int ret = -1;

	if (NULL == dir) return -EINVAL;
	if (CloudUpload::GetInstance().get_running_status()) {
		return 0;
	}

	CloudUpload::GetInstance().set_base_dir(dir);

	// get dir file list into CloudUpload list
	do {
		if (0 != (ret = CloudUpload::GetInstance().AddFiles())) {
			break;
		}
		if (0 != (ret = CloudUpload::GetInstance().StartUpload())) {
			break;
		}
	} while(0);
	return ret;
}
int cloud_upload_add_file(const char* filename)
{
	if (NULL == filename) return -EINVAL;

	return CloudUpload::GetInstance().AddFile(filename);
}
int cloud_upload_remove_file(const char* filename) {
	if (NULL == filename) return -EINVAL;

	return CloudUpload::GetInstance().RemoveFile(filename);
}
void cloud_upload_stop()
{
	CloudUpload::GetInstance().StopUpload();
}

#ifdef __cplusplus
}
#endif
