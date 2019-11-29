#include "cloud_upload_thread.h"
#include "cloud_upload.h"
#include "cJSON_user_define.h"
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

#define UPLOAD_REQUEST_MAX_SIZE (1024+1*1024*1024)
#define UPLOAD_RESPONSE_MAX_SIZE (4096)

using namespace mw_util;

typedef struct upload_base_response_info_t {
	char code[16];
	char desc[64];
}upload_base_response_info_t;

typedef struct upload_check_response_info_t {
	upload_base_response_info_t base_info_;
	char audio_path_[256];
	char order_file_id_[64];
	int order_upload_status_;
	long order_uploaded_size_;
	long cloud_file_id_;
	int cloud_upload_status_;
	long cloud_uploaded_size_;
	int is_error_;
	char error_desc_[256];
}upload_check_response_info_t;

typedef struct upload_prepare_response_info_t {
	upload_base_response_info_t base_info_;
	char audio_path_[256];
	char order_file_id_[64];
	int order_upload_status_;
	long order_uploaded_size_;
	char upload_url[256];
	int cloud_file_id_;
	int cloud_upload_status_;
	long cloud_uploaded_size_;
}upload_prepare_response_info_t;

static int upload_post_request(const char *url, const char* request, int req_len, char *response, int res_len, int is_bin = false)
{
	int ret = 0;
	char *content_type = NULL;
	HTTPClient http_client;

	if (is_bin) {
		content_type = "application/octet-stream";
	}
	else {
		content_type = "application/json; charset=utf-8";
	}

	http_client.HttpAddHeader("Content-type", content_type)
		.HttpAddHeader("X-Channel", "11020000")
		.HttpAddHeader("X-Platform", "iOS")
		.HttpAddHeader("X-Biz-Id", "xftjapp")
		.HttpAddHeader("X-Client-Version", "2.0.1199")
		.HttpAddHeader("X-Session-Id", "512199876020347890");
	ret = http_client.HttpPost(url, request, req_len, response, res_len);
	return ret;
}

void* CUploadRunable::OnRun(void* arg)
{
	int ret;
	int file_id, chunks;
	CloudUpload *upload = reinterpret_cast<CloudUpload *>(arg);
	char *url = NULL;
	char *request = (char *)malloc(UPLOAD_REQUEST_MAX_SIZE);
	char *response = (char *)malloc(UPLOAD_RESPONSE_MAX_SIZE);
	// printf("%p,%p\n", request, response);
	while (CloudUpload::GetInstance().get_running_status()) {

		CloudUploadFileInfo info;
		ret = CloudUpload::GetInstance().GetFile(&info);
		if (!ret) {

			// 1.上传检查
		    url = (char *)"https://testrec.iflytek.com/MSeriesAdaptService/v1/audios/uploadCheck";
		    upload_check_response_info_t upload_check_res_info;
		    upload_check_response_info_t *res_info = &upload_check_res_info;

		    JSON_DESERIALIZE_CREATE_ARRAY_START(json_array);
			JSON_DESERIALIZE_CREATE_OBJECT_START(json_obj);
			JSON_DESERIALIZE_ADD_STRING_TO_OBJECT(json_obj, "audioPath", info.path_name_);
			JSON_DESERIALIZE_ADD_INT_TO_OBJECT(json_obj, "audioSize", info.size_);
			JSON_DESERIALIZE_ADD_OBJECT_TO_ARRAY(json_array, json_obj);
			JSON_DESERIALIZE_STRING(json_array, request, UPLOAD_REQUEST_MAX_SIZE);
			JSON_DESERIALIZE_CREATE_END(json_array);
			printf("upload request:%s\n", request);
		    ret = upload_post_request(url, request, strlen(request)+1, response, UPLOAD_RESPONSE_MAX_SIZE);
		    printf("upload response:%s\n", response);
		    // {
			//   "code": "000000",
			//   "desc": "success",
			//   "biz": [
			//     {
			//       "audioPath": "/mnt/card/test1.pcm",
			//       "orderFileId": null,
			//       "orderUploadStatus": 0,
			//       "orderUploadedSize": null,
			//       "cloudFileId": 3933,
			//       "cloudUploadStatus": 0,
			//       "cloudUploadedSize": null,
			//       "isError": 1,
			//       "errorDesc": "获取上传音频信息失败"
			//     }
			//   ]
			// }
		    JSON_SERIALIZE_START(json_root, response, ret);
			JSON_SERIALIZE_GET_STRING_COPY(json_root, "code", res_info->base_info_.code, sizeof(res_info->base_info_.code), ret, 1);
			JSON_SERIALIZE_GET_STRING_COPY(json_root, "desc", res_info->base_info_.desc, sizeof(res_info->base_info_.desc), ret, 0);
			JSON_SERIALIZE_GET_ARRAY(json_root, "biz", json_array, ret, 1);
			JSON_SERIALIZE_ARRAY_FOR_EACH_START(json_array, sub_item, pos, total);
				JSON_SERIALIZE_GET_STRING_COPY(sub_item, "audioPath", res_info->audio_path_, sizeof(res_info->audio_path_), ret, 1);
				JSON_SERIALIZE_GET_STRING_COPY(sub_item, "orderFileId", res_info->order_file_id_, sizeof(res_info->order_file_id_), ret, 0);
				JSON_SERIALIZE_GET_INT(sub_item, "orderUploadStatus", res_info->order_upload_status_, ret, 0);
				JSON_SERIALIZE_GET_INT(sub_item, "orderUploadedSize", res_info->order_uploaded_size_, ret, 0);
				JSON_SERIALIZE_GET_INT(sub_item, "cloudFileId", res_info->cloud_file_id_, ret, 0);
				JSON_SERIALIZE_GET_INT(sub_item, "cloudUploadStatus", res_info->cloud_upload_status_, ret, 0);
				JSON_SERIALIZE_GET_INT(sub_item, "cloudUploadedSize", res_info->cloud_uploaded_size_, ret, 0);
				JSON_SERIALIZE_GET_INT(sub_item, "isError", res_info->is_error_, ret, 0);
				JSON_SERIALIZE_GET_STRING_COPY(sub_item, "errorDesc", res_info->error_desc_, sizeof(res_info->error_desc_), ret, 0);
			JSON_SERIALIZE_ARRAY_FOR_EACH_END();
			JSON_SERIALIZE_END(json_root, ret);

		    printf("ret:%d, code:%s, desc:%s, audioPath:%s, orderFileId:%s, orderUploadStatus:%d, orderUploadedSize:%ld,"
				"cloudFileId:%ld, cloudUploadStatus:%d, cloudUploadedSize:%ld, isError:%d, errorDesc:%s\n",
				ret, res_info->base_info_.code, res_info->base_info_.desc, res_info->audio_path_, 
				res_info->order_file_id_, res_info->order_upload_status_, res_info->order_uploaded_size_, 
				res_info->cloud_file_id_, res_info->cloud_upload_status_, res_info->cloud_uploaded_size_, 
				res_info->is_error_, res_info->error_desc_);
		    if (0 != atoi(res_info->base_info_.code)) continue;
		    if (0 != res_info->cloud_upload_status_) continue;
		    if (0 != ret) continue;

		    // 2.准备上传
		    file_id = res_info->cloud_file_id_;
		    upload_prepare_response_info_t pre_info;
		    // if (file_id <= 0) {
		    if (1) {
		    	url = (char *)"https://testrec.iflytek.com/MSeriesAdaptService/v1/audios/prepare";

				JSON_DESERIALIZE_CREATE_OBJECT_START(json_prepare_obj);
				JSON_DESERIALIZE_ADD_STRING_TO_OBJECT(json_prepare_obj, "audioPath", info.path_name_);
				JSON_DESERIALIZE_ADD_INT_TO_OBJECT(json_prepare_obj, "audioSize", info.size_);
				JSON_DESERIALIZE_ADD_STRING_TO_OBJECT(json_prepare_obj, "audioName", strrchr(info.path_name_, '/')+1);
				JSON_DESERIALIZE_ADD_INT_TO_OBJECT(json_prepare_obj, "uploadType", 1);
				JSON_DESERIALIZE_STRING(json_prepare_obj, request, UPLOAD_REQUEST_MAX_SIZE);
				JSON_DESERIALIZE_CREATE_END(json_prepare_obj);
				printf("upload prepare request:%s\n", request);
	    		ret = upload_post_request(url, request, strlen(request)+1, response, UPLOAD_RESPONSE_MAX_SIZE);
	    		printf("upload prepare response:%s\n", response);
	   			// {
				//     "code": "000000",
				//     "desc": "success",
				//     "biz": {
				//         "audioPath": "/mnt/card/decode_test.pcm",
				//         "orderFileId": null,
				//         "orderUploadStatus": 0,
				//         "uploadUrl": "https://testrec.iflytek.com/CloudStreamService/v1/cloud_files/?/chunks/?",
				//         "cloudFileId": 3941,
				//         "cloudUploadStatus": 0
				//     }
				// }
	    		
	    		JSON_SERIALIZE_START(json_root, response, ret);
				JSON_SERIALIZE_GET_STRING_COPY(json_root, "code", pre_info.base_info_.code, sizeof(pre_info.base_info_.code), ret, 1);
				JSON_SERIALIZE_GET_STRING_COPY(json_root, "desc", pre_info.base_info_.desc, sizeof(pre_info.base_info_.desc), ret, 0);
				JSON_SERIALIZE_GET_OBJECT(json_root, "biz", json_sub_obj, ret, 1);
				JSON_SERIALIZE_GET_STRING_COPY(json_sub_obj, "audioPath", pre_info.audio_path_, sizeof(pre_info.audio_path_), ret, 1);
				JSON_SERIALIZE_GET_STRING_COPY(json_sub_obj, "orderFileId", pre_info.order_file_id_, sizeof(pre_info.order_file_id_), ret, 0);
				JSON_SERIALIZE_GET_INT(json_sub_obj, "orderUploadStatus", pre_info.order_upload_status_, ret, 1);
				JSON_SERIALIZE_GET_INT(json_sub_obj, "orderUploadedSize", pre_info.order_uploaded_size_, ret, 0);
				JSON_SERIALIZE_GET_STRING_COPY(json_sub_obj, "uploadUrl", pre_info.upload_url, sizeof(pre_info.upload_url), ret, 1);
				JSON_SERIALIZE_GET_INT(json_sub_obj, "cloudFileId", pre_info.cloud_file_id_, ret, 0);
				JSON_SERIALIZE_GET_INT(json_sub_obj, "cloudUploadStatus", pre_info.cloud_upload_status_, ret, 1);
				JSON_SERIALIZE_GET_INT(json_root, "cloudUploadedSize", pre_info.cloud_uploaded_size_, ret, 0);
				JSON_SERIALIZE_END(json_root, ret);

				printf("ret:%d, code:%s, desc:%s, audioPath:%s, orderFileId:%s, orderUploadStatus:%d, orderUploadedSize:%ld,"
					"uploadUrl:%s, cloudFileId:%d, cloudUploadStatus:%d, cloudUploadedSize:%ld\n",
					ret, pre_info.base_info_.code, pre_info.base_info_.desc, pre_info.audio_path_, 
					pre_info.order_file_id_, pre_info.order_upload_status_, pre_info.order_uploaded_size_, 
					pre_info.upload_url, pre_info.cloud_file_id_, pre_info.cloud_upload_status_, 
					pre_info.cloud_uploaded_size_);
				file_id = pre_info.cloud_file_id_;
		    }
		    if (0 != atoi(pre_info.base_info_.code)) continue;
		    if (0 != ret) continue;
    		
    		// 3.上传云空间
    		int max_upload_size = 1024*1024;
    		int totalChunk, chunkSize, duration, leftsize, fread_size;
    		char *pPreUrl, *pPostUrl;
    		char base_url[256];
    		char chunks_name[32];
    		char file_id_buf[32];
    		char chunk_buf[32];
    		char full_url[256];
    		leftsize = info.size_;
    		totalChunk = info.size_ / max_upload_size + 1;

    		pPreUrl = pPostUrl = pre_info.upload_url;
    		pPostUrl = strchr(pPreUrl, '?');
    		strncpy(base_url, pPreUrl, pPostUrl-pPreUrl);
    		pPostUrl++;
    		pPreUrl = pPostUrl;

    		pPostUrl = strchr(pPreUrl, '?');
    		strncpy(chunks_name, pPreUrl, pPostUrl-pPreUrl);
    		pPostUrl++;
    		pPreUrl = pPostUrl;

    		printf("audio_path:%s\n", pre_info.audio_path_);
    		FILE* fp = fopen(pre_info.audio_path_, "rb");
    		fread_size = 0;
    		for (int i = 1; i <= totalChunk; i++) {
    			snprintf(full_url, sizeof(full_url), "%s%d%s%d", base_url, file_id, chunks_name, i);
    			if (leftsize <= 0) break;
    			if (leftsize <= max_upload_size) {
    				chunkSize = leftsize;
    				leftsize = 0;
    			}
    			else {
    				chunkSize = max_upload_size;
    				leftsize -= max_upload_size;
    			}
    			// printf("upload file url:%s\n", full_url);
    			JSON_DESERIALIZE_CREATE_OBJECT_START(json_file_obj);
				JSON_DESERIALIZE_ADD_INT_TO_OBJECT(json_file_obj, "totalChunk", totalChunk);
				JSON_DESERIALIZE_ADD_INT_TO_OBJECT(json_file_obj, "chunkSize", chunkSize);
				JSON_DESERIALIZE_ADD_INT_TO_OBJECT(json_file_obj, "duration", chunkSize/32);
				JSON_DESERIALIZE_STRING(json_file_obj, request+sizeof(int), UPLOAD_REQUEST_MAX_SIZE-sizeof(int));
				JSON_DESERIALIZE_CREATE_END(json_file_obj);
				printf("upload file request:%s\n", request+sizeof(int));

    			unsigned int num,num1,num2,num3,num4;
				unsigned int jsonSize = strlen(request+sizeof(int))+1;
			    char *pJsonSize = (char *)&jsonSize;
			    num1=(unsigned int)(*pJsonSize)<<24;
			    num2=((unsigned int)*(pJsonSize+1))<<16;
			    num3=((unsigned int)*(pJsonSize+2))<<8;
			    num4=((unsigned int)*(pJsonSize+3));
			    num=num1+num2+num3+num4;
			    int data_len = sizeof(jsonSize) + jsonSize + chunkSize;
			    memcpy(request, &num, sizeof(num));
			    fread_size = chunkSize;
			    while (fp && fread_size > 0) {
			    	printf("fread start:%d\n", fread_size);
			    	ret = fread(request+sizeof(jsonSize)+jsonSize, chunkSize, 1, fp);
			    	if (ret <= 0) break;
			    	fread_size -= ret*chunkSize;
			    	printf("fread end:%d,ret:%d, error:%s\n", fread_size, ret, strerror(errno));
			    }
			    // memcpy(request+sizeof(jsonSize)+jsonSize, json_string, jsonSize);

			    ret = upload_post_request(full_url, request, data_len, response, UPLOAD_RESPONSE_MAX_SIZE, true);
			    printf("upload file response:%s\n", response);
    		}
    		if (fp) fclose(fp);

			// TODO：save upload file to file_uploaded_list_
		}
		else {
			// waiting for upload file
			printf("waiting for upload file\n");
			sleep(10);
			// CloudUpload::GetInstance().set_running_status(0);
		}
	}
	return NULL;
}
