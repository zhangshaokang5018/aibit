#include "WM8978.h"
#include "soft_iic.h"
#include "myiic.h"

#if WM8978_USE_SOFT_IIC
/**
 *由于WM8978的IIC接口不支持读取操作，因此寄存器值缓存在内存中当写寄存器时同步更新缓存，
 *当你想读寄存器时直接返回缓存中的值。
*/
static uint16_t WM8978_RegCash[] =
{
	0x0000,0x0000,0x0000,0x0000,0x0050,0x0000,0x0140,0x0000,
	0x0000,0x0000,0x0000,0x00FF,0x00FF,0x0000,0x0100,0x00FF,
	0x00FF,0x0000,0x012C,0x002C,0x002C,0x002C,0x002C,0x0000,
	0x0032,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0038,0x000B,0x0032,0x0000,0x0008,0x000C,0x0093,0x00E9,
	0x0000,0x0000,0x0000,0x0000,0x0003,0x0010,0x0010,0x0100,
	0x0100,0x0002,0x0001,0x0001,0x0039,0x0039,0x0039,0x0039,
	0x0001,0x0001
};

uint16_t WM8978_ReadReg(uint8_t RegAddr)
{
	return WM8978_RegCash[RegAddr];
}

uint8_t WM8978_WriteReg(uint8_t RegAddr, uint16_t Value)
{
	//时钟线高电平时拉低数据线，启动IIC传输，然后拉低时钟线
	vSoftIIC_Start();
	//发送IIC从机地址，最后一位是0，表示写入数据
	if (xSoftIIC_WriteByte(WM8978_ADDR | IIC_WR))
	{
		return 1;
	}
	//地址7bit 指令9bit，先发送最高的第一个bit
	if (xSoftIIC_WriteByte(((RegAddr << 1) & 0xFE) | ((Value >> 8) & 0x1)))
	{
		return 1;
	}
	if (xSoftIIC_WriteByte(Value & 0xFF))
	{
		return 1;
	}
	vSoftIIC_Stop();
	//更新内存中的缓存
	WM8978_RegCash[RegAddr] = Value;
	return 0;
}

void WM8978_SetVolume(uint8_t Volume)
{
	uint16_t regV;
	
	Volume &= 0x3F;
	regV = Volume;
	//耳机左右声道音量设置
	WM8978_WriteReg(52, regV);
	WM8978_WriteReg(53, regV|(1<<8));//同步更新(SPKVU=1)
	//喇叭左右声道音量设置
	WM8978_WriteReg(54, regV);
	WM8978_WriteReg(55, regV|(1<<8));
}

uint8_t WM8978_ReadVolume(void)
{
	return (uint8_t)(WM8978_ReadReg(52)&0x3F);
}

void WM8978_MIC_Gain(uint8_t Gain)
{
	Gain &= 0x3F;
	WM8978_WriteReg(45, Gain);
	WM8978_WriteReg(46, Gain|(1<<8));
}
/**
 *设置I2S工作模式
 *fmt:0,LSB(右对齐);1,MSB(左对齐);2,飞利浦标准I2S;3,PCM/DSP;
 *len:0,16位;1,20位;2,24位;3,32位;
*/
void WM8978_I2S_Cfg(uint8_t fmt,uint8_t len)
{
	//限定范围
	fmt &= 0x03;
	len &= 0x03;
	//R4,WM8978工作模式设置
	WM8978_WriteReg(4,(fmt<<3)|(len<<5));
}

/**
 *WM8978 DAC/ADC配置
 *adcen:adc使能(1)/关闭(0)
 *dacen:dac使能(1)/关闭(0)
*/
void WM8978_ADDA_Cfg(uint8_t dacen,uint8_t adcen)
{
	uint16_t regval;
	
	regval=WM8978_ReadReg(3);
	if(dacen)
	{
		//R3最低2个位设置为1,开启DACR&DACL
		regval|=3<<0;
	}
	else
	{
		//R3最低2个位清零,关闭DACR&DACL.
		regval&=~(3<<0);
	}
	WM8978_WriteReg(3,regval);
	regval=WM8978_ReadReg(2);
	if(adcen)
	{
		//R2最低2个位设置为1,开启ADCR&ADCL
		regval|=3<<0;
	}
	else
	{
		//R2最低2个位清零,关闭ADCR&ADCL.
		regval&=~(3<<0);
	}
	WM8978_WriteReg(2,regval);
}

/**
 *WM8978 L2/R2(也就是Line In)增益设置(L2/R2-->ADC输入部分的增益)
 *gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
*/
void WM8978_LINEIN_Gain(uint8_t gain)
{
	uint16_t regval;
	
	gain&=0X07;
	regval=WM8978_ReadReg(47);
	//清除原来的设置
	regval&=~(7<<4);
	WM8978_WriteReg(47,regval|gain<<4);
	regval=WM8978_ReadReg(48);
	//清除原来的设置
	regval&=~(7<<4);
	WM8978_WriteReg(48,regval|gain<<4);
}

/**
 *WM8978 AUXR,AUXL(PWM音频部分)增益设置(AUXR/L-->ADC输入部分的增益)
 *gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
*/
void WM8978_AUX_Gain(uint8_t gain)
{
	uint16_t regval;
	
	gain&=0X07;
	regval=WM8978_ReadReg(47);
	//清除原来的设置
	regval&=~(7<<0);
	WM8978_WriteReg(47,regval|gain<<0);	
	regval=WM8978_ReadReg(48);
	//清除原来的设置
	regval&=~(7<<0);
	WM8978_WriteReg(48,regval|gain<<0);
}

/*
 *WM8978 输入通道配置
 *micen:MIC开启(1)/关闭(0)
 *lineinen:Line In开启(1)/关闭(0)
 *auxen:aux开启(1)/关闭(0)
*/
void WM8978_Input_Cfg(uint8_t micen,uint8_t lineinen,uint8_t auxen)
{
	uint16_t regval;
	
	regval=WM8978_ReadReg(2);
	if(micen)
	{
		//开启INPPGAENR,INPPGAENL(MIC的PGA放大)
		regval|=3<<2;
	}
	else
	{
		//关闭INPPGAENR,INPPGAENL.
		regval&=~(3<<2);
	}
	WM8978_WriteReg(2,regval);
	regval=WM8978_ReadReg(44);
	if(micen)
	{
		//开启LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
		regval|=3<<4|3<<0;
	}
	else
	{
		//关闭LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
		regval&=~(3<<4|3<<0);
	}
	WM8978_WriteReg(44,regval);
	if(lineinen)
	{
		//LINE IN 0dB增益
		WM8978_LINEIN_Gain(5);
	}
	else
	{
		//关闭LINE IN
		WM8978_LINEIN_Gain(0);
	}
	if(auxen)
	{
		//AUX 6dB增益
		WM8978_AUX_Gain(7);
	}
	else
	{
		//关闭AUX输入
		WM8978_AUX_Gain(0);
	}
}

/**
 *WM8978 输出配置
 *dacen:DAC输出(放音)开启(1)/关闭(0)
 *bpsen:Bypass输出(录音,包括MIC,LINE IN,AUX等)开启(1)/关闭(0)
*/
void WM8978_Output_Cfg(uint8_t dacen,uint8_t bpsen)
{
	uint16_t regval=0;
	if(dacen)
	{
		//DAC输出使能
		regval|=1<<0;
	}
	if(bpsen)
	{
		//BYPASS使能
		regval|=1<<1;
		//0dB增益
		regval|=5<<2;
	}
	WM8978_WriteReg(50,regval);
	WM8978_WriteReg(51,regval);
}

/**
 *设置耳机左右声道音量
 *voll:左声道音量(0~63)
 *volr:右声道音量(0~63)
*/
void WM8978_HPvol_Set(uint8_t voll,uint8_t volr)
{
	//限定范围
	voll&=0X3F;
	volr&=0X3F;
	//音量为0时,直接mute
	if(voll==0)
	{
		voll|=1<<6;
	}
	//音量为0时,直接mute
	if(volr==0)
	{
		volr|=1<<6;
	}
	//R52,耳机左声道音量设置
	WM8978_WriteReg(52,voll);
	//R53,耳机右声道音量设置,同步更新(HPVU=1)
	WM8978_WriteReg(53,volr|(1<<8));
}

/**
 *设置喇叭音量
 *voll:左声道音量(0~63)
*/
void WM8978_SPKvol_Set(uint8_t volx)
{
	//限定范围
	volx&=0x3F;
	
	//音量为0时,直接mute
	if(volx==0)volx|=1<<6;
	//R54,喇叭左声道音量设置
	WM8978_WriteReg(54,volx);
	//R55,喇叭右声道音量设置,同步更新(SPKVU=1)
	WM8978_WriteReg(55,volx|(1<<8));
}

void WM8978_PlayMode(void)
{
	WM8978_ADDA_Cfg(1,0);		//开启DAC
	WM8978_Input_Cfg(0,0,0);	//关闭输入通道
	WM8978_Output_Cfg(1,0);		//开启DAC输出
	WM8978_HPvol_Set(20,20);
	WM8978_SPKvol_Set(0x3F);
	WM8978_MIC_Gain(0);
	WM8978_I2S_Cfg(2,0);		//设置I2S接口模式
}

void WM8978_RecoMode(void)
{
	WM8978_ADDA_Cfg(0,1);		//开启ADC
	WM8978_Input_Cfg(1,0,0);	//开启输入通道(MIC)
	WM8978_Output_Cfg(0,1);		//开启BYPASS输出
	WM8978_HPvol_Set(0,0);
	WM8978_SPKvol_Set(0);
	WM8978_MIC_Gain(50);		//MIC增益设置
	WM8978_I2S_Cfg(2,0);		//飞利浦标准,16位数据长度
}
#else
static u16 WM8978_REGVAL_TBL[58]=
{
	0X0000,0X0000,0X0000,0X0000,0X0050,0X0000,0X0140,0X0000,
	0X0000,0X0000,0X0000,0X00FF,0X00FF,0X0000,0X0100,0X00FF,
	0X00FF,0X0000,0X012C,0X002C,0X002C,0X002C,0X002C,0X0000,
	0X0032,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,
	0X0038,0X000B,0X0032,0X0000,0X0008,0X000C,0X0093,0X00E9,
	0X0000,0X0000,0X0000,0X0000,0X0003,0X0010,0X0010,0X0100,
	0X0100,0X0002,0X0001,0X0001,0X0039,0X0039,0X0039,0X0039,
	0X0001,0X0001
};
u8 WM8978_Write_Reg(u8 reg,u16 val)
{ 
	IIC_Start(); 
	IIC_Send_Byte((WM8978_ADDR<<1)|0);//·¢ËÍÆ÷¼þµØÖ·+Ð´ÃüÁî	 
	if(IIC_Wait_Ack())return 1;	//µÈ´ýÓ¦´ð(³É¹¦?/Ê§°Ü?) 
    IIC_Send_Byte((reg<<1)|((val>>8)&0X01));//Ð´¼Ä´æÆ÷µØÖ·+Êý¾ÝµÄ×î¸ßÎ»
	if(IIC_Wait_Ack())return 2;	//µÈ´ýÓ¦´ð(³É¹¦?/Ê§°Ü?) 
	IIC_Send_Byte(val&0XFF);	//·¢ËÍÊý¾Ý
	if(IIC_Wait_Ack())return 3;	//µÈ´ýÓ¦´ð(³É¹¦?/Ê§°Ü?) 
    IIC_Stop();
	WM8978_REGVAL_TBL[reg]=val;	//±£´æ¼Ä´æÆ÷Öµµ½±¾µØ
	return 0;	
}  
//WM8978¶Á¼Ä´æÆ÷
//¾ÍÊÇ¶ÁÈ¡±¾µØ¼Ä´æÆ÷Öµ»º³åÇøÄÚµÄ¶ÔÓ¦Öµ
//reg:¼Ä´æÆ÷µØÖ· 
//·µ»ØÖµ:¼Ä´æÆ÷Öµ
u16 WM8978_Read_Reg(u8 reg)
{  
	return WM8978_REGVAL_TBL[reg];	
} 
//WM8978 DAC/ADCÅäÖÃ
//adcen:adcÊ¹ÄÜ(1)/¹Ø±Õ(0)
//dacen:dacÊ¹ÄÜ(1)/¹Ø±Õ(0)
void WM8978_ADDA_Cfg(u8 dacen,u8 adcen)
{
	u16 regval;
	regval=WM8978_Read_Reg(3);	//¶ÁÈ¡R3
	if(dacen)regval|=3<<0;		//R3×îµÍ2¸öÎ»ÉèÖÃÎª1,¿ªÆôDACR&DACL
	else regval&=~(3<<0);		//R3×îµÍ2¸öÎ»ÇåÁã,¹Ø±ÕDACR&DACL.
	WM8978_Write_Reg(3,regval);	//ÉèÖÃR3
	regval=WM8978_Read_Reg(2);	//¶ÁÈ¡R2
	if(adcen)regval|=3<<0;		//R2×îµÍ2¸öÎ»ÉèÖÃÎª1,¿ªÆôADCR&ADCL
	else regval&=~(3<<0);		//R2×îµÍ2¸öÎ»ÇåÁã,¹Ø±ÕADCR&ADCL.
	WM8978_Write_Reg(2,regval);	//ÉèÖÃR2	
}
//WM8978 ÊäÈëÍ¨µÀÅäÖÃ 
//micen:MIC¿ªÆô(1)/¹Ø±Õ(0)
//lineinen:Line In¿ªÆô(1)/¹Ø±Õ(0)
//auxen:aux¿ªÆô(1)/¹Ø±Õ(0) 
void WM8978_Input_Cfg(u8 micen,u8 lineinen,u8 auxen)
{
	u16 regval;  
	regval=WM8978_Read_Reg(2);	//¶ÁÈ¡R2
	if(micen)regval|=3<<2;		//¿ªÆôINPPGAENR,INPPGAENL(MICµÄPGA·Å´ó)
	else regval&=~(3<<2);		//¹Ø±ÕINPPGAENR,INPPGAENL.
 	WM8978_Write_Reg(2,regval);	//ÉèÖÃR2 
	
	regval=WM8978_Read_Reg(44);	//¶ÁÈ¡R44
	if(micen)regval|=3<<4|3<<0;	//¿ªÆôLIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
	else regval&=~(3<<4|3<<0);	//¹Ø±ÕLIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
	WM8978_Write_Reg(44,regval);//ÉèÖÃR44
	
	if(lineinen)WM8978_LINEIN_Gain(5);//LINE IN 0dBÔöÒæ
	else WM8978_LINEIN_Gain(0);	//¹Ø±ÕLINE IN
	if(auxen)WM8978_AUX_Gain(7);//AUX 6dBÔöÒæ
	else WM8978_AUX_Gain(0);	//¹Ø±ÕAUXÊäÈë  
}
//WM8978 Êä³öÅäÖÃ 
//dacen:DACÊä³ö(·ÅÒô)¿ªÆô(1)/¹Ø±Õ(0)
//bpsen:BypassÊä³ö(Â¼Òô,°üÀ¨MIC,LINE IN,AUXµÈ)¿ªÆô(1)/¹Ø±Õ(0) 
void WM8978_Output_Cfg(u8 dacen,u8 bpsen)
{
	u16 regval=0;
	if(dacen)regval|=1<<0;	//DACÊä³öÊ¹ÄÜ
	if(bpsen)
	{
		regval|=1<<1;		//BYPASSÊ¹ÄÜ
		regval|=5<<2;		//0dBÔöÒæ
	} 
	WM8978_Write_Reg(50,regval);//R50ÉèÖÃ
	WM8978_Write_Reg(51,regval);//R51ÉèÖÃ 
}
//WM8978 MICÔöÒæÉèÖÃ(²»°üÀ¨BOOSTµÄ20dB,MIC-->ADCÊäÈë²¿·ÖµÄÔöÒæ)
//gain:0~63,¶ÔÓ¦-12dB~35.25dB,0.75dB/Step
void WM8978_MIC_Gain(u8 gain)
{
	gain&=0X3F;
	WM8978_Write_Reg(45,gain);		//R45,×óÍ¨µÀPGAÉèÖÃ 
	WM8978_Write_Reg(46,gain|1<<8);	//R46,ÓÒÍ¨µÀPGAÉèÖÃ
}
//WM8978 L2/R2(Ò²¾ÍÊÇLine In)ÔöÒæÉèÖÃ(L2/R2-->ADCÊäÈë²¿·ÖµÄÔöÒæ)
//gain:0~7,0±íÊ¾Í¨µÀ½ûÖ¹,1~7,¶ÔÓ¦-12dB~6dB,3dB/Step
void WM8978_LINEIN_Gain(u8 gain)
{
	u16 regval;
	gain&=0X07;
	regval=WM8978_Read_Reg(47);	//¶ÁÈ¡R47
	regval&=~(7<<4);			//Çå³ýÔ­À´µÄÉèÖÃ 
 	WM8978_Write_Reg(47,regval|gain<<4);//ÉèÖÃR47
	regval=WM8978_Read_Reg(48);	//¶ÁÈ¡R48
	regval&=~(7<<4);			//Çå³ýÔ­À´µÄÉèÖÃ 
 	WM8978_Write_Reg(48,regval|gain<<4);//ÉèÖÃR48
} 
//WM8978 AUXR,AUXL(PWMÒôÆµ²¿·Ö)ÔöÒæÉèÖÃ(AUXR/L-->ADCÊäÈë²¿·ÖµÄÔöÒæ)
//gain:0~7,0±íÊ¾Í¨µÀ½ûÖ¹,1~7,¶ÔÓ¦-12dB~6dB,3dB/Step
void WM8978_AUX_Gain(u8 gain)
{
	u16 regval;
	gain&=0X07;
	regval=WM8978_Read_Reg(47);	//¶ÁÈ¡R47
	regval&=~(7<<0);			//Çå³ýÔ­À´µÄÉèÖÃ 
 	WM8978_Write_Reg(47,regval|gain<<0);//ÉèÖÃR47
	regval=WM8978_Read_Reg(48);	//¶ÁÈ¡R48
	regval&=~(7<<0);			//Çå³ýÔ­À´µÄÉèÖÃ 
 	WM8978_Write_Reg(48,regval|gain<<0);//ÉèÖÃR48
}  
//ÉèÖÃI2S¹¤×÷Ä£Ê½
//fmt:0,LSB(ÓÒ¶ÔÆë);1,MSB(×ó¶ÔÆë);2,·ÉÀûÆÖ±ê×¼I2S;3,PCM/DSP;
//len:0,16Î»;1,20Î»;2,24Î»;3,32Î»;  
void WM8978_I2S_Cfg(u8 fmt,u8 len)
{
	fmt&=0X03;
	len&=0X03;//ÏÞ¶¨·¶Î§
	WM8978_Write_Reg(4,(fmt<<3)|(len<<5));	//R4,WM8978¹¤×÷Ä£Ê½ÉèÖÃ	
}	

//ÉèÖÃ¶ú»ú×óÓÒÉùµÀÒôÁ¿
//voll:×óÉùµÀÒôÁ¿(0~63)
//volr:ÓÒÉùµÀÒôÁ¿(0~63)
void WM8978_HPvol_Set(u8 voll,u8 volr)
{
	voll&=0X3F;
	volr&=0X3F;//ÏÞ¶¨·¶Î§
	if(voll==0)voll|=1<<6;//ÒôÁ¿Îª0Ê±,Ö±½Ómute
	if(volr==0)volr|=1<<6;//ÒôÁ¿Îª0Ê±,Ö±½Ómute 
	WM8978_Write_Reg(52,voll);			//R52,¶ú»ú×óÉùµÀÒôÁ¿ÉèÖÃ
	WM8978_Write_Reg(53,volr|(1<<8));	//R53,¶ú»úÓÒÉùµÀÒôÁ¿ÉèÖÃ,Í¬²½¸üÐÂ(HPVU=1)
}
//ÉèÖÃÀ®°ÈÒôÁ¿
//voll:×óÉùµÀÒôÁ¿(0~63) 
void WM8978_SPKvol_Set(u8 volx)
{ 
	volx&=0X3F;//ÏÞ¶¨·¶Î§
	if(volx==0)volx|=1<<6;//ÒôÁ¿Îª0Ê±,Ö±½Ómute 
 	WM8978_Write_Reg(54,volx);			//R54,À®°È×óÉùµÀÒôÁ¿ÉèÖÃ
	WM8978_Write_Reg(55,volx|(1<<8));	//R55,À®°ÈÓÒÉùµÀÒôÁ¿ÉèÖÃ,Í¬²½¸üÐÂ(SPKVU=1)	
}
//ÉèÖÃ3D»·ÈÆÉù
//depth:0~15(3DÇ¿¶È,0×îÈõ,15×îÇ¿)
void WM8978_3D_Set(u8 depth)
{ 
	depth&=0XF;//ÏÞ¶¨·¶Î§ 
 	WM8978_Write_Reg(41,depth);	//R41,3D»·ÈÆÉèÖÃ 	
}
//ÉèÖÃEQ/3D×÷ÓÃ·½Ïò
//dir:0,ÔÚADCÆð×÷ÓÃ
//    1,ÔÚDACÆð×÷ÓÃ(Ä¬ÈÏ)
void WM8978_EQ_3D_Dir(u8 dir)
{
	u16 regval; 
	regval=WM8978_Read_Reg(0X12);
	if(dir)regval|=1<<8;
	else regval&=~(1<<8); 
 	WM8978_Write_Reg(18,regval);//R18,EQ1µÄµÚ9Î»¿ØÖÆEQ/3D·½Ïò
}

//ÉèÖÃEQ1
//cfreq:½ØÖ¹ÆµÂÊ,0~3,·Ö±ð¶ÔÓ¦:80/105/135/175Hz
//gain:ÔöÒæ,0~24,¶ÔÓ¦-12~+12dB
void WM8978_EQ1_Set(u8 cfreq,u8 gain)
{ 
	u16 regval;
	cfreq&=0X3;//ÏÞ¶¨·¶Î§ 
	if(gain>24)gain=24;
	gain=24-gain;
	regval=WM8978_Read_Reg(18);
	regval&=0X100;
	regval|=cfreq<<5;	//ÉèÖÃ½ØÖ¹ÆµÂÊ 
	regval|=gain;		//ÉèÖÃÔöÒæ	
 	WM8978_Write_Reg(18,regval);//R18,EQ1ÉèÖÃ 	
}
//ÉèÖÃEQ2
//cfreq:ÖÐÐÄÆµÂÊ,0~3,·Ö±ð¶ÔÓ¦:230/300/385/500Hz
//gain:ÔöÒæ,0~24,¶ÔÓ¦-12~+12dB
void WM8978_EQ2_Set(u8 cfreq,u8 gain)
{ 
	u16 regval=0;
	cfreq&=0X3;//ÏÞ¶¨·¶Î§ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//ÉèÖÃ½ØÖ¹ÆµÂÊ 
	regval|=gain;		//ÉèÖÃÔöÒæ	
 	WM8978_Write_Reg(19,regval);//R19,EQ2ÉèÖÃ 	
}
//ÉèÖÃEQ3
//cfreq:ÖÐÐÄÆµÂÊ,0~3,·Ö±ð¶ÔÓ¦:650/850/1100/1400Hz
//gain:ÔöÒæ,0~24,¶ÔÓ¦-12~+12dB
void WM8978_EQ3_Set(u8 cfreq,u8 gain)
{ 
	u16 regval=0;
	cfreq&=0X3;//ÏÞ¶¨·¶Î§ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//ÉèÖÃ½ØÖ¹ÆµÂÊ 
	regval|=gain;		//ÉèÖÃÔöÒæ	
 	WM8978_Write_Reg(20,regval);//R20,EQ3ÉèÖÃ 	
}
//ÉèÖÃEQ4
//cfreq:ÖÐÐÄÆµÂÊ,0~3,·Ö±ð¶ÔÓ¦:1800/2400/3200/4100Hz
//gain:ÔöÒæ,0~24,¶ÔÓ¦-12~+12dB
void WM8978_EQ4_Set(u8 cfreq,u8 gain)
{ 
	u16 regval=0;
	cfreq&=0X3;//ÏÞ¶¨·¶Î§ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//ÉèÖÃ½ØÖ¹ÆµÂÊ 
	regval|=gain;		//ÉèÖÃÔöÒæ	
 	WM8978_Write_Reg(21,regval);//R21,EQ4ÉèÖÃ 	
}
//ÉèÖÃEQ5
//cfreq:ÖÐÐÄÆµÂÊ,0~3,·Ö±ð¶ÔÓ¦:5300/6900/9000/11700Hz
//gain:ÔöÒæ,0~24,¶ÔÓ¦-12~+12dB
void WM8978_EQ5_Set(u8 cfreq,u8 gain)
{ 
	u16 regval=0;
	cfreq&=0X3;//ÏÞ¶¨·¶Î§ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//ÉèÖÃ½ØÖ¹ÆµÂÊ 
	regval|=gain;		//ÉèÖÃÔöÒæ	
 	WM8978_Write_Reg(22,regval);//R22,EQ5ÉèÖÃ 	
}
#endif

uint8_t WM8978_Init(void)
{
	uint8_t ucFun_res = 0;
	
#if WM8978_USE_SOFT_IIC
	vSoftIIC_Init();
	ucFun_res |= WM8978_WriteReg(0,0x0000);	//向0寄存器写入任意值，就可以软件复位
	//先把声音关到最小，防止噪音
	WM8978_HPvol_Set(40,40);
	WM8978_SPKvol_Set(50);
	ucFun_res |= WM8978_WriteReg(1,0x001B);	//R1,MICEN设置为1(MIC使能),BIASEN设置为1(模拟器工作),VMIDSEL[1:0]设置为:11(5K)
	ucFun_res |= WM8978_WriteReg(2,0x01B0);	//R2,ROUT1,LOUT1输出使能(耳机可以工作),BOOSTENR,BOOSTENL使能
	ucFun_res |= WM8978_WriteReg(3,0x006C);	//R3,LOUT2,ROUT2输出使能(喇叭工作),RMIX,LMIX使能
	ucFun_res |= WM8978_WriteReg(6,0x0000);	//R6,MCLK由外部提供

	ucFun_res |= WM8978_WriteReg(43,1<<4);	//R43,INVROUT2反向,驱动喇叭
	ucFun_res |= WM8978_WriteReg(47,1<<8);	//R47设置,PGABOOSTL,左通道MIC获得20倍增益
	ucFun_res |= WM8978_WriteReg(48,1<<8);	//R48设置,PGABOOSTR,右通道MIC获得20倍增益
	ucFun_res |= WM8978_WriteReg(49,1<<1);	//R49,TSDEN,开启过热保护
	ucFun_res |= WM8978_WriteReg(10,1<<3);	//R10,SOFTMUTE关闭,128x采样,最佳SNR
	ucFun_res |= WM8978_WriteReg(14,1<<3);	//R14,ADC 128x采样率
	
	return ucFun_res;
#else
    IIC_Init();
    ucFun_res = WM8978_Write_Reg(0,0);	//向0寄存器写入任意值，就可以软件复位
	if(ucFun_res)return 1;			
	WM8978_Write_Reg(1,0X1B);	        //R1,MICEN设置为1(MIC使能),BIASEN设置为1(模拟器工作),VMIDSEL[1:0]设置为:11(5K)
	WM8978_Write_Reg(2,0X1B0);	        //R2,ROUT1,LOUT1输出使能(耳机可以工作),BOOSTENR,BOOSTENL使能
	WM8978_Write_Reg(3,0X6C);	        //R3,LOUT2,ROUT2输出使能(喇叭工作),RMIX,LMIX使能
	WM8978_Write_Reg(6,0);		        //R6,MCLK由外部提供
	WM8978_Write_Reg(43,1<<4);	        //R43,INVROUT2反向,驱动喇叭
	WM8978_Write_Reg(47,1<<8);	        //R47设置,PGABOOSTL,左通道MIC获得20倍增益
	WM8978_Write_Reg(48,1<<8);	        //R48设置,PGABOOSTR,右通道MIC获得20倍增益
	WM8978_Write_Reg(49,1<<1);	        //R49,TSDEN,开启过热保护
	WM8978_Write_Reg(10,1<<3);	        //R10,SOFTMUTE关闭,128x采样,最佳SNR
	WM8978_Write_Reg(14,1<<3);	        //R14,ADC 128x采样率
	return 0;                   
#endif
}
