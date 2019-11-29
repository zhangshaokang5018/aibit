#ifndef __L298N_H
#define __L298N_H

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#define __LEFT_MOTOR_IN1_LOW()		GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define __LEFT_MOTOR_IN1_HIGH()		GPIO_SetBits(GPIOA, GPIO_Pin_4)

#define __LEFT_MOTOR_IN2_LOW()		GPIO_ResetBits(GPIOA, GPIO_Pin_5)
#define __LEFT_MOTOR_IN2_HIGH()		GPIO_SetBits(GPIOA, GPIO_Pin_5)

#define __LEFT_MOTOR_IN3_LOW()		GPIO_ResetBits(GPIOA, GPIO_Pin_11)
#define __LEFT_MOTOR_IN3_HIGH()		GPIO_SetBits(GPIOA, GPIO_Pin_11)

#define __LEFT_MOTOR_IN4_LOW()		GPIO_ResetBits(GPIOA, GPIO_Pin_12)
#define __LEFT_MOTOR_IN4_HIGH()		GPIO_SetBits(GPIOA, GPIO_Pin_12)

void vMotor_Init(void);

void Motor_Forword(void);
void Motor_Backword(void);
void Motor_Left(void);
void Motor_Right(void);
void Motor_Forword_Left(void);
void Motor_Forword_Right(void);
void Motor_Backword_Left(void);
void Motor_Backword_Right(void);
void Motor_Stop(void);

#endif
