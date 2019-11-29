#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include "https.h"

#define HTTP_HEADER_CONTENT_TYPE 		"Content-Type: %s\r\n"
#define HTTP_HEADER_CHANNEL 			"X-Channel: 11020000\r\n"
#define HTTP_HEADER_PLATFORM 			"X-Platform: iOS\r\n"
#define HTTP_HEADER_BIZID 				"X-Biz-Id: xftjapp\r\n"
#define HTTP_HEADER_CLIENTVERSION 		"X-Client-Version: 2.0.1199\r\n"
#define HTTP_HEADER_SESSIONID 			"X-Session-Id: 512199876020347890\r\n"

#ifdef __cplusplus

namespace mw_util {

	class HTTPClient {
	public:
		HTTPClient();
		~HTTPClient();

		HTTPClient& HttpAddHeader(const char *key, const char *value);
		HTTPClient& HttpAddHeader(const char *key, int value);
		int HttpGet(const char *url, const char *request, int req_len, char *response, int res_len);
		int HttpPost(const char *url, const char *request, int req_len, char *response, int res_len);
		
	protected:
	private:
		HTTP_INFO http_info_;
	};
}

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * http/https post
 *
 * @param hi http info
 * @param url http url
 * @param request http request body
 * @param req_len http request length
 * @param response http reponse
 * @param res_len http response length
 * @return 200 when successed, otherwise failed
 */
int tiny_http_post(HTTP_INFO *hi, const char *url, const char *request, int req_len, char *response, int res_len);
/**
 * http/https get
 *
 * @param hi http info
 * @param url http url
 * @param request http request body
 * @param req_len http request length
 * @param response http reponse
 * @param res_len http response length
 * @return 200 when successed, otherwise failed
 */
int tiny_http_get(HTTP_INFO *hi, const char *url, const char *request, int req_len, char *response, int res_len);

#ifdef __cplusplus
}
#endif

#endif //_CLOUD_UPLOAD_H_
