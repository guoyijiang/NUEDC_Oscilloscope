#ifndef __RLCMEASURE_H
#define __RLCMEASURE_H
#include "sys.h"
#include "math.h"
#include "connectFPGA.h"
#include "lcd.h"
#include "lcdpro.h"

#define AddrFreq 2
#define AddrMeasureTick 3
#define AddrRegUxx 12
#define AddrRegUxy 13
#define AddrRegUsx 14
#define AddrRegUsy 15
#define AddrInMeasure 16

#define VLIFT -2.0
#define ADVREF 4.4993
#define DAVREF 2.5000
#define NRS 7

#define	SWITCH6 		PDout(11) 
#define	SWITCH1 		PDout(12)
#define	SWITCH0 		PDout(13) 
#define	SWITCH5 		PGout(2)
#define	SWITCH4 		PGout(3) 
#define	SWITCH3 		PGout(4) 
#define	SWITCH2 		PGout(5) 
#define	SWITCHUX		PGout(6) 
#define	SWITCHUS 		PGout(7)

typedef struct 
{
	u32 readDataUxx;
	u32	readDataUxy;
	u32 readDataUsx;
	u32 readDataUsy;
	
	double Uxx;
	double Uxy;
	double Usx;
	double Usy;
	
	double Zabs;
	double Zre;
	double Zim;
	double QD;
	
	double freq;
	int RsState;
	double Rs[NRS];
	
	char feature; // R L C 
	char isIt;
	char autMode;
	
}RLC_Measure;

int rlc_Initial(RLC_Measure* RLC);
int rlc_SetFreq(RLC_Measure* RLC,double freq);
int rlc_MeasureEn(RLC_Measure* RLC);
int rlc_GetResult(RLC_Measure* RLC);
int rlc_Calc(RLC_Measure* RLC);
int rlc_UpdataPlay(RLC_Measure* RLC);
int rlc_stopMeasure(RLC_Measure* RLC);
int rlc_ChangeRange(RLC_Measure* RLC);
int rlc_ChangeFreq(RLC_Measure* RLC,char inputChar);
int rlc_Measure(RLC_Measure* RLC,char inputChar);
int changeState(char target);
int changeXS(char target);
#endif
