#include "network.h"
#include "player.h"
#include "bsp_init.h"
#include "urlencode.h"
#include "netcmd.h"
#include "syslog.h"
#include <string.h>



uint8_t pucNet_SendBuf[NET_BUFFER_SIZE*2] = {0};
uint8_t pucNet_ReceiveBuf[NET_BUFFER_SIZE*2] = {0};

SemaphoreHandle_t xNetStart_Binary = NULL;
SemaphoreHandle_t xNetEnd_Binary = NULL;


static char SSID_gbk[128] = {"AIBIT"};
static char PSWD[64] = {"12348765"};


#if 0
//IDE和网络的编码都是UTF-8的，如果使用中文的SSID实际使用需要转换成GBK的格式
static char SSID_utf8[128] = {""};
//函数说明，将utf-8的SSID转换为GBK格式
static void vSSID_ChangeCode(void)
{
	char temp[128] = {0};
	uint8_t unilen = 0;		//unicode可能中间包含有0x00，所以不能strlen;
	
	unilen = ulUtf_8toUnicode(SSID_utf8, strlen(SSID_utf8), temp);
	UnicodetoGbk(temp, unilen, SSID_gbk);
}
#endif

static u8 packetBuffer[48] = {0xe3,0,6,0xec,0,0,0,0,0,0,0,0,0x31,0x4e,0x31,0x34};
uint32_t ulNTP_Get()
{    
    unsigned long ulTime = 0;
    //连接证书服务器
    //ucEsp_CIPSTART("UDP", "time.windows.com", 123);
	ucEsp_CIPSTART("UDP", "1.cn.pool.ntp.org", 123);
    //ucEsp_CIPSTART("UDP", "2.cn.pool.ntp.org", 123);
    //ucEsp_CIPSTART("UDP", "3.cn.pool.ntp.org", 123);
    //ucEsp_CIPSTART("UDP", "0.cn.pool.ntp.org", 123);
    //ucEsp_CIPSTART("UDP", "cn.pool.ntp.org", 123);
	//开启透传模式
	ucEsp_CIPMODE(1);
	//准备发送数据
	ucEsp_CIPSEND(0,0);
	//把信号量置零
	xSemaphoreTake(xNetLsn_RecBinary, 0);
	vNet_SendBuf((const char *)packetBuffer, sizeof (packetBuffer));
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
    ulTime = (unsigned long)PubBuf[40];
    ulTime = PubBuf[40] << 24 | PubBuf[41] << 16 | PubBuf[42] << 8 | PubBuf[43];
    //ulTime -= 2207520000;
    ulTime -= 2208988800;
    
    ts_printf("ulTime:%u\r\n", ulTime);
    ts_printf("%u %u:%u:%u\r\n", ulTime/31556736+1970, (ulTime/3600)%24+8, (ulTime/60)%60, ulTime%60);
    return ulTime;
}

void vNet_Task(void *pvParameters)
{
	static char VocText[1280] = {0};
	
	vNetLsn_Init();
	vSemaphoreCreateBinary(xNetStart_Binary);
	vSemaphoreCreateBinary(xNetEnd_Binary);
	xSemaphoreTake(xNetStart_Binary, 0);
	
net_reset:
	ts_printf("正在初始化网络连接...\r\n");
	vEsp_Init();
	ts_printf("after vEsp_Init...\r\n");
	if ( ucEsp_CWMODE_CUR(1) )
    {
        ts_printf("ucEsp_CWMODE_CUR error\r\n");
        goto net_reset;
    }
	if ( ucEsp_CWJAP_CUR(SSID_gbk, PSWD) ) 
    {
        ts_printf("ucEsp_CWJAP_CUR error\r\n");
        goto net_reset;
    }
    ts_printf("网络连接成功！\r\n");

    #if USE_AIUI_ENGINE
    #else
	vNet_ReacquireToken();
	if ( ucNet_ReadToken() ) 
    {
        ts_printf("ucNet_ReadToken error\r\n");
        goto net_reset;
    }
    #endif
	
	for (;;)
	{
		ts_printf("start xSemaphoreGive\r\n");
		xSemaphoreGive(xNetEnd_Binary);
		ts_printf("start xSemaphoreTake\r\n");
		xSemaphoreTake(xNetStart_Binary, portMAX_DELAY);
		ts_printf("start ucEsp_CWJAP\r\n");
		if ( ucEsp_CWJAP() ) {goto net_reset;}//检查网络连接是否正常
		ts_printf("正在识别中...\r\n");
        #if USE_AIUI_ENGINE
        if ( ucAIUI_Audio2Text("0:/SmartSpeaker/record") ) {
            ts_printf("ucAIUI_Audio2Text error\r\n");
            goto net_reset;
        }
        #else
        if ( ucNet_Audio2Text("0:/SmartSpeaker/record") ) {goto net_reset;}
        #endif
		memset(VocText, 0, sizeof (VocText));
		if ( ucNet_GetText(VocText) == 0 )
		{
			ts_printf("识别结果：%s \r\n",VocText);
			ucParseWord(1, (char **)(&VocText));
		}
		else
		{
			ts_printf("识别失败！\r\n");
		}
	}
}



