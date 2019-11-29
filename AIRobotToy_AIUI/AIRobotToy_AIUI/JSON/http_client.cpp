#include "http_client.h"
#include <string.h>
#include <stdlib.h>

using namespace mw_util;

HTTPClient::HTTPClient()
{
	memset(&http_info_, 0, sizeof(http_info_));
}

HTTPClient::~HTTPClient()
{

}

HTTPClient& HTTPClient::HttpAddHeader(const char *key, const char *value)
{
	if (NULL == key || NULL == value) return *this;
	if (0 == strcmp(key, "Content-type")) {
		snprintf(http_info_.request.content_type, H_FIELD_SIZE, "%s: %s", key, value);
	}
	else if (0 == strcmp(key, "X-Channel")) {
		snprintf(http_info_.request.xChannel, H_FIELD_SIZE, "%s: %s", key, value);
	}
	else if (0 == strcmp(key, "X-Platform")) {
		snprintf(http_info_.request.xPlatform, H_FIELD_SIZE, "%s: %s", key, value);
	}
	else if (0 == strcmp(key, "X-Biz-Id")) {
		snprintf(http_info_.request.xBizId, H_FIELD_SIZE, "%s: %s", key, value);
	}
	else if (0 == strcmp(key, "X-Client-Version")) {
		snprintf(http_info_.request.xClientVersion, H_FIELD_SIZE, "%s: %s", key, value);
	}
	else if (0 == strcmp(key, "X-Session-Id")) {
		snprintf(http_info_.request.xSessionId, H_FIELD_SIZE, "%s: %s", key, value);
	}
	return *this;
}
HTTPClient& HTTPClient::HttpAddHeader(const char *key, int value)
{
	if (NULL == key) return *this;
	return *this;
}
int HTTPClient::HttpGet(const char *url, const char *request, int req_len, char *response, int res_len)
{
	int ret = -1;
	if (NULL == url || NULL == request || NULL == response) return ret;

	if (http_open(&http_info_, (char *)url) < 0) {
        http_strerror(response, res_len);
        return ret;
    }

	do {
		
	    snprintf(http_info_.request.method, sizeof(http_info_.request.method), "GET");
	    http_info_.request.close = FALSE;
    	http_info_.request.chunked = FALSE;

	    http_info_.request.content_length = req_len;

	    if (http_write_header(&http_info_) < 0) {
	        http_strerror(response, res_len);
	        break;
	    }

	    if (http_write(&http_info_, (char *)request, req_len) != req_len) {
	        http_strerror(response, res_len);
	        break;
	    }

	    // Write end-chunked
	    if (http_write_end(&http_info_) < 0) {
	        http_strerror(response, res_len);
	        break;
	    }

	    ret = http_read_chunked(&http_info_, response, res_len);
	    http_close(&http_info_);
	    
	} while(0);

    return ret;
}
int HTTPClient::HttpPost(const char *url, const char *request, int req_len, char *response, int res_len)
{
	int ret = -1;
	if (NULL == url || NULL == request || NULL == response) return ret;

	if (http_open(&http_info_, (char *)url) < 0) {
        http_strerror(response, res_len);
        return ret;
    }

	do {
		
	    snprintf(http_info_.request.method, sizeof(http_info_.request.method), "POST");
	    http_info_.request.close = FALSE;
    	http_info_.request.chunked = FALSE;

	    http_info_.request.content_length = req_len;

	    if (http_write_header(&http_info_) < 0) {
	        http_strerror(response, res_len);
	        break;
	    }

	    if (http_write(&http_info_, (char *)request, req_len) != req_len) {
	        http_strerror(response, res_len);
	        break;
	    }

	    // Write end-chunked
	    if (http_write_end(&http_info_) < 0) {
	        http_strerror(response, res_len);
	        break;
	    }

	    ret = http_read_chunked(&http_info_, response, res_len);
	    http_close(&http_info_);

	} while(0);

    return ret;
}

#ifdef __cplusplus
extern "C" {
#endif

int tiny_http_post(HTTP_INFO *hi, const char *url, const char *request, int req_len, char *response, int res_len)
{
	return 0;
}
int tiny_http_get(HTTP_INFO *hi, const char *url, const char *request, int req_len, char *response, int res_len)
{
	return 0;
}

#ifdef __cplusplus
}
#endif