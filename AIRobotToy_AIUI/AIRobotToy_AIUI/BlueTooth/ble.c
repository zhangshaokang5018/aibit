#include "ble.h"
#include "string.h"
#include "syslog.h"
#include "usart.h"
#include "usart3.h"
#include "L298N.h"
#include "sr04.h"

static char pcSerialAT_Cache[4];

u8 ucRecvData()
{
    u8 ret = 1;
    char rxtemp = 0;
    
    xSemaphoreTake(xUsart3_RxMutex, portMAX_DELAY);
    memset(pcSerialAT_Cache, 0, sizeof(pcSerialAT_Cache));
    for (uint16_t i=0; i<sizeof(pcSerialAT_Cache); i++)
    {
        if (ucUsart3_GetChar(&rxtemp, 1000/portTICK_RATE_MS) == 0)
        {
            pcSerialAT_Cache[i] = rxtemp;
            ucUsart1_PutChar(rxtemp, 100);
            ret = 0;
            break;
        }
        else
            break;
    }
    xSemaphoreGive(xUsart3_RxMutex);
    return ret;
}

void vBLE_Task(void *pvParameters)
{
    u8 ret, start = 1;
    vTaskDelay(5000);
	for(;;)
	{
		//vTaskDelay(1000);
        ret = ucRecvData();
        if (ret) continue;
        switch(pcSerialAT_Cache[0])
        {
            case 'w':
                ts_printf("上\r\n");
                Motor_Forword();
                break;
            case 'x':
                ts_printf("下\r\n");
                Motor_Backword();
                break;
            case 'a':
                ts_printf("左\r\n");
                Motor_Left();
                break;
            case 'd':
                ts_printf("右\r\n");
                Motor_Right();
                break;
            case 's':
                ts_printf("停止\r\n");
                start = 0;
                Motor_Stop();
                break;
            case 'q':
                ts_printf("左上\r\n");
                Motor_Forword_Left();
                break;
            case 'e':
                ts_printf("右上\r\n");
                Motor_Forword_Right();
                break;
            case 'z':
                ts_printf("左下\r\n");
                Motor_Backword_Left();
                break;
            case 'c':
                ts_printf("右下\r\n");
                Motor_Backword_Right();
                
                break;
            default:
                ts_printf("未知\r\n");
                break;
        }
        //sr04_timer(start);

        u3_printf("hello, ble\r\n");
	}
}
