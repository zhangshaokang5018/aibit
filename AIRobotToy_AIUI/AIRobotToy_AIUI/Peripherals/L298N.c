#include "L298N.h"
#include "stm32f4xx_tim.h"

void vMotor_GPIO_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void vLeftPWM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_DeInit(TIM3);

	TIM_TimeBaseStructure.TIM_Period = 1000;		//1ms(周期)
	TIM_TimeBaseStructure.TIM_Prescaler = 10000;		//定时器分频系数，定时器时钟为82MHz，分频后得1MHz,即1000kHz,0.001ms定时器加1
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//PWM1为正常占空比模式，PWM2为反极性模式
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 500;//输入CCR（占空比数值）
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM3, ENABLE); //ARR自动装载默认是打开的，可以不设置
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}

void vRightPWM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    TIM_DeInit(TIM14);
    
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_TIM14);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_TimeBaseStructure.TIM_Period = 1000;		    //1ms(周期)
	TIM_TimeBaseStructure.TIM_Prescaler = 10000;		//定时器分频系数，定时器时钟为82MHz，分频后得1MHz,即1000kHz,0.001ms定时器加1
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM14, &TIM_TimeBaseStructure);

	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//PWM1为正常占空比模式，PWM2为反极性模式
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 500;//输入CCR（占空比数值）
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC1Init(TIM14, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM14, ENABLE); //ARR自动装载默认是打开的，可以不设置
	TIM_ClearFlag(TIM14, TIM_FLAG_Update);
	TIM_ITConfig(TIM14, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM14, ENABLE);
}

void vMotor_Init()
{
    vMotor_GPIO_Init();
    vLeftPWM_Init();
    vRightPWM_Init();
    
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_LOW();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_LOW();
    TIM_SetCompare1(TIM3, 200);
    TIM_SetCompare1(TIM14, 300);
}

void Motor_Forword()
{
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_HIGH();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_HIGH();
}

void Motor_Backword()
{
    __LEFT_MOTOR_IN1_HIGH();
    __LEFT_MOTOR_IN2_LOW();
    __LEFT_MOTOR_IN3_HIGH();
    __LEFT_MOTOR_IN4_LOW();
}

void Motor_Left()
{
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_HIGH();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_HIGH();
}

void Motor_Right()
{
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_HIGH();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_HIGH();
}

void Motor_Forword_Left()
{
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_LOW();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_HIGH();
}

void Motor_Forword_Right()
{
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_HIGH();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_LOW();
}

void Motor_Backword_Left()
{
    __LEFT_MOTOR_IN1_LOW();
    __LEFT_MOTOR_IN2_LOW();
    __LEFT_MOTOR_IN3_HIGH();
    __LEFT_MOTOR_IN4_LOW();
}

void Motor_Backword_Right()
{
    __LEFT_MOTOR_IN1_HIGH();
    __LEFT_MOTOR_IN2_LOW();
    __LEFT_MOTOR_IN3_LOW();
    __LEFT_MOTOR_IN4_LOW();
}

void Motor_Stop()
{
    __LEFT_MOTOR_IN1_HIGH();
    __LEFT_MOTOR_IN2_HIGH();
    __LEFT_MOTOR_IN3_HIGH();
    __LEFT_MOTOR_IN4_HIGH();
}
