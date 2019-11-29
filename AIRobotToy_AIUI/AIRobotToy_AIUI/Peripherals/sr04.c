#include "sr04.h"
#include "syslog.h"
#include <usart.h>

void SR04_Trig_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOC时钟

  //GPIOF9,F10初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化
}

void SR04_Echo_Init(u32 arr, u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  	//TIM5时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//使能PORTA时钟	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; //GPIOA0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; //下拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA0

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_TIM5); //PA0复用位定时器5
  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseStructure);

	//初始化TIM5输入捕获参数
    TIM_ICInitTypeDef  TIM5_ICInitStructure;
	TIM5_ICInitStructure.TIM_Channel = TIM_Channel_1; //CC1S=01 	选择输入端 IC1映射到TI1上
    TIM5_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	//上升沿捕获
    TIM5_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //映射到TI1上
    TIM5_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //配置输入分频,不分频 
    TIM5_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000 配置输入滤波器 不滤波
    TIM_ICInit(TIM5, &TIM5_ICInitStructure);
		
	TIM_ITConfig(TIM5,TIM_IT_Update|TIM_IT_CC1,ENABLE);//允许更新中断 ,允许CC1IE捕获中断	
	
    TIM_Cmd(TIM5,ENABLE ); 	//使能定时器5
 
    NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
}

//捕获状态
//[7]:0,没有成功的捕获;1,成功捕获到一次.
//[6]:0,还没捕获到低电平;1,已经捕获到低电平了.
//[5:0]:捕获低电平后溢出的次数(对于32位定时器来说,1us计数器加1,溢出时间:4294秒)
u8  TIM5CH1_CAPTURE_STA=0;	//输入捕获状态
u32	TIM5CH1_CAPTURE_VAL;	//输入捕获值(TIM2/TIM5是32位)
//定时器5中断服务程序	 
void TIM5_IRQHandler(void)
{ 		    

 	if((TIM5CH1_CAPTURE_STA&0X80)==0)//还未成功捕获	
	{
		if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)//溢出
		{	     
			if(TIM5CH1_CAPTURE_STA&0X40)//已经捕获到高电平了
			{
				if((TIM5CH1_CAPTURE_STA&0X3F)==0X3F)//高电平太长了
				{
					TIM5CH1_CAPTURE_STA|=0X80;		//标记成功捕获了一次
					TIM5CH1_CAPTURE_VAL=0XFFFFFFFF;
				}else TIM5CH1_CAPTURE_STA++;
			}	 
		}
		if(TIM_GetITStatus(TIM5, TIM_IT_CC1) != RESET)//捕获1发生捕获事件
		{	
			if(TIM5CH1_CAPTURE_STA&0X40)		//捕获到一个下降沿 		
			{	  			
				TIM5CH1_CAPTURE_STA|=0X80;		//标记成功捕获到一次高电平脉宽
			  TIM5CH1_CAPTURE_VAL=TIM_GetCapture1(TIM5);//获取当前的捕获值.
	 			TIM_OC1PolarityConfig(TIM5,TIM_ICPolarity_Rising); //CC1P=0 设置为上升沿捕获
			}else  								//还未开始,第一次捕获上升沿
			{
				TIM5CH1_CAPTURE_STA=0;			//清空
				TIM5CH1_CAPTURE_VAL=0;
				TIM5CH1_CAPTURE_STA|=0X40;		//标记捕获到了上升沿
				TIM_Cmd(TIM5,DISABLE ); 	//关闭定时器5
	 			TIM_SetCounter(TIM5,0);
	 			TIM_OC1PolarityConfig(TIM5,TIM_ICPolarity_Falling);		//CC1P=1 设置为下降沿捕获
				TIM_Cmd(TIM5,ENABLE ); 	//使能定时器5
			}		    
		}			     	    					   
 	}
	TIM_ClearITPendingBit(TIM5, TIM_IT_CC1|TIM_IT_Update); //清除中断标志位
}

void vSR04_Init()
{
    SR04_Echo_Init(0XFFFFFFFF,84-1);    //以1Mhz的频率计数
    SR04_Trig_Init();
}

void UltrasonicWave_StartMeasure(void)
{
	__TRIG_HIGH();
	vTaskDelay(20);                     //延时20US
	__TRIG_LOW();
}

TimerHandle_t xTimers = NULL;
void vSR04TimerCallback(TimerHandle_t xTimer)
{
    u32 time = 0; 
    float UltrasonicWave_Distance=0;
    UltrasonicWave_StartMeasure();//开始测距
    if(TIM5CH1_CAPTURE_STA&0X80)//成功捕获到了一次上升沿
    {
        time=TIM5CH1_CAPTURE_STA&0X3F;
        time*=65536;//溢出时间总和
        time+=TIM5CH1_CAPTURE_VAL;//得到总的高电平时间

        UltrasonicWave_Distance=(float)time*170/10000;//距离计算公式：距离=高电平持续时间*声速/2   
                                                      //即 距离= 高电平持续时间*340*100/（2*1000 000） = 高电平持续时间*170/10000
        if (UltrasonicWave_Distance < 10.0f && UltrasonicWave_Distance > 1.0f)
        {
            ucUsart3_PutChar('s', 100);
        }
        ts_printf("Distance:%f cm\r\n",UltrasonicWave_Distance);//打印总的高电平时间
        TIM5CH1_CAPTURE_STA=0;//开启下一次捕获
    }
}

void sr04_timer(unsigned char start)
{
    if (start)
    {
        if (!xTimers)
            xTimers = xTimerCreate("sr04", pdMS_TO_TICKS(300), pdTRUE, (void *)0, vSR04TimerCallback);
        if (xTimers)
            xTimerStart(xTimers, 0);
    }
    else
    {
        if (xTimers)
        {
            xTimerStop(xTimers, 0);
            xTimerDelete(xTimers,100);
            xTimers = NULL;
        }
    }
}

void vSR04_Task(void *pvParameters)
{
    u32 time=0; 
    float UltrasonicWave_Distance=0;
    
    while(1) 
    {
        UltrasonicWave_StartMeasure();//开始测距
        if(TIM5CH1_CAPTURE_STA&0X80)//成功捕获到了一次上升沿
        {
            time=TIM5CH1_CAPTURE_STA&0X3F;
            time*=65536;//溢出时间总和
            time+=TIM5CH1_CAPTURE_VAL;//得到总的高电平时间

            UltrasonicWave_Distance=(float)time*170/10000;//距离计算公式：距离=高电平持续时间*声速/2   
                                                          //即 距离= 高电平持续时间*340*100/（2*1000 000） = 高电平持续时间*170/10000
            if (UltrasonicWave_Distance < 10.0f && UltrasonicWave_Distance > 1.0f)
            {
                ucUsart3_PutChar('s', 100);
            }
            ts_printf("Distance:%f cm\r\n",UltrasonicWave_Distance);//打印总的高电平时间
            TIM5CH1_CAPTURE_STA=0;//开启下一次捕获
        }
        vTaskDelay(500);
    }
}
