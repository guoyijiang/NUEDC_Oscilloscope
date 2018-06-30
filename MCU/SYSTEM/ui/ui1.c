#include "ui1.h"





/*
	按键位置自动分配，大小自定，名称自定，返回键值；
	配合DebugStatePrint函数使用；


*/
		
int keyboard1_Input(KEY1STRUCT *key1array)
{
	//int t;
	int i;
	int xlst,ylst;
	tp_dev.scan(0);
	//for(t=0;t<CT_MAX_TOUCH;t++)//最多5点触摸
			//{
				if((tp_dev.sta)&(1<<0))//第0点触摸
				{
					if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)//在LCD范围内
					{
						xlst = tp_dev.x[0];
						ylst = tp_dev.y[0];
						while(1) //直到放手
						{
							tp_dev.scan(0);
							if((tp_dev.x[0] != xlst)&&(tp_dev.y[0] != ylst)) break;
						}
						tp_dev.x[0] = xlst;
						tp_dev.y[0] = ylst;

						for(i=1;i<=NKEY1;i++)
							if((tp_dev.x[0]>key1array[i].xb)&&(tp_dev.x[0]<key1array[i].xe)&&(tp_dev.y[0]>key1array[i].yb)&&(tp_dev.y[0]<key1array[i].ye))
								return i;							
					  return 0;
					}
				}
			//}
	return 0;
}
//根据需求添加任意个按键
int keyBoard1_Generate(KEY1STRUCT *key1array)
{	
	u16 i = 0;
	u16 x = 8;
	u16 y = 400;
	u16 xend = 472;
	u16 yend = 792;
	
	LCD_DrawRectangle(8-1, 16-1, 472+1,400-1);
	
	//用户命名
	strcpy(key1array[1].name,"12dB");
	strcpy(key1array[2].name,"16dB");
	strcpy(key1array[3].name,"20dB");
	strcpy(key1array[4].name,"24dB");
	strcpy(key1array[5].name,"28dB");
	strcpy(key1array[6].name,"32dB");
	strcpy(key1array[7].name,"36dB");
	strcpy(key1array[8].name,"40dB");
	strcpy(key1array[9].name,"44dB");
	strcpy(key1array[10].name,"48dB");
	strcpy(key1array[11].name,"52dB");
	strcpy(key1array[12].name,"<-");
	
	strcpy(key1array[13].name," 0");
	strcpy(key1array[14].name," 1");
	strcpy(key1array[15].name," 2");
	strcpy(key1array[16].name," 3");
	strcpy(key1array[17].name," 4");
	strcpy(key1array[18].name," 5");
	strcpy(key1array[19].name," 6");
	strcpy(key1array[20].name," 7");
	strcpy(key1array[21].name," 8");
	strcpy(key1array[22].name," 9");
	strcpy(key1array[23].name," .");
	strcpy(key1array[24].name," set/v");
	strcpy(key1array[25].name," -");
	

	while(1)
	{
		if(((x+WITHKEY1) <= xend)&&(y+HIGHKEY1 <= yend))
		{
			if(i < NKEY1)
			{
				i++;
				key1array[i].num = i;
				key1array[i].xb = x;
				key1array[i].yb = y;	
				x += WITHKEY1;
				key1array[i].xe = x;
				key1array[i].ye = y + HIGHKEY1;
				
				//sprintf(key1array[i].name,"%d",i);
				
				LCD_DrawRectangle(key1array[i].xb , key1array[i].yb, key1array[i].xe,key1array[i].ye);
				LCD_ShowString(key1array[i].xb+1,key1array[i].yb+1,WITHKEY1-1,HIGHKEY1-1,16, (u8*)key1array[i].name);
									
			}		
			else 
				return 0;	

		}
		else if((x+WITHKEY1) > xend)
		{
			x = 8;
			y += HIGHKEY1;
			if((y+HIGHKEY1) > yend)
				return 1;
		}		
	}
	
	
}
