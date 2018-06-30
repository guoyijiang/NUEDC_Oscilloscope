#include "rlcmeasure.h"
#include "delay.h"

int rlc_Initial(RLC_Measure* RLC)
{
//初始化Swith档位选择IO
	GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	//GPIOD
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//
  GPIO_Init(GPIOD, &GPIO_InitStructure);
	//GPIOG
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//
  GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	RLC->Rs[0] = 4.0130;
	RLC->Rs[1] = 36.0090;
	RLC->Rs[2] = 360.783;
	RLC->Rs[3] = 3580.483;
	RLC->Rs[4] = 35992.483;
	RLC->Rs[5] = 360119.483;
	RLC->Rs[6] = 3276399.48;
	
	RLC->autMode = 0;
	RLC->freq = 1000.0f;
	
	SWITCH0 = 0;
	SWITCH1 = 0;
	SWITCH2 = 0;
	SWITCH3 = 0;
	SWITCH4 = 0;	
	SWITCH5 = 0;
	SWITCH6 = 0;
	return 0;
}
int rlc_SetFreq(RLC_Measure* RLC,double freq)
{
	u32 temp;
	RLC->freq = freq;
	//temp = (int)(freq*16.777216);
	temp = (u32)(freq*429.4967296);
	Fpga_WriteReg(AddrFreq,temp);
	return 0;
}

int rlc_MeasureEn(RLC_Measure* RLC)
{
	Fpga_WriteReg(AddrMeasureTick,0xff);
	return 0;
}
int rlc_GetResult(RLC_Measure* RLC)
{
	RLC->readDataUxx = Fpga_ReadReg(AddrRegUxx);
	RLC->readDataUxy = Fpga_ReadReg(AddrRegUxy);
	RLC->readDataUsx = Fpga_ReadReg(AddrRegUsx);
	RLC->readDataUsy = Fpga_ReadReg(AddrRegUsy);
	
	RLC->Uxx = ((double)((int)RLC->readDataUxx))/65536.0 * ADVREF + VLIFT;
	RLC->Uxy = ((double)((int)RLC->readDataUxy))/65536.0 * ADVREF + VLIFT;
	RLC->Usx = ((double)((int)RLC->readDataUsx))/65536.0 * ADVREF + VLIFT;
	RLC->Usy = ((double)((int)RLC->readDataUsy))/65536.0 * ADVREF + VLIFT;
	return 0;
}
int rlc_Calc(RLC_Measure* RLC)
{
	double father = .0f;
	
	father = ((RLC->Usx * RLC->Usx) + (RLC->Usy * RLC->Usy)) / RLC->Rs[RLC->RsState] * -1.0f;
	RLC->Zre = ((RLC->Uxx * RLC->Usx) + (RLC->Uxy * RLC->Usy))/father;
	RLC->Zim = ((RLC->Uxy * RLC->Usx) + (RLC->Uxx * RLC->Usy))/father;
	RLC->Zabs = sqrt((RLC->Zre * RLC->Zre)+(RLC->Zim * RLC->Zim));
	RLC->QD = fabs(RLC->Uxx / RLC->Uxy);
	if(RLC->Zre > RLC->Zim) RLC->feature = 'R';
		else if(RLC->Zim >0) RLC->feature = 'L';
			else RLC->feature = 'C';
	return 0;
}

int rlc_UpdataPlay(RLC_Measure* RLC)
{
	DebugStatePrint(0,"uxx= %f    uxy= %f",RLC->Uxx,RLC->Uxy);
	DebugStatePrint(1,"usx= %f    usy= %f",RLC->Usx,RLC->Usy);
	DebugStatePrint(2,"z= %f      zre= %f    zim=%f",RLC->Zabs,RLC->Zre,RLC->Zim);
	return 0;
}
int rlc_stopMeasure(RLC_Measure* RLC)
{
 return 0;
}

//return 1表示测量Z太小 2表示Z太大 3表示量程转换了
int rlc_ChangeRange(RLC_Measure* RLC)
{
	int i;
	int lastState = RLC->RsState;
	for(i=0;i<NRS;i++)
	{
		if(RLC->Zabs < RLC->Rs[i])
		{
			if((RLC->RsState - 1)>= 0)
				RLC->RsState = RLC->RsState-1;
			else	return 1;
		}
		else if(RLC->Zabs > RLC->Rs[i])
		{
			if((RLC->RsState + 1)< NRS)
				RLC->RsState = RLC->RsState+1;
			else
			{
				//rlc_stopMeasure(RLC);
				return 2;	
			}			
		}
	}
	if(lastState == RLC->RsState)
	{
		RLC->isIt = 1;
		return 0;
	}
	else 
	{
	 RLC->isIt = 0;
		if(RLC->RsState == 0) SWITCH0 = 1;
		else SWITCH0 = 0;
		if(RLC->RsState == 1) SWITCH1 = 1;
		else SWITCH1 = 0;
		if(RLC->RsState == 2) SWITCH2 = 1;
		else SWITCH2 = 0;
		if(RLC->RsState == 3) SWITCH3 = 1;
		else SWITCH3 = 0;
		if(RLC->RsState == 4) SWITCH4 = 1;
		else SWITCH4 = 0;
		if(RLC->RsState == 5) SWITCH5 = 1;
		else SWITCH5 = 0;
		if(RLC->RsState == 6) SWITCH6 = 1;
		else SWITCH6 = 0;		
	 return 3;
	}
}

int changeState(char target)
{
		if(target == 0) SWITCH0 = 1;
		else SWITCH0 = 0;
		if(target == 1) SWITCH1 = 1;
		else SWITCH1 = 0;
		if(target == 2) SWITCH2 = 1;
		else SWITCH2 = 0;
		if(target == 3) SWITCH3 = 1;
		else SWITCH3 = 0;
		if(target == 4) SWITCH4 = 1;
		else SWITCH4 = 0;
		if(target == 5) SWITCH5 = 1;
		else SWITCH5 = 0;
		if(target == 6) SWITCH6 = 1;
		else SWITCH6 = 0;		
		return 0;
}
int changeXS(char target) //0 Ux 1 Us
{
		if(target == 0) SWITCHUX = 1;
		else SWITCHUX = 0;
		if(target == 1) SWITCHUS = 1;
		else SWITCHUS = 0;	
		return 0;
	
}
//return 2auto 1freqchange 0keep
int rlc_ChangeFreq(RLC_Measure* RLC,char inputChar)
{
	if(RLC->isIt == 1) 
	{
		if(RLC->autMode == 1)
		{
			rlc_SetFreq(RLC, 10000.0f);
			RLC->autMode = 0;
			return 2;
		}
	  else if(inputChar == 1) RLC->autMode =1;
		return 2;	
	}
	else if(inputChar == 0) return 0;
	else if(inputChar == 1) //设定频率 A=auto F=1k G=10k H=100k
		RLC->autMode =1;
	else if(inputChar == 2)
		rlc_SetFreq(RLC, 1000.0);
	else if(inputChar == 3)
		rlc_SetFreq(RLC, 10000.0);
	else if(inputChar == 4)
		rlc_SetFreq(RLC, 100000.0);
	return 1;
	


}

int rlc_Measure(RLC_Measure* RLC,char inputChar)
{
	

	rlc_GetResult(RLC);//获取测量数据
	rlc_Calc(RLC);//计算结果
	rlc_UpdataPlay(RLC);//更新显示
	rlc_ChangeRange(RLC);//量程转换
	rlc_ChangeFreq(RLC,inputChar);//频率重设
	
	rlc_MeasureEn(RLC);//开始使能
	return 0;
}

//	DAC8811_SetVoltage(2.5f);
//	DAC8811_SetVoltage(2.4f);
//	DAC8811_SetVoltage(2.2f);
//	DAC8811_SetVoltage(2.0f);
//	DAC8811_SetVoltage(1.8f);
//	DAC8811_SetVoltage(1.5f);
//	DAC8811_SetVoltage(1.0f);
//	DAC8811_SetVoltage(0.8f);
//	DAC8811_SetVoltage(0.5f);
//	DAC8811_SetVoltage(0.2f);
//	DAC8811_SetVoltage(0.0f);
//	DAC8811_SetVoltage(-0.2f);
//	DAC8811_SetVoltage(-0.5f);
//	DAC8811_SetVoltage(-0.8f);
//	DAC8811_SetVoltage(-1.0f);
//	DAC8811_SetVoltage(-1.2f);
//	DAC8811_SetVoltage(-1.5f);
//	DAC8811_SetVoltage(-1.8f);
//	DAC8811_SetVoltage(-2.0f);
//	DAC8811_SetVoltage(-2.2f);
//	DAC8811_SetVoltage(-2.4f);
//	DAC8811_SetVoltage(-2.5f);
	
