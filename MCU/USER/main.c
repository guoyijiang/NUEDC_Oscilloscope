#include "sys.h"
#include "misc.h"
#include "delay.h"
#include "usart.h"
#include "key.h"
#include "led.h"
#include "lcd.h"
#include "lcdpro.h"
#include "connectFPGA.h"
#include "touch.h" 
#include "spi.h"
#include "rlcmeasure.h"
#include "ui1.h"
#include "timer.h"
#include "osci.h"
#include "sram.h"
#include "malloc.h" 
#include "dac8811.h"
#include "sdio_sdcard.h" 
#include "ff.h"  
#include "exfuns.h" 


#define	DA8811CS 		PAout(15)  		//CS
#define ADS8866CS   PAout(15)  		//CS

int osci_DAC8811_SetVoltage(double voltage)
{
	//y = 0.9763711 x - 0.0635625 
	float x;
	// x= (y+0.0635625)/0.9763711
	
	//if((voltage < DAVREF)&&(voltage > -1.0 * DAVREF))
	x = (voltage+0.0635625)/0.9763711;
	
	Fpga_WriteReg(DAC8811REG,(int16_t)( (x/DAVREF +1)*32768.0));
	return 0;
}

int DA8811_Init()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOB时钟
//  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//使能GPIOG时钟

	//GPIOA15
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;//PA15
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化

//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;//PG7
//  GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化
 
//	GPIO_SetBits(GPIOG,GPIO_Pin_7);//PG7输出1,防止NRF干扰SPI FLASH的通信 
	
	DA8811CS=1;			//SPI FLASH不选中
	
	SPI3_Init();		   			//初始化SPI
	//SPI3_SetSpeed(SPI_BaudRatePrescaler_4);		//设置为21M时钟,高速模式 
	return 0;
}
void DAC8811_writereg(u16 DATA)
{
	DA8811CS = 0;
	SPI3_ReadWriteByte((u16)DATA);
	DA8811CS = 1;
	//delay_us(2);
}
//+-Vref
int DAC8811_SetVoltage(double voltage)
{
	//y = 0.9763711 x - 0.0635625 
	float x;
	// x= (y+0.0635625)/0.9763711
	
	//if((voltage < DAVREF)&&(voltage > -1.0 * DAVREF))
	x = (voltage+0.0635625)/0.9763711;
	
		DAC8811_writereg( (int16_t)( (x/DAVREF +1)*32768.0) );
	return 0;
}


int ADS8866_Init()
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOB时钟

	//GPIOA15
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;//PA15
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
	ADS8866CS=1;			//SPI FLASH不选中
	SPI3_Init();		   			//初始化SPI // 
	return 0;
}
u16 ADS8866_readreg()
{
	u16 temp;
	ADS8866CS = 0;
	delay_us(1);
	temp = SPI3_ReadWriteByte(0xaaaa);
	delay_us(1);
	ADS8866CS = 1;
	delay_us(10);
	return temp;
}
double ADS8866_SampleVoltage()
{

	 return (double)((u16)ADS8866_readreg())/65536.0 * ADVREF -2.0;
}

int testADDA_FPAG()
{
	
	u32 data;
	double u;
	
	data = 0x0000ffff & Fpga_ReadReg(16);
	u = ((double)data)/65536.0 * 4.5;
//	DebugPrintf("data= %x\tu= %x\r\n",data,u);
//	delay_ms(300);
	return 0;
}

char timer3ItFlag = 0;
//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
		timer3ItFlag = 1;
		LED1=!LED1;//DS1翻转
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}

int funtest()
{
	u32 temp;
	float v;
	temp = Fpga_ReadReg(12);
	v = (float)temp/65536.0 * ADVREF + VLIFT;
	DebugStatePrint(5,"READ REG12= %x    v= %f",temp,v);
	
	return 0;
}
int mainweek1(void)
{
	RLC_Measure RLC;
	KEY1STRUCT key1array[NKEY1];
	
	u32 temp;
	int keyvalue =0;
	int keyvaluetemp =0;

	//系统初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	uart_init(115200);
	TIM3_Int_Init(3000-1,8400-1); //0.1ms*3000 = 300ms
	
	LED_Init();
	LCD_Init();	
	tp_dev.init();
	
	POINT_COLOR=BLACK;
	BACK_COLOR = WHITE;
	
	KEY_Init();
	SPI1_Init();
	//DA8811_Init(); //2edge High 16fen
	//ADS8866_Init(); //1edge Low 256fen
	
	rlc_Initial(&RLC);
	
	
	DebugPrintf("Hello\r\n");
	Fpga_WriteReg(0,5000);
	Fpga_WriteReg(1,555);
	Fpga_WriteReg(1,666);
	Fpga_WriteReg(1,3259);
	temp = Fpga_ReadReg(11);
	DebugPrintf("Read REG11 = %d\r\n",temp);
	if(temp == 520) DebugPrintf("ConnectFPGA OK...\r\n");
	
	rlc_SetFreq(&RLC, 1000.0);
	temp = Fpga_ReadReg(2);
	DebugStatePrint(2,"Read REG2 = %d  %f",temp,temp * 0.0023283064365386962890625);
	
	
//	DebugPrintf("measure v = %f\r\n",ADS8866_SampleVoltage());

	
//	ADS8866_SampleVoltage();
//	ADS8866_SampleVoltage();

//	ADS8866_readreg();
//	ADS8866_readreg();
//	ADS8866_readreg();
//	ADS8866_readreg();

	keyBoard1_Generate(key1array);
	while(1)
	{
		keyvaluetemp = keyboard1_Input(key1array);	
		keyvalue = (keyvaluetemp == 0) ? 0:keyvaluetemp;
		if(timer3ItFlag)
		{
			funtest();
			DebugStatePrint(6,"touch:%d",keyvalue);
			if(keyvalue == 2)
				rlc_SetFreq(&RLC, 1000.0);
			else if(keyvalue == 3)
				rlc_SetFreq(&RLC, 10000.0);
			else if(keyvalue == 4)
				rlc_SetFreq(&RLC, 100000.0);
			keyvalue =0;
			//rlc_Measure(&RLC,(char)keyvalue);
			timer3ItFlag =0;
			
			temp = Fpga_ReadReg(2);
			DebugStatePrint(2,"Read REG2 = %d  %f",temp,temp * 0.0023283064365386962890625);
			
		}	
	}
	
}


int mainweek2(void)
{
	KEY1STRUCT key1array[NKEY1];
	
	u32 temp = 1;
	int keyvalue =0;
	int keyvaluetemp =0;
	char inputbuf[20];
	char *pinput;
	u16 paddr;

	//系统初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	uart_init(115200);
	TIM3_Int_Init(500-1,8400-1); //0.1ms*3000 = 50ms
	
	LED_Init();
	LCD_Init();	
	tp_dev.init();
	
	POINT_COLOR=BLACK;
	BACK_COLOR = WHITE;
	
	KEY_Init();
	SPI1_Init();
	DA8811_Init(); //2edge High 16fen
	//ADS8866_Init(); //1edge Low 256fen
	//rlc_Initial(&RLC);
	DebugPrintf("Hello World~\r\n");
	Fpga_WriteReg(0,520);
	temp = Fpga_ReadReg(0);
	//DebugPrintf("Read REG11 = %d\r\n",temp);
	if(temp == 520) DebugPrintf("Connect FPGA OK\r\n");
	else DebugPrintf("Connect FPGA Failed\r\n");

	DAC8811_SetVoltage(0);
	pinput = inputbuf;
	paddr =0;
	keyBoard1_Generate(key1array);
	
	DebugStatePrint(2,"Gain:");
	DebugStatePrint(3,"V = ");
	DebugStatePrint(4,"SET V:%f",0.0);
	while(1)
	{
		keyvaluetemp = keyboard1_Input(key1array);	
		keyvalue = (keyvaluetemp == 0) ? 0:keyvaluetemp;
		if(keyvalue != 0 ) temp = keyvalue;
		if(timer3ItFlag)
		{
			//funtest();
			if((keyvalue <=11)&&(keyvalue != 0))
			{
				DebugStatePrint(2,"Gain:%d",12 + (temp -1)*4 );
				DebugStatePrint(5,"touch:%d",keyvalue);
			}
			if(keyvalue == 2)//12
				DAC8811_SetVoltage(0.2);
			else if(keyvalue == 3)//16
				DAC8811_SetVoltage(0.4);
			else if(keyvalue == 3)//20
				DAC8811_SetVoltage(0.6);
			else if(keyvalue == 4)//24
				DAC8811_SetVoltage(0.8);
			else if(keyvalue == 5)//28
				DAC8811_SetVoltage(1.0);
			else if(keyvalue == 6)//32
				DAC8811_SetVoltage(-0.20);
			else if(keyvalue == 7)//36
				DAC8811_SetVoltage(-0.40);
			else if(keyvalue == 8)//40
				DAC8811_SetVoltage(-0.60);
			else if(keyvalue == 9)//44
				DAC8811_SetVoltage(-0.80);
			else if(keyvalue == 10)//48
				DAC8811_SetVoltage(-1.00);
			else if(keyvalue == 11)//52
				DAC8811_SetVoltage(0);
			else if(keyvalue == 12)
			{
				if(paddr>0) 
				{
					paddr--;
					inputbuf[paddr] = '\0';
					DebugStatePrint(3,"V = %s", inputbuf);
				}
				
			}
		if((keyvalue <= 22)&&(keyvalue >=13))
				if(paddr < 20-1) 
				{
					
					inputbuf[paddr++] = (keyvalue -13 + '0');
					inputbuf[paddr] = '\0';
					DebugStatePrint(3,"V = %s", inputbuf);
				}
		if(keyvalue == 23) 
				if(paddr < 20-1) 
				{
					inputbuf[paddr++] = '.';
					inputbuf[paddr] = '\0';
					DebugStatePrint(3,"V = %s", inputbuf);					
				}
		if(keyvalue == 24) 
			{
				double vtemp;
				inputbuf[paddr] = '\0';
				vtemp = atof(inputbuf);
				if((vtemp > -1.2)&&(vtemp < 1.2))
				{
					DAC8811_SetVoltage(vtemp);
					DebugStatePrint(4,"SET V:%f", vtemp);
				}
				paddr =0;
				inputbuf[paddr] = '\0';
				DebugStatePrint(3,"V = %s", inputbuf);
			}
		if(keyvalue == 25) 
				if(paddr < 20-1) 
				{
					inputbuf[paddr++] = '-';
					inputbuf[paddr] = '\0';
					DebugStatePrint(3,"V = %s", inputbuf);					
				}
			
			keyvalue =0;
			timer3ItFlag =0;
		}	
	}
	
	
	
}
int main(void)
{
	
	u32 temp;
	u8 touchKey =0;
	char* ptest;
	FIL *ftemp;
	
	int keyvalue =0;
	int keyvaluetemp =0;

	//系统初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init(168);
	uart_init(115200);
	
	FSMC_SRAM_Init();		
//	my_mem_init(SRAMIN);	
	my_mem_init(SRAMEX);		
//	my_mem_init(SRAMCCM);		
	
	SPI1_Init();
//	DA8811_Init();
	TIM3_Int_Init(10000-1,8400-1); //0.1ms*3000 = 1000ms
	LED_Init();
	LCD_Init();	
	LCD_Display_Dir(1);	
	
	tp_dev.init();
	KEY_Init();
	
	POINT_COLOR=WHITE;
	BACK_COLOR = BLACK;
	
	while(SD_Init())//检测不到SD卡
	{
		LCD_ShowString(30,150,200,16,16,"SD Card Error!");
		delay_ms(500);					
		LCD_ShowString(30,150,200,16,16,"Please Check! ");
		delay_ms(500);
		LED0=!LED0;//DS0闪烁
	}
	exfuns_init();							//为fatfs相关变量申请内存		
	f_mount(fs[0],"0:",1); 					//挂载SD卡 
	
	ftemp=(FIL*)mymalloc(SRAMIN,sizeof(FIL));
	
	osci_Init();
	osci_SetLinable(0);
	
	DebugPrintf("Hello from MCU\r\n");
	ptest = (char*)mymalloc(SRAMEX,sizeof(char)*60);
	if(ptest == NULL)
	{
		DebugPrintf("ini ptest wrong!\r\n");
		while(1);
	}
	strcpy(ptest,"Hello from SRAMEX\r\n");
	DebugPrintf(ptest);
	Fpga_WriteReg(0,520);
	temp = Fpga_ReadReg(0);
	if(temp == 520) DebugPrintf("Connect FPGA OK\r\n");
	else DebugPrintf("Connect FPGA Failed %d\r\n",temp);
	
	
	delay_ms(10);
	
	while(1)
	{
		touchKey = osci_KeyScan();
		osci_KeyDeal(touchKey);
		if(timer3ItFlag)
		{	
			osci_Read();
			delay_us(5);
			osci_Sample();
			osci_DrawData();
			timer3ItFlag =0;			
		}	
		
	}
	
}
