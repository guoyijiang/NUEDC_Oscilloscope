#include "fft.h"
#include "sys.h"
#include "math.h"
#include "malloc.h" 
#include "lcdpro.h"





//typedef struct Complex_ Complex;
////////////////////////////////////////////////////////////////////
//定义一个复数计算，包括乘法，加法，减法
///////////////////////////////////////////////////////////////////
void Add_Complex(Complex * src1,Complex *src2,Complex *dst){
    dst->imagin=src1->imagin+src2->imagin;
    dst->real=src1->real+src2->real;
}
void Sub_Complex(Complex * src1,Complex *src2,Complex *dst){
    dst->imagin=src1->imagin-src2->imagin;
    dst->real=src1->real-src2->real;
}
void Multy_Complex(Complex * src1,Complex *src2,Complex *dst){
    double r1=0.0,r2=0.0;
    double i1=0.0,i2=0.0;
    r1=src1->real;
    r2=src2->real;
    i1=src1->imagin;
    i2=src2->imagin;
    dst->imagin=r1*i2+r2*i1;
    dst->real=r1*r2-i1*i2;
}
////////////////////////////////////////////////////////////////////
//在FFT中有一个WN的n次方项，在迭代中会不断用到，具体见算法说明
///////////////////////////////////////////////////////////////////
void getWN(double n,double size_n,Complex * dst){
    double x=2.0*M_PI*n/size_n;
    dst->imagin=-sin(x);
    dst->real=cos(x);
}

////////////////////////////////////////////////////////////////////
//定义FFT的初始化数据，因为FFT的数据经过重新映射，递归结构
///////////////////////////////////////////////////////////////////
int FFT_remap(double * src,int size_n){

    int i;
		double *temp;
    if(size_n==1)
        return 0;
    temp=(double *)mymalloc(SRAMEX,sizeof(double)*size_n);
		if(temp == NULL)
		{
				DebugPrintf("FFT_remap MEM ERROR\r\n");
				while(1);
				
		}
    for(i=0;i<size_n;i++)
        if(i%2==0)
            temp[i/2]=src[i];
        else
            temp[(size_n+i)/2]=src[i];
    for(i=0;i<size_n;i++)
        src[i]=temp[i];
    myfree(SRAMEX,temp);
    FFT_remap(src, size_n/2);
    FFT_remap(src+size_n/2, size_n/2);
    return 1;


}
////////////////////////////////////////////////////////////////////
//定义FFT，具体见算法说明，注释掉的显示部分为数据显示，可以观察结果
///////////////////////////////////////////////////////////////////
void FFT(double * src,double * zabs,int size_n){

    int i,j,k,z;
		Complex *src_com;
    FFT_remap(src, size_n);
   // for(int i=0;i<size_n;i++)
    //    printf("%lf\n",src[i]);
    //clock_t start,end;
    //start=clock();
    k=size_n;
    z=0;
    while (k/=2) {
        z++;
    }
    k=z;
    if(size_n!=(1<<k))
		{
				DebugPrintf("FFT  ERROR\r\n");
				while(1);			
		}
    src_com=(Complex*)mymalloc(SRAMEX,sizeof(Complex)*size_n);
    if(src_com==NULL)
		{
				DebugPrintf("FFT MEM ERROR\r\n");
				while(1);
				
		}
    for(i=0;i<size_n;i++){
        src_com[i].real=src[i];
        src_com[i].imagin=0;
    }
    for(i=0;i<k;i++){
        z=0;
        for(j=0;j<size_n;j++){
            if((j/(1<<i))%2==1){
                Complex wn;
								Complex temp;
								int neighbour;
                getWN(z, size_n, &wn);
                Multy_Complex(&src_com[j], &wn,&src_com[j]);
                z+=1<<(k-i-1);       
                neighbour=j-(1<<(i));
                temp.real=src_com[neighbour].real;
                temp.imagin=src_com[neighbour].imagin;
                Add_Complex(&temp, &src_com[j], &src_com[neighbour]);
                Sub_Complex(&temp, &src_com[j], &src_com[j]);
            }
            else
                z=0;
        }

    }

   /* for(int i=0;i<size_n;i++)
        if(src_com[i].imagin>=0.0){
            printf("%lf+%lfj\n",src_com[i].real,src_com[i].imagin);
        }
        else
            printf("%lf%lfj\n",src_com[i].real,src_com[i].imagin);*/
//    for(i=0;i<size_n;i++){
//        dst[i].imagin=src_com[i].imagin;
//        dst[i].real=src_com[i].real;
//    }
		for(i=0;i<size_n;i++){
			zabs[i] = (src_com[i].imagin*src_com[i].imagin + src_com[i].real*src_com[i].real);
    }
    //end=clock();
    //printf("FFT use time :%lfs for Datasize of:%d\n",(double)(end-start)/CLOCKS_PER_SEC,size_n);
	myfree(SRAMEX,src_com);
}
////////////////////////////////////////////////////////////////////
