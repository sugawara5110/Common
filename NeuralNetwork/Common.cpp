//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　       Common                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Common.h"

void MesB(char *str, float num) {
	MessageBoxA(0, str, 0, MB_OK);
	char st[50];
	sprintf(st, "%f", num);
	MessageBoxA(0, st, 0, MB_OK);
}

float Sigmoid(float in) {
	return 1 / (1 + pow(2.71828182846, -in));
}

void SigmoidArr(float *out, float *in, unsigned int Num) {
	for (unsigned int i = 0; i < Num; i++)out[i] = Sigmoid(in[i]);
}

float Logit(float in) {
	if (in < 0.01f)in = 0.01f;
	if (in > 0.99f)in = 0.99f;
	return log(in / (1 - in));
}

float ReLU(float in) {
	if (in < 0)in = 0;
	return in;
}
