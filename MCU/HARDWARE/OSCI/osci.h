#ifndef __OSCI_H
#define __OSCI_H
#include "sys.h"
#include "touch.h"
#include "sdio_sdcard.h" 
#include "ff.h"  
#include "exfuns.h"  
#define OSCISTATEREG 12
#define OSCIDATAREG 13
#define OSCIENREG 3
#define SAMPLERATEREG 4
#define DACBREG 5

#define OSCIDATALENGTH 200
#define OSCIDATARANGE 256

#define OSCILENGTH 600 //为20整数倍
#define OSCIHIGH 400 //256.1.5 为8整数倍

//#define OSCISX 0
//#define OSCISY 0

#define COEFI2MV 120.0
#define COEFI0V1 2.3636
#define COEFI1V0 0.237037037

#define RANGE2MV  1.92   	// 	120.0*16/1000
#define RANGE0V1	1.89088		//	2.3636*0.8
#define RANGE1V0	1.8962963		//	0.237037037f *8

#define OSCIARRAYLENGTH  120000 //400*600
#define OSCITRIVLIT		1.34f
#define OSCIVREF	3.0f

//#define	SWITCH1V0 		PDout(11) 
//#define	SWITCH0V1 		PDout(12)
//#define	SWITCH2MV 		PDout(13) 

#define	SWITCH0V1 		PDout(11)
#define	SWITCH2MV 		PDout(12) 
#define	SWITCH1V0 		PDout(13) 

//{26'b0,ReadFifoState,WriteFifoState,stack_empty,stack_full}
typedef struct
{
	u8 readsState;
	u8 writeState;
	u8 empty;
	u8 full;
}_osci;
typedef struct
{
	u16 x;
	u16 y;
}_oscipoint;


int osci_Init(void);
int osci_Off(void);
int osci_DrawBack(void);
int osci_DrawGrid(void);
int osci_DrawData(void);

int osci_DrawUI(void);

int osci_Sample(void);
int osci_Read(void);
int osci_GetState(void);
int osci_SetLinable(u8);
int osci_DrawPointToArray(u16 x,u16 y);
int osci_DrawLineToArray(u16 x1, u16 y1, u16 x2, u16 y2);
int osci_PrintToArray(void);

u8 osci_KeyScan(void);
int osci_KeyDeal(u8 keyvalue);
int changeTState(u8 target);
int changeVState(u8 target);

int osci_Save(FIL *datafile,u8 *databuff,u32 n);
int osci_Load(FIL *datafile,u8 *databuff,u32 n);

int osci_Measure(void);
int osci_ShowResult(void);
#endif
