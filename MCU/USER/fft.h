#ifndef __FFT_H
#define __FFT_H


#define M_PI 3.14159265358979323846
#define PI 3.14159265358979323846
#define SIZE 256
#define VALUE_MAX 1000



////////////////////////////////////////////////////////////////////
//定义一个复数结构体
///////////////////////////////////////////////////////////////////
typedef struct Complex_{
    double real;
    double imagin;
}Complex;


void Add_Complex(Complex * src1,Complex *src2,Complex *dst);
void Sub_Complex(Complex * src1,Complex *src2,Complex *dst);
void Multy_Complex(Complex * src1,Complex *src2,Complex *dst);
void getWN(double n,double size_n,Complex * dst);
int FFT_remap(double * src,int size_n);
void FFT(double * src,double * zabs,int size_n);


#endif