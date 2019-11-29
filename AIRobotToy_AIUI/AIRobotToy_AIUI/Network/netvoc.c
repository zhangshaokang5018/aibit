#include "netvoc.h"

#include "esp8266.h"
#include "netlsn.h"
#include "network.h"
#include "urlencode.h"


#include <string.h>
#include <stdio.h>

#include "syslog.h"

#include "NTP.h"
#include "base64.h"
#include "md5.h"

//注册IFLYTEK AIUI开发者账号，配置以下内容。

#if USE_AIUI_ENGINE
#define SERVER_ADDR "http://openapi.xfyun.cn/v2/aiui"
#define AIUI_APPID "5d2f27d2"
#define AIUI_API_KEY "a605c4712faefae730cc84b62c0eb92f"
#define AIUI_AUE "raw"
#define AIUI_AUTH_ID "27853aa9684eb19789b784a89ea5befd"
#define AIUI_DATA_TYPE "audio"
#define AIUI_SAMPLE_RATE "16000"
#define AIUI_SCENE "main_box"
//#define AIUI_RESULT_LEVEL "complete"
#define AIUI_RESULT_LEVEL "plain"
#define AIUI_LAT "39.938838"
#define AIUI_LNG "116.368624"
#define AIUI_PERS_PARAM = "{\\\"auth_id\\\":\\\"e59720c21212f571769c7e85ac6c9ce7\\\"}"
//#define AIUI_PARAM "{\"result_level\":\"AIUI_RESULT_LEVEL\",\"auth_id\":\"AIUI_AUTH_ID\",\"data_type\":\"AIUI_DATA_TYPE\",\"sample_rate\":\"AIUI_SAMPLE_RATE\",\"scene\":\"AIUI_SCENE\",\"lat\":\"AIUI_LAT\",\"lng\":\"AIUI_LNG\"}"
#define AIUI_PARAM "{\"result_level\":\"plain\",\"auth_id\":\"27853aa9684eb19789b784a89ea5befd\",\"data_type\":\"audio\",\"sample_rate\":\"8000\",\"scene\":\"main_box\"}"
//使用个性化参数时参数格式如下：
//param = "{\"result_level\":\""+RESULT_LEVEL+"\",\"auth_id\":\""+AUTH_ID+"\",\"data_type\":\""+DATA_TYPE+"\",\"sample_rate\":\""+SAMPLE_RATE+"\",\"scene\":\""+SCENE+"\",\"lat\":\""+LAT+"\",\"lng\":\""+LNG+"\",\"pers_param\":\""+PERS_PARAM+"\"}"

//X-Param=eyJyZXN1bHRfbGV2ZWwiOiJjb21wbGV0ZSIsImF1dGhfaWQiOiJjODRhYmI3NDE3Y2QzOGJkOWQzNGI0MTBjZTI2ZmFkZSIsImRhdGFfdHlwZSI6ImF1ZGlvIiwic2FtcGxlX3JhdGUiOiIxNjAwMCIsInNjZW5lIjoibWFpbl9ib3giLCJsYXQiOiIzOS45Mzg4MzgiLCJsbmciOiIxMTYuMzY4NjI0In0=
//X-CurTime: 1571012315\r\n
//X-CheckSum: 1ce620b4bb78c059e33bf2dead70b293\r\n
//X-Appid: 5d2f27d2\r\n
static const char __audio2text[] = 
{
"POST /v2/aiui HTTP/1.1\r\n\
Host: openapi.xfyun.cn\r\n\
Connection: keep-alive\r\n\
User-Agent: AIRobotToy/1.0\r\n\
Accept: */*\r\n\
Accept-Encoding: gzip, deflate\r\n\
X-Appid: %s\r\n\
X-CurTime: %d\r\n\
X-Param: %s\r\n\
X-CheckSum: %s\r\n\
Content-Length: %s\r\n\r\n"
};
#else

#define SERVER_ADDR "openapi.baidu.com"
static const char __audio2text[] = 
{
"POST /server_api?\
lan=zh\
&cuid=xxxxxxxx\
&token=%s \
HTTP/1.1\r\nHost: vop.baidu.com\r\n\
Content-Type: wav;rate=8000\r\nContent-Length: %s\r\n\r\n"
};
#endif

//注册BAIDU开发者账号，将下面的xxxxxxxx替换为自己申请的ID。
static const char __get_token[] = 
{
"GET /oauth/2.0/token?\
grant_type=client_credentials\
&client_id=oOB2hX8gzU1jtP5m9GI4WDQK\
&client_secret=NW3AUu7MTnwb9RGjVh2OT0rhNeZnxbt0 \
HTTP/1.1\r\nHost: openapi.baidu.com\r\n\r\n"
};




static const char __text2audio[] = 
{
"GET /text2audio?\
tex=%s\
&lan=zh\
&cuid=001\
&ctp=1\
&per=4\
&tok=%s \
HTTP/1.1\r\nHost: tsn.baidu.com\r\n\r\n"
};

#include "bsp_init.h"
#define __TOKEN_SIZE	(72U)
static char pcTokenBuf[__TOKEN_SIZE+2] = {0};


uint8_t ucNet_ReadToken(void)
{
	//若http响应正确
	ts_printf("recv from server:%s\r\n", PubBuf);
	strcpy(pcTokenBuf, "25.1a9f61468dc17f63251c07a96a76d06c.315360000.1879466391.282335-16886296");
	return 0;
	if (strstr((char *)PubBuf, "HTTP/1.1 200 OK\r\n") != NULL)
	{
		char *pT = NULL;
		char *pS = NULL;
		char *pE = NULL;
		
		pT = strstr((char *)PubBuf, "refresh_token");
		pS = strstr(pT, ": \"");
		pE = strstr(pS, "\",");
		ts_printf("pS:%p, pE:%p, len:%d,*pS:%c,*pE:%c\r\n", pS, pE, pE - pS, *pS, *pE);
		if ((pE - pS) == __TOKEN_SIZE+3)
		{
			strncpy(pcTokenBuf, (char *)(pS+3), __TOKEN_SIZE);
			return 0;
		}
	}
	return 1;
}

void vNet_ReacquireToken(void)
{
	//连接证书服务器
	ucEsp_CIPSTART("TCP", SERVER_ADDR, 80);
	//开启透传模式
	ucEsp_CIPMODE(1);
	//准备发送数据
	ucEsp_CIPSEND(0,0);
	//把信号量置零
	xSemaphoreTake(xNetLsn_RecBinary, 0);
	vNet_SendBuf(__get_token, sizeof (__get_token));
	vNet_LsnStart(10000);
	//等待数据接收完毕
	xSemaphoreTake(xNetLsn_RecBinary, portMAX_DELAY);
	//发送关闭监听信号量
	vNetLsn_Close();
	//退出透传模式
	ucEsp_BreakSEND();
	//关闭透传
	ucEsp_CIPMODE(0);
	//断开和服务器的连接
	ucEsp_CIPCLOSE();
}

//最多支持64个汉字(64*2*3)
#define URL_BUF_SIZE				(384U)
static char pcNetUrl[URL_BUF_SIZE] = {0};

//文字转语音接口，输入参数UTF-8格式的汉字串
void vNet_Text2Audio(const char *pcText)
{
	memset(pcNetUrl, 0, sizeof (pcNetUrl));
	ulURL_Encode(pcText, \
				(strlen(pcText)*3 < URL_BUF_SIZE) ? strlen(pcText) : URL_BUF_SIZE, \
				pcNetUrl);
	//填充字段到发送缓冲区
	memset(pNET_SEND_BUF0, 0, NET_BUFFER_SIZE);
	snprintf((char *)pNET_SEND_BUF0, NET_BUFFER_SIZE, __text2audio, pcNetUrl, pcTokenBuf);
	
	ucEsp_CIPSTART("TCP", "tsn.baidu.com", 80);
	ucEsp_CIPMODE(1);
	ucEsp_CIPSEND(0,0);
	xSemaphoreTake(xNetLsn_RecBinary, 0);
	vNet_SendBuf((char *)pNET_SEND_BUF0, strlen((char *)pNET_SEND_BUF0));
	vNet_LsnStart(10000);
	xSemaphoreTake(xNetLsn_RecBinary, portMAX_DELAY);
	vNetLsn_Close();
	ucEsp_BreakSEND();
	ucEsp_CIPMODE(0);
	ucEsp_CIPCLOSE();
}



#include "ff.h"
//语音文件的文件指针(不使用动态分配)
static FIL redAu_fp = {0};
//每次读取的文件大小
static uint32_t ulrByte = 0;
//语音的文件大小的字符串
static char pcConLen[16] = {0};

#if USE_AIUI_ENGINE
static char pcBase64[1024] = {0};
static char pcCheckSum[1024] = {0};
static unsigned char pcMD5[16] = {0};
static unsigned char pcMD5_Dist[33] = {0};
uint8_t ucAIUI_Audio2Text(const char *pcfile)
{
    uint32_t ulTime;
	if (f_open(&redAu_fp, pcfile, FA_READ) == FR_OK)
	{
		memset(pcConLen, 0, sizeof(pcConLen));
		snprintf(pcConLen, sizeof(pcConLen)-1, "%lld", f_size(&redAu_fp));
		
		memset((char *)pNET_SEND_BUF0, 0, NET_BUFFER_SIZE);
        
        ulTime = ulNTP_Get();
        ts_printf("ulTime:%u\r\n", ulTime);
        iBase64_Encode(AIUI_PARAM, pcBase64, strlen(AIUI_PARAM));
        ts_printf("pcBase64:%s, AIUI_PARAM:%s\r\n", pcBase64, AIUI_PARAM);
        sprintf(pcCheckSum, "%s%d%s", AIUI_API_KEY, ulTime, pcBase64);
        vMD5((uint8_t *)pcCheckSum, strlen(pcCheckSum), pcMD5);
        //HASH_MD5((uint8_t *)pcCheckSum, strlen(pcCheckSum), pcMD5);
        for(int i=0; i<16; i++)
        {
            sprintf(pcMD5_Dist+i*2, "%2.2x",  pcMD5[i]);
            ts_printf("%2.2x", pcMD5[i]);
        }
        ts_printf("pcMD5_Dist:%s\r\n", pcMD5_Dist);
		sprintf((char *)pNET_SEND_BUF0, __audio2text, AIUI_APPID, ulTime, pcBase64, pcMD5_Dist, pcConLen);
        ts_printf("pNET_SEND_BUF0:%s\r\n", pNET_SEND_BUF0);
		xSemaphoreTake(xNetLsn_RecBinary, 0);
		ucEsp_CIPSTART("TCP", "openapi.xfyun.cn", 80);
		
		//因为涉及到大文件的传输，使用透传会发生数据丢失
        ts_printf("vNet_SendBuf, len:%d\r\n", strlen((char *)pNET_SEND_BUF0));
		ucEsp_CIPMODE(0);
		ucEsp_CIPSEND(0xFF, strlen((char *)pNET_SEND_BUF0));
		vNet_SendBuf((char *)pNET_SEND_BUF0, strlen((char *)pNET_SEND_BUF0));
		if (ucEsp_WaitSendOK())
		{
			f_close(&redAu_fp);
			ucEsp_CIPCLOSE();
            ts_printf("ucEsp_WaitSendOK error, len:%d\r\n", strlen((char *)pNET_SEND_BUF0));
			return 2;
		}
		for (;;)
		{
			memset(pNET_SEND_BUF0, 0, NET_BUFFER_SIZE);
			f_read(&redAu_fp, pNET_SEND_BUF0, NET_BUFFER_SIZE, &ulrByte);
			ucEsp_CIPSENDBUF(0xFF, ulrByte);
			vNet_SendBuf((char *)pNET_SEND_BUF0, ulrByte);
			if (ucEsp_WaitSendOK())
			{
				f_close(&redAu_fp);
				ucEsp_CIPCLOSE();
                ts_printf("ucEsp_WaitSendOK error 2\r\n");
				return 2;
			}
			if(ulrByte < NET_BUFFER_SIZE)
			{
				break;
			}
		}
		vNet_LsnStart(10000);
		f_close(&redAu_fp);
		xSemaphoreTake(xNetLsn_RecBinary, portMAX_DELAY);
		ucEsp_CIPCLOSE();
		return 0;
	}
	else
	{
		ts_printf("文件系统异常，请检查！");
		f_close(&redAu_fp);
		return 1;
	}
}
#endif

uint8_t ucNet_Audio2Text(const char *pcfile)
{
	if (f_open(&redAu_fp, pcfile, FA_READ) == FR_OK)
	{
		memset(pcConLen, 0, sizeof(pcConLen));
		snprintf(pcConLen, sizeof(pcConLen)-1, "%lld", f_size(&redAu_fp));
		
		memset((char *)pNET_SEND_BUF0, 0, NET_BUFFER_SIZE);
		sprintf((char *)pNET_SEND_BUF0, __audio2text, pcTokenBuf, pcConLen);
		xSemaphoreTake(xNetLsn_RecBinary, 0);
		ucEsp_CIPSTART("TCP", "vop.baidu.com", 80);
		
		//因为涉及到大文件的传输，使用透传会发生数据丢失
		ucEsp_CIPMODE(0);
		ucEsp_CIPSEND(0xFF, strlen((char *)pNET_SEND_BUF0));
		vNet_SendBuf((char *)pNET_SEND_BUF0, strlen((char *)pNET_SEND_BUF0));
		if (ucEsp_WaitSendOK())
		{
			f_close(&redAu_fp);
			ucEsp_CIPCLOSE();
			return 2;
		}
		for (;;)
		{
			memset(pNET_SEND_BUF0, 0, NET_BUFFER_SIZE);
			f_read(&redAu_fp, pNET_SEND_BUF0, NET_BUFFER_SIZE, &ulrByte);
			ucEsp_CIPSENDBUF(0xFF, ulrByte);
			vNet_SendBuf((char *)pNET_SEND_BUF0, ulrByte);
			if (ucEsp_WaitSendOK())
			{
				f_close(&redAu_fp);
				ucEsp_CIPCLOSE();
				return 2;
			}
			if(ulrByte < NET_BUFFER_SIZE)
			{
				break;
			}
		}
		vNet_LsnStart(10000);
		f_close(&redAu_fp);
		xSemaphoreTake(xNetLsn_RecBinary, portMAX_DELAY);
		ucEsp_CIPCLOSE();
		return 0;
	}
	else
	{
		ts_printf("文件系统异常，请检查！");
		f_close(&redAu_fp);
		return 1;
	}
}


#if USE_AIUI_ENGINE
uint8_t ucNet_GetText(char *pcText)
{
	//若http响应正确
    ts_printf("http response:%s\r\n", PubBuf);
	if (strstr((char *)PubBuf, "HTTP/1.1 200 OK\r\n") != NULL)
	{
		//若识别成功
		if (strstr((char *)PubBuf, "\"result_id\":0,") == NULL)
		{
			char *pT = NULL;
			char *pS = NULL;
			char *pE = NULL;
			
			pT = strstr((char *)PubBuf, "\"text\":");
            ts_printf("pT:%s\r\n", pT);
			pS = strstr(pT, ":\"");
            ts_printf("pS:%s\r\n", pS);
			pE = strstr(pS, "\",");
            ts_printf("pE:%s\r\n", pE);
            
			if (pS != NULL && pE != NULL)
			{
				strncpy(pcText, (char *)(pS+2), (pE-pS-2));
				return 0;
			}
		}
	}
	return 1;
}
#else
uint8_t ucNet_GetText(char *pcText)
{
	//若http响应正确
    ts_printf("http response:%s\r\n", PubBuf);
	if (strstr((char *)PubBuf, "HTTP/1.1 200 OK\r\n") != NULL)
	{
		//若识别成功
		if (strstr((char *)PubBuf, "\"err_no\":0,") != NULL)
		{
			char *pT = NULL;
			char *pS = NULL;
			char *pE = NULL;
			
			pT = strstr((char *)PubBuf, "\"result\":");
			pS = strstr(pT, "[");
			pE = strstr(pS, "],");
			if (pS != NULL && pE != NULL)
			{
				strncpy(pcText, (char *)(pS+2), (pE-pS-6));
				return 0;
			}
		}
	}
	return 1;
}
#endif

