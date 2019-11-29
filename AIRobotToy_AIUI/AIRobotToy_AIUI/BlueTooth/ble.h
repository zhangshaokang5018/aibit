#ifndef __BLE_H
#define __BLE_H

#include "stm32f4xx.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

void vBLE_Task(void *pvParameters);

#endif

