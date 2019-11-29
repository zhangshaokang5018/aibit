#ifndef __SR04_H
#define __SR04_H

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#define __TRIG_LOW()		GPIO_ResetBits(GPIOC, GPIO_Pin_1)
#define __TRIG_HIGH()		GPIO_SetBits(GPIOC, GPIO_Pin_1)

void vSR04_Init(void);//≥ı ºªØ
void vSR04_Task(void *pvParameters);
void UltrasonicWave_StartMeasure(void);
void sr04_timer(unsigned char start);
#endif
