#include "osci.h"
#include "lcd.h"
#include "lcdpro.h"
#include "connectFPGA.h"
#include "sram.h"
#include "malloc.h" 
#include "fft.h"


u8 osciPoint[OSCIDATALENGTH] ={0};
u8 osciLINE = 0;
u8 osciRUN =0;
u8 osciSINGLE = 0;
u8 osciSAVE =0;
u8 osciLOAD =0;
u8 osciFFT =0;
u8 vstateChange =0;

double osciCoefNow = COEFI1V0;
double osciVrangeNow = RANGE1V0;

double osciTriVolt = .0f;
double osciVolt =.0f;
double osciFreq =.0f;

double osciMulti[4];
double osciCorrect[4][4];

//double osciDisplayCoef = 1.0f;

_osci osci;
_oscipoint* osciPrintArrayP;
_oscipoint* osciPrintArrayPl;

u32 osciArryUsed;
u32 osciArrylUsed;
u8 vstate;
u8 tstate;
int osci_Init(void) 
{
	//IO口初始化
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	//GPIOD
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//
  GPIO_Init(GPIOD, &GPIO_InitStructure);	
	
	//初始化数据
	osciPrintArrayP = (_oscipoint*)mymalloc(SRAMEX,sizeof(_oscipoint)*OSCIARRAYLENGTH);
	if(osciPrintArrayP == NULL)
	{
		DebugPrintf("ini osciPrintArrayP wrong!\r\n");
		while(1);
	}
	osciPrintArrayPl =(_oscipoint*)mymalloc(SRAMEX,sizeof(_oscipoint)*OSCIARRAYLENGTH);
	if(osciPrintArrayPl == NULL)
	{
		DebugPrintf("ini osciPrintArrayPl wrong!\r\n");
		while(1);		
	}
	osciMulti[3] = 0.237037037;
	osciMulti[2] = 2.3636;
	osciMulti[1] = 120.0;
	osciMulti[0] = 1.0;
	
	// 1v/div
	osciCorrect[3][3] = 0.6186;
	osciCorrect[2][3] = 1.3754;
	osciCorrect[1][3] = 1.3807;

	// 0.1v/div
	osciCorrect[3][2] = 1.2114;
	osciCorrect[2][2] = 1.4239;
	osciCorrect[1][2] = 1.3989;

	//2mv/div
	osciCorrect[3][1] = 1.0781;
	osciCorrect[2][1] = 1.3026;
	osciCorrect[1][1] = 1.200;
	//kong
	
	osciCorrect[3][0] = 1.0;
	osciCorrect[2][0] = 1.0;
	osciCorrect[1][0] = 1.0;
	
	
	
	osciArryUsed =0;
	osciArrylUsed =0;
	vstate = 3;
	tstate = 3;


	//画UI和背景
	osci_DrawUI();
	osci_DrawBack();
	
	osciRUN =1;
	osciFFT = 0;
	POINT_COLOR = MYYELLOW1;
	LCD_ShowStringCentre(620,250,690,310,24,"RUN");
	POINT_COLOR = WHITE;
	
	changeVState(vstate);
	changeTState(tstate);	
//	osci_DrawData();
	Fpga_WriteReg(DACBREG,(u16)81) ; //0.3V
	
	osci_Sample();
	return 0;
}
int osci_DrawGrid(void)//画网格
{
	u16 i = 0;
	u16 tempx = 0;
	
	//垂直方向8div
	for(i=0;i<=8;i++)
	{
		if((i == 0)||(i == 4)||(i == 8) ) POINT_COLOR=BLUE;
		else POINT_COLOR= MYGRAY1;
		LCD_DrawLine(0 ,OSCIHIGH/8*i,OSCILENGTH-1, OSCIHIGH/8*i);
		POINT_COLOR= WHITE;
	}
	//水平10div
	for(i=0;i<=10;i++)
	{
		if((i == 0)||(i == 5)||(i == 10) ) POINT_COLOR=BLUE;
		else POINT_COLOR= MYGRAY1;
		LCD_DrawLine(OSCILENGTH/10*i ,0 ,OSCILENGTH/10*i, OSCIHIGH-1);
	}	
	POINT_COLOR = WHITE;	
	return 0;
}
int osci_DrawBack(void) // 填背景色和画网格
{
	LCD_Fill(0,0,OSCILENGTH-1,OSCIHIGH-1,BLACK);
	osci_DrawGrid();
	return 0;
}
int osci_DrawData(void)//更新并显示数据点 再重新画网格
{
	u16 iold =0;
	u16 inew =0;
	u16 oldx=0;
	
	osci_PrintToArray(); //更新数组数据和used情况
	
	//遍历数组
	osci_DrawGrid();
	if(osciSINGLE == 1)	//单次触发先把上一次数据抹去
	{
		while(iold < osciArrylUsed)
		{
			oldx = osciPrintArrayPl[iold].x;
			POINT_COLOR= BLACK;
			while((osciPrintArrayPl[iold].x <= oldx)&&(iold<osciArrylUsed))
			{
				LCD_DrawPoint( osciPrintArrayPl[iold].x,osciPrintArrayPl[iold].y);
				iold++;
			}					
			
		}
	}
	else
	{
		if(vstateChange ==1)
		{
			osci_DrawBack();
			vstateChange =0;
		}
		while(iold < osciArrylUsed)
		{
			oldx = osciPrintArrayPl[iold].x;
			
			//抹去老数据 
			POINT_COLOR= BLACK;
			while((osciPrintArrayPl[iold].x <= oldx)&&(iold<osciArrylUsed))
			{
				LCD_DrawPoint( osciPrintArrayPl[iold].x,osciPrintArrayPl[iold].y);
				iold++;
			}
			//打印新数据
			POINT_COLOR= MYYELLOW1;
			while((osciPrintArrayP[inew].x <= oldx)&&(inew<osciArryUsed))
			{		
				LCD_DrawPoint( osciPrintArrayP[inew].x,osciPrintArrayP[inew].y);
				inew++;
			}
		}
	}
	//补刀
	for(;inew<osciArryUsed;inew++)
	{
		LCD_DrawPoint( osciPrintArrayP[inew].x,osciPrintArrayP[inew].y);
	}
//	osci_DrawGrid();
	osci_ShowResult();
	if(osciSINGLE ==1) osciSINGLE =2;
	else if(osciSINGLE ==2) //单次触发结束
	{
		osciSINGLE =0;
		osciRUN =0;
		POINT_COLOR = WHITE;
		LCD_ShowStringCentre(620,250,690,310,24,"RUN");
		LCD_ShowStringCentre(710,320,780,380,16,"SINGLE");
	}
	POINT_COLOR = WHITE; 
	return 0;
}

int osci_Sample(void) //采样信号 要延时后才能read
{
//		osci_GetState();
		Fpga_WriteReg(3,1);
//		osci_GetState();
	return 0;
}
int osci_Read(void) //获取数据
{
	u32 i;
	u8 res =0;
	volatile u8 datatemp;
//	osci_GetState();
//	if(osciSINGLE ==2)
//	{
//		while(1)
//		{
//			osci_GetState();
//			if(osci.full) break;
//		}
//	}
	Fpga_WriteReg(3,2);//准备读取
	
//	osci_GetState();  
	
	//Fpga_ReadReg(13);//第一个废数据
	
	//osci_GetState();
	
	if(osciLOAD)
	{
		res=f_open(ftemp,"0:/test.dat",FA_READ); 
		if(res){ DebugPrintf("LOAD Failed\r\n"); while(1);}
		else
		{
			osciLOAD =0;
			osci_Load(ftemp,osciPoint,OSCIDATALENGTH);
			
			POINT_COLOR = WHITE;
			LCD_ShowStringCentre(710,1,780,60,24,"LOAD");
			
			osciRUN =0;
			POINT_COLOR = WHITE;
			LCD_ShowStringCentre(620,250,690,310,24,"RUN");	
			osci_Measure();				
		}
		f_close(ftemp);
		
	}
	else if(osciFFT)
		{
			u16 i;
			double *src;
			double *zabs;
			double zabsMax = 0.0;
			src=(double*)mymalloc(SRAMIN,sizeof(double)*256);
			if(src==NULL)
			{
					DebugPrintf("FFT MEM ERROR\r\n");
					while(1);
					
			}
			zabs=(double*)mymalloc(SRAMIN,sizeof(double)*256);
			if(src==NULL)
			{
					DebugPrintf("FFT MEM ERROR\r\n");
					while(1);
					
			}
			for(i=0;i<200;i++)
			{
				src[i] = (double)osciPoint[i] - OSCITRIVLIT/OSCIVREF*256;			
			}
			for(;i<256;i++)
			{
				src[i] = 0.0;
			}
			
			FFT(src,zabs,256);
			
			for(i=0;i<128;i++)
			{
				if(zabs[i] > zabsMax) zabsMax = zabs[i];
			}
			
			
			for(i=0;i<128;i++)
			{
				osciPoint[i] = (u8)(zabs[i]*256.0/zabsMax);			
			}
			for(;i<256;i++)
			{
				osciPoint[i] = 0;
			}
			
			myfree(SRAMIN,src);
			myfree(SRAMIN,zabs);
			
			//osciFFT = 0;
			osciRUN =0;
			POINT_COLOR = WHITE;
			LCD_ShowStringCentre(620,250,690,310,24,"RUN");	
			
		}
	else if(osciRUN)
	{

			for(i=0;i<OSCIDATALENGTH;i++)
			{
					datatemp = (u8)Fpga_ReadReg(13);//加偏置 ?
					osciPoint[i] = datatemp;
				delay_us(1);
			}
			for(i =0;i<10;i++) //无用
			{
					Fpga_ReadReg(13);
					delay_us(1);	
			}
			osci_Measure();	
	}
	else for(i=0;i<OSCIDATALENGTH +10;i++) Fpga_ReadReg(13);
	
	if(osciSAVE)
	{	
		osciSAVE =0;
		res=f_open(ftemp,"0:/test.dat",FA_WRITE|FA_OPEN_ALWAYS); 
		if(res) { DebugPrintf("SAVE Failed\r\n"); while(1);} 
		else
		{
			osci_Save(ftemp,osciPoint,OSCIDATALENGTH);
			POINT_COLOR = WHITE;
			LCD_ShowStringCentre(620,1,690,60,24,"SAVE");	
		}
		f_close(ftemp);	
	}
	
	
//	osci_GetState();
	return 0;
}
int osci_GetState(void) //获取状态 ？？？
{
	//RREG[12] <={26'b0,ReadFifoState,WriteFifoState,stack_empty,stack_full}
	u8 temp;
	temp = Fpga_ReadReg(12);
	osci.full = 0x01 & temp; //1满
	osci.empty = 0x01 & (temp>>1); // 1空
	osci.writeState = 0x03&(temp>>2); //0空闲 1预留 2等待触发 3采样
	osci.readsState = 0x03&(temp>>4);//0等待FIFO满 1可读等待读取 2读出数据
	//DebugStatePrint(5,"ReadState:%d  WriteState:%d  empty:%d full:%d",osci.readsState,osci.writeState,osci.empty,osci.full);
	
	temp = Fpga_ReadReg(4);
	//DebugStatePrint(6,"tState:%d",temp);
	return 0;
}
int osci_SetLinable(u8 en)//设置拟合 debug用
{
	en ? (osciLINE=1):(osciLINE=0);
	return 0;
}

int osci_DrawPointToArray(u16 x,u16 y) // 向数组打印数据 点
{
	osciPrintArrayP[osciArryUsed].x = x;
	osciPrintArrayP[osciArryUsed].y = y;
	osciArryUsed++;   //计点数
	return 0;
}
int osci_DrawLineToArray(u16 x1, u16 y1, u16 x2, u16 y2) // 向数组打印数据 线
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
//	else return 1;
		
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	//distance=delta_x;//显示算法要求
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		osci_DrawPointToArray(uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}
  return 0;  
}  
//int osci_PrintToArray(void) //打印数据到数组 
//{
//	int i;
//	_oscipoint* temparrayP =NULL;
//	
//		//上一次的波形存为老波形，used变为老used 新的used从0开始
//		temparrayP = osciPrintArrayP;
//		osciPrintArrayP = osciPrintArrayPl;
//		osciPrintArrayPl = temparrayP;
//		
//		osciArrylUsed = osciArryUsed;
//		osciArryUsed =0;
//		
//		if(!osciLINE)
//			for(i=0;i<OSCIDATALENGTH;i++)
//				osci_DrawPointToArray( (u16)((float)i*(float)OSCILENGTH/(float)OSCIDATALENGTH-0.5f), OSCIHIGH - (u16)((float)osciPoint[i]*(float)OSCIHIGH/(float)OSCIDATARANGE)-0.5f);		
//		else
//			for(i=0;i<OSCIDATALENGTH -1;i++)
//				osci_DrawLineToArray((u16)((float)i*(float)OSCILENGTH/(float)OSCIDATALENGTH -0.5f),OSCIHIGH - (u16)((float)osciPoint[i]*(float)OSCIHIGH/(float)OSCIDATARANGE -0.5f), (u16)((float)(i+1)*(float)OSCILENGTH/(float)OSCIDATALENGTH -0.5f),OSCIHIGH - (u16)((float)osciPoint[i+1]*(float)OSCIHIGH/(float)OSCIDATARANGE -0.5f));

//	return 0;
//}
int osci_PrintToArray(void) //打印数据到数组 
{
	int i;
	u16 y1;
	u16 y2;
	_oscipoint* temparrayP =NULL;
	
		//上一次的波形存为老波形，used变为老used 新的used从0开始
		temparrayP = osciPrintArrayP;
		osciPrintArrayP = osciPrintArrayPl;
		osciPrintArrayPl = temparrayP;
		
		osciArrylUsed = osciArryUsed;
		osciArryUsed =0;
	
		if(osciFFT)
		{		
				if(!osciLINE)
					for(i=0;i<128;i++)
				{
					y1 = (u16)abs((float)osciPoint[i]/256.0f*400.0);
					if(y1 > 399) y1=399;
					osci_DrawPointToArray( (u16)((float)i*128.0f/(float)OSCIDATALENGTH + 0.5f), y1 );	
				}			
				else
					for(i=0;i<128 -1;i++)
				{
					y1 = (u16)abs((float)osciPoint[i]/256.0f*400.0);
					y2 = (u16)abs((float)osciPoint[i+1]/256.0f*400.0);
					if(y1 > 399) y1=399;
					if(y2 > 399) y2=399;
					osci_DrawLineToArray((u16)((float)i*128.0f/(float)OSCIDATALENGTH + 0.5f),y1, (u16)((float)(i+1)*128.0f/(float)OSCIDATALENGTH +0.5f),y2);
				}				
			
				osciVolt = 0.0;
				osciFreq = 0.0;
				osciFFT = 0;
		}
		else
		{
				if(!osciLINE)
					for(i=0;i<OSCIDATALENGTH;i++)
				{
					y1 = (u16)(200 - (int)( ((float)osciPoint[i]*OSCIVREF/256.0f - OSCITRIVLIT)/osciVrangeNow*200.0f));
					if(y1 > 400) y1=400;
					osci_DrawPointToArray( (u16)((float)i*(float)OSCILENGTH/(float)OSCIDATALENGTH),y1 );	
				}			
				else
					for(i=0;i<OSCIDATALENGTH -1;i++)
				{
					y1 = (u16)(200 - (int)( ((float)osciPoint[i]*OSCIVREF/256.0f - OSCITRIVLIT)/osciVrangeNow*200.0f));
					y2 = (u16)(200 - (int)( ((float)osciPoint[i+1]*OSCIVREF/256.0f - OSCITRIVLIT)/osciVrangeNow*200.0f));
					if(y1 > 400) y1=400;
					if(y2 > 400) y1=400;
					osci_DrawLineToArray((u16)((float)i*(float)OSCILENGTH/(float)OSCIDATALENGTH ),y1, (u16)((float)(i+1)*(float)OSCILENGTH/(float)OSCIDATALENGTH ),y2);
				}		
		}

	
	return 0;
}

int osci_DrawUI(void) // 控制和显示界面
{
		LCD_Clear(BLACK);
	
	
		POINT_COLOR = BLUE;
		//电压轴
		LCD_DrawRectangle(0,400,60,479);//+  0,49	400,479
	
		LCD_DrawRectangle(60 ,400,240,439);// 60,239 400 439
		LCD_DrawRectangle(60 ,439,240,479);// 60,239 440 479
	
		LCD_DrawRectangle(240,400,300,479);//- 240,299 400 479
	
		//时间轴
		LCD_DrawRectangle(300,400,360,479);//+ 300，359 400 479
		
		LCD_DrawRectangle(360,400,540,439);// 360,539 400 439
		LCD_DrawRectangle(360,439,540,479);// 360,539 440 479
		
		LCD_DrawRectangle(540,400,600,479);//-	540,599 400,479

		POINT_COLOR = WHITE;
		LCD_ShowString(24,428,24,24,24,"+");
		LCD_ShowString(61,408,168,24,24," v/div");
		LCD_ShowString(61,448,168,24,24," mv");
		LCD_ShowString(264,428,24,24,24,"-");
	
		LCD_ShowString(324,428,24,24,24,"+");
		LCD_ShowString(361,408,168,24,24," s/div");
		LCD_ShowString(361,448,168,24,24," s");
		LCD_ShowString(564,428,24,24,24,"-");
		
		POINT_COLOR = BLUE;
		//储存和load
		LCD_DrawRectangle(620,1,690,60);//SAVE
		LCD_DrawRectangle(710,1,780,60);//LOAD
		//显示控制	
		LCD_DrawRectangle(675,70,725,120);//Up
		LCD_DrawRectangle(675,130,725,180);//O
		LCD_DrawRectangle(615,130,665,180);//l	
		LCD_DrawRectangle(735,130,785,180);//r	
		LCD_DrawRectangle(675,190,725,240);//down
		
		LCD_DrawRectangle(620,250,690,310);//RUN
		LCD_DrawRectangle(710,250,780,310);//interpotation
		
		LCD_DrawRectangle(620,320,690,380);//AUTO
		LCD_DrawRectangle(710,320,780,380);//SINGLE		
		
		POINT_COLOR = WHITE;
		//键名
		LCD_ShowStringCentre(620,1,690,60,24,"SAVE");
		LCD_ShowStringCentre(710,1,780,60,24,"LOAD");
		LCD_ShowStringCentre(675,70,725,120,24,"U");
		LCD_ShowStringCentre(675,130,725,180,24,"O");
		LCD_ShowStringCentre(615,130,665,180,24,"L");
		LCD_ShowStringCentre(735,130,785,180,24,"R");
		LCD_ShowStringCentre(675,190,725,240,24,"D");
		LCD_ShowStringCentre(620,250,690,310,24,"RUN");
		LCD_ShowStringCentre(710,250,780,310,24,"LINE");
		LCD_ShowStringCentre(620,320,690,380,24,"FFT");
		LCD_ShowStringCentre(710,320,780,380,16,"SINGLE");
	
	
	return 0;
}

u8 osci_KeyScan(void)
{
	int i;
	int timecnt =0;
	int xlst,ylst;
	tp_dev.scan(0);
				if((tp_dev.sta)&(1<<0))//第0点触摸
				{
					if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)//在LCD范围内
					{
						
					//	if((tp_dev.x[0] == xlst)&&(tp_dev.y[0] == ylst)) return 0;
						xlst = tp_dev.x[0];
						ylst = tp_dev.y[0];
						while(1) //直到放手
						{
							tp_dev.scan(0);
							if((tp_dev.x[0] != xlst)&&(tp_dev.y[0] != ylst)) break;
							if(timecnt > 5e+5) break;
							timecnt++;
						}
						tp_dev.x[0] = xlst;
						tp_dev.y[0] = ylst;
						
						
						if(JUDGEKEY(620,1,690,60,tp_dev.x[0],tp_dev.y[0])) return 1;	//SAVE
						if(JUDGEKEY(710,1,780,60,tp_dev.x[0],tp_dev.y[0])) return 2;	//LOAD
						if(JUDGEKEY(675,70,725,120,tp_dev.x[0],tp_dev.y[0])) return 3;	//U
						if(JUDGEKEY(675,130,725,180,tp_dev.x[0],tp_dev.y[0])) return 4;	//O
						if(JUDGEKEY(615,130,665,180,tp_dev.x[0],tp_dev.y[0])) return 5;	//L
						if(JUDGEKEY(735,130,785,180,tp_dev.x[0],tp_dev.y[0])) return 6;	//R
						if(JUDGEKEY(675,190,725,240,tp_dev.x[0],tp_dev.y[0])) return 7;	//D
						if(JUDGEKEY(620,250,690,310,tp_dev.x[0],tp_dev.y[0])) return 8;	//RUN
						if(JUDGEKEY(710,250,780,310,tp_dev.x[0],tp_dev.y[0])) return 9;	//LINE

						if(JUDGEKEY(620,320,690,380,tp_dev.x[0],tp_dev.y[0])) return 10;	//FFT
						if(JUDGEKEY(710,320,780,380,tp_dev.x[0],tp_dev.y[0])) return 11;	//SINGLE
						
						if(JUDGEKEY(0,400,60,479,tp_dev.x[0],tp_dev.y[0])) return 12;	//v+
						if(JUDGEKEY(240,400,300,479,tp_dev.x[0],tp_dev.y[0])) return 13;	//v-
						if(JUDGEKEY(300,400,360,479,tp_dev.x[0],tp_dev.y[0])) return 14;	//t+
						if(JUDGEKEY(540,400,600,479,tp_dev.x[0],tp_dev.y[0])) return 15;	//t-
//						
//						if((tp_dev.x[0]>620)&&(tp_dev.x[0]<690)&&(tp_dev.y[0]>250)&&(tp_dev.y[0]<310)) return 8;//RUN
//						if((tp_dev.x[0]>710)&&(tp_dev.x[0]<780)&&(tp_dev.y[0]>250)&&(tp_dev.y[0]<310)) return 9;//LINE
						
					  return 0;
					}
				}
			//}
	return 0;	
	
	
}
int osci_KeyDeal(u8 keyvalue)
{
		if(keyvalue !=0)
		{
			POINT_COLOR = WHITE;
			//DebugStatePrint(4,"touch:%d",keyvalue);
			if(keyvalue==1)
			{
				osciSAVE = 1;
				POINT_COLOR = MYYELLOW1;
				LCD_ShowStringCentre(620,1,690,60,24,"SAVE");				
				return 0;
			}
			if(keyvalue==2)
			{
				osciLOAD =1;
				POINT_COLOR = MYYELLOW1;
				LCD_ShowStringCentre(710,1,780,60,24,"LOAD");
				
				return 0;
			}
//			if(keyvalue==3);
			if(keyvalue==4) //FFT
			{
				
				vstateChange =1;
				
			}
//			if(keyvalue==5);
//			if(keyvalue==6);
//			if(keyvalue==7);
			if(keyvalue==8)	
			{
				osciRUN = osciRUN ? 0:1;	
				POINT_COLOR = osciRUN ? MYYELLOW1:WHITE;
				LCD_ShowStringCentre(620,250,690,310,24,"RUN");
				vstateChange =1;
				return 0;
			}
			if(keyvalue==9)
			{
				osciLINE = osciLINE ? 0:1;
				POINT_COLOR = osciLINE ? MYYELLOW1:WHITE;
				LCD_ShowStringCentre(710,250,780,310,24,"LINE");
				vstateChange =1;
				return 0;
			}
			if(keyvalue==10)
			{
				vstateChange =1;
				osciFFT =1;
			}
			if(keyvalue==11)
			{
				osciSINGLE = 1;	
				osciRUN = 1;
				POINT_COLOR = MYYELLOW1;
				LCD_ShowStringCentre(710,320,780,380,16,"SINGLE");
				return 0;
			}
			if(keyvalue==12)//v+
			{
				if(vstate<=2) 
				{			
					vstate++;
					changeVState(vstate);
					vstateChange =1;
				}
				
			}
			if(keyvalue==13)//v-
			{
				if(vstate >=2)
				{
					vstate--;
					changeVState(vstate);
					vstateChange =1;
				}
				
			}
			if(keyvalue==14)//t+
			{	
				if(tstate<=2)
				{
					tstate++;
					changeTState(tstate);
					vstateChange =1;
					
				}
			}
			if(keyvalue==15)//t-
			{
				if(tstate >=2)			
				{
					tstate--;
					changeTState(tstate);	
					vstateChange =1;
				}					
			}
		}
		return 0;
}
int changeTState(u8 target)
{
	Fpga_WriteReg(SAMPLERATEREG,target);
			 if(target ==0) LCD_ShowString(361,408,168,24,24,"  ? s/div");
	else if(target ==1) LCD_ShowString(361,408,168,24,24,"100 ns/div");
	else if(target ==2) LCD_ShowString(361,408,168,24,24,"  2 us/div");
	else if(target ==3) LCD_ShowString(361,408,168,24,24," 20 ms/div");
	
	
	
	return 0;
}
int changeVState(u8 target)
{
	POINT_COLOR=WHITE;
	if(target ==0)
	{
		SWITCH1V0 = 1;
		SWITCH0V1 = 0;
		SWITCH2MV = 0;
		osciCoefNow = COEFI1V0;
		osciVrangeNow = RANGE1V0;		
		LCD_ShowString(61,408,168,24,24,"  ?  v/div");
		return 0;
	}
	if(target ==1)
	{
		SWITCH1V0 = 0;
		SWITCH0V1 = 0;
		SWITCH2MV = 1;
		
		osciCoefNow = COEFI2MV;
		osciVrangeNow = RANGE2MV;
		
		LCD_ShowString(61,408,168,24,24," 2 mv/div");
		return 0;
	}	
	if(target ==2)
	{
		SWITCH1V0 = 0;
		SWITCH0V1 = 1;
		SWITCH2MV = 0;
		osciCoefNow = COEFI0V1;
		osciVrangeNow = RANGE0V1;		
		LCD_ShowString(61,408,168,24,24," 0.1v/div");
		return 0;
	}
	if(target ==3)
	{
		SWITCH1V0 = 1;
		SWITCH0V1 = 0;
		SWITCH2MV = 0;
		osciCoefNow = COEFI1V0;
		osciVrangeNow = RANGE1V0;		
		LCD_ShowString(61,408,168,24,24,"  1 v/div");
		return 0;
	}	
	return 1;
	
}
int osci_Save(FIL *datafile,u8 *databuff,u32 n)
{
	u8 res=0;
	u32 i=0;
	
	for(i=0;i<n;i++)
		f_printf(datafile,"%03d\n",databuff[i]);
	return 0;
}
int osci_Load(FIL *datafile,u8 *databuff,u32 n)
{
	u8 res=0;
	u32 i=0;
	char chtemp[5];
	
	for(i=0;i<n;i++)
	{
		f_gets ((TCHAR*) chtemp,5, datafile);
		chtemp[3] = '\0';
		databuff[i] = atoi(chtemp);
	}
	return 0;
}
int osci_Measure(void)
{
	u16 i;
	u16 Tcnt =0;
	u16 vstand;
	u16 x1 =0,x2 =0;
	double Tdang;
	double deltat = 0.0;
	double fcorrect;
	
	u16 vmax,vmin;
	double deltav;
//osciTriVolt
//	osciFreq
	vstand = (u16)((osciTriVolt + OSCITRIVLIT)/OSCIVREF *256.0f);
	for(i=0; i< OSCIDATALENGTH-1;i++)
	{
		if((osciPoint[i] < vstand)&&(osciPoint[i+1] >= vstand)){
			if(Tcnt ==0)x1 =i;
			else if(Tcnt>0) {
				x2 =i;
				deltat += x2-x1;
				x1 = x2;
			}
			Tcnt++;
		}
	}
	
	if(Tcnt <= 2)
	{
		int halfXl;
		int halfXr;
		
		for(i=0; i< OSCIDATALENGTH-1;i++)
		{
			//if((osciPoint[i] >= vstand)&&(osciPoint[i+1] < vstand)){
			if((osciPoint[i] < vstand)&&(osciPoint[i+1] >= vstand)){
				halfXl = i;
				break;
			}
		}
		
		for(i=0; i< OSCIDATALENGTH-1;i++)
		{
			//if((osciPoint[i] >= vstand)&&(osciPoint[i+1] < vstand)){
			if((osciPoint[i] >= vstand)&&(osciPoint[i+1] < vstand)){
				halfXr = i;
				break;
			}
		}
		deltat = abs(halfXr -halfXl)*2;
		//DebugStatePrint(6,"l= %d,r = %d ,deltat = %f ",halfXl,halfXr,deltat);
		
	}
		
	//DebugStatePrint(3,"Tcnt = %d,vstand = %d",Tcnt,vstand);
	if(Tcnt>2) deltat = deltat/(double)(Tcnt-1);
	if(tstate == 0) Tdang = 1;
	else if(tstate == 1) Tdang = 5e-3;//M
		else if(tstate == 2) Tdang = 1e-4;//K
			else if(tstate == 3) Tdang = 1e-3;//1
	osciFreq = 1/(deltat * Tdang);
//	osciVolt
	vmax = osciPoint[0];
	vmin = osciPoint[0];
	for(i=0; i< OSCIDATALENGTH-1;i++)
	{
		if(osciPoint[i] > vmax) vmax = osciPoint[i];
		if(osciPoint[i] < vmin) vmin = osciPoint[i];
	}
	
	//DebugStatePrint(5,"delta = %d osciCoefNow = %f",vmax-vmin,osciCoefNow);
	deltav = ((float)(vmax-vmin)/256.0*OSCIVREF)/(osciMulti[vstate]*osciCorrect[tstate][vstate]);

	if(vstate == 0) osciVolt = 0.0f;
	else if(vstate == 1) osciVolt = deltav*1000.0;//m
		else if(vstate == 2) osciVolt = deltav;//v
			else if(vstate == 3) osciVolt = deltav;//v
	
	

 if((tstate ==3)&&(vstate == 3)) //1V/div dipin
	{
		fcorrect = -0.0001943 *osciFreq*osciFreq + 0.0324223 *osciFreq + 0.7456233 ;
		//osciDisplayCoef = 1/fcorrect;
		osciVolt = osciVolt/fcorrect;		
		//y = -0.0001943 x2 + 0.0324223 x + 0.7456233 
	}
	else if((tstate ==3)&&(vstate == 2)) //0.1V/div dipin
	{
		fcorrect = -0.0000095682 *osciFreq*osciFreq + 0.0016309242 *osciFreq + 0.0415366667 ;
		//osciDisplayCoef = 1/fcorrect;
		osciVolt = osciVolt/fcorrect;		
		//y = -0.0000095682 x2 + 0.0016309242 x + 0.0415366667 
	}
	else if((tstate ==3)&&(vstate == 1)) // 2mV/div dipin
	{
		fcorrect = -0.0001008788 *osciFreq*osciFreq + 0.0174047273 *osciFreq + 0.4265933333 ;
		//osciDisplayCoef = 1/fcorrect;
		osciVolt = osciVolt/fcorrect;		
		//y = -0.0001008788 x2 + 0.0174047273 x + 0.4265933333 

	}
	else if(tstate ==1)  //1V/div gaopin
	{
		fcorrect = 0.005011 *osciFreq*osciFreq*osciFreq-0.090199 *osciFreq*osciFreq + 0.361196 *osciFreq + 0.695673;
		//osciDisplayCoef = 1/fcorrect;
		osciVolt = osciVolt/fcorrect;
	}
	//else osciDisplayCoef = 1.0f;
	

		return 0;
}
int osci_ShowResult(void)
{
	
	char unitT[4];
	char strT[12];
	char strV[12];
	
	unitT[1] = 'H';
	unitT[2] = 'z';
	unitT[3] = '\0';

	
	
	if(tstate == 0) unitT[0] = '?';
		else if(tstate == 1) unitT[0] = 'M';
			else if(tstate == 2) unitT[0] = 'K';
				else if(tstate == 3) unitT[0] = ' ';
//	
//	if(vstate == 0) unitT[0] = '?';
//		else if(vstate == 1) unitT[0] = 'm';
//			else if(vstate == 2) unitT[0] = 'm';
//				else if(vstate == 3) unitT[0] = ' ';	

	sprintf(strT,"%8.3f%s",osciFreq,unitT);
	
	sprintf(strV,"%8.3f",osciVolt);
	POINT_COLOR = WHITE;
	
	LCD_ShowString(61,448,168,24,24,strV);
	LCD_ShowString(361,448,168,24,24,strT);	
	
	return 0;
}


