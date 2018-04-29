//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　       Common                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Common_Header
#define Common_Header

#include <windows.h>
#include <math.h>
#include <stdio.h>

#define SIN_DELETE(p)   if(p){delete p;      p=NULL;}
#define ARRAY_DELETE(p) if(p){delete[] p;    p=NULL;}

void MesB(char *str, float num);
float Sigmoid(float in);
void SigmoidArr(float *out, float *in, unsigned int Num);
float Logit(float in);
float ReLU(float in);

#endif