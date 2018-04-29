//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　  　　Pooling                                               **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Pooling.h"

Pooling::Pooling(unsigned int width, unsigned int height, unsigned int poolNum) {

	PoolNum = poolNum;
	Width = width;
	if (Width % 2 == 1)OddNumWid = 1;
	if (Height % 2 == 1)OddNumHei = 1;
	Height = height;
	input = new float*[PoolNum];
	output = new float*[PoolNum];
	inerror = new float*[PoolNum];
	outerror = new float*[PoolNum];
	for (unsigned int i = 0; i < PoolNum; i++) {
		input[i] = new float[Width * Height];
		outerror[i] = new float[Width * Height];
		output[i] = new float[(Width / POOL) * (Height / POOL)];
		inerror[i] = new float[(Width / POOL) * (Height / POOL)];
	}
	OutWid = Width / POOL;
	OutHei = Height / POOL;
}

Pooling::~Pooling() {
	for (unsigned int i = 0; i < PoolNum; i++) {
		ARRAY_DELETE(input[i]);
		ARRAY_DELETE(output[i]);
		ARRAY_DELETE(inerror[i]);
		ARRAY_DELETE(outerror[i]);
	}
	ARRAY_DELETE(input);
	ARRAY_DELETE(output);
	ARRAY_DELETE(inerror);
	ARRAY_DELETE(outerror);
}

void Pooling::FirstInput(float el, unsigned int ElNum) {
	for (unsigned int i = 0; i < PoolNum; i++)InputEl(el - 0.5f, i, ElNum);
}

void Pooling::Input(float *inArr, unsigned int arrNum) {
	memcpy(input[arrNum], inArr, sizeof(float) * Width * Height);
}

void Pooling::InputEl(float el, unsigned int arrNum, unsigned int ElNum) {
	input[arrNum][ElNum] = el;
}

void Pooling::ForwardPropagation() {
	for (unsigned int k = 0; k < PoolNum; k++) {
		for (unsigned int j = 0; j < Height - OddNumHei; j += POOL) {
			for (unsigned int i = 0; i < Width - OddNumWid; i += POOL) {
				float tmp = 0;
				float tmp2 = 0;
				int errorX = -1;
				int errorY = -1;
				for (unsigned int j1 = j; j1 < j + POOL; j1++) {
					for (unsigned int i1 = i; i1 < i + POOL; i1++) {
						//POOL*POOL内でMAX値をoutput
						tmp2 = input[k][Width * j1 + i1];
						if (tmp < tmp2) {
							tmp = tmp2;
							//outputした位置を記録
							//逆伝搬時その位置に戻す,それ以外は0とする
							if (errorX != -1) {
								outerror[k][Width * errorY + errorX] = 0.0f;
							}
							outerror[k][Width * j1 + i1] = 1.0f;
							errorX = i1;
							errorY = j1;
						}
						else {
							outerror[k][Width * j1 + i1] = 0.0f;
						}
					}
				}
				output[k][(Width / POOL) * (j / POOL) + (i / POOL)] = tmp / 2.0f;//調整方法を考える
			}
		}
	}
}

float *Pooling::Output(unsigned int arrNum) {
	return output[arrNum];
}

float Pooling::OutputEl(unsigned int arrNum, unsigned int ElNum) {
	return output[arrNum][ElNum];
}

void Pooling::InputError(float *inArr, unsigned int arrNum) {
	memcpy(inerror[arrNum], inArr, sizeof(float) * (Width / POOL) * (Height / POOL));
}

void Pooling::InputErrorEl(float el, unsigned int arrNum, unsigned int ElNum) {
	inerror[arrNum][ElNum] = el;
}

void Pooling::BackPropagation() {
	for (unsigned int k = 0; k < PoolNum; k++) {
		for (unsigned int j = 0; j < Height - OddNumHei; j += POOL) {
			for (unsigned int i = 0; i < Width - OddNumWid; i += POOL) {
				for (unsigned int j1 = j; j1 < j + POOL; j1++) {
					for (unsigned int i1 = i; i1 < i + POOL; i1++) {
						//Max値位置を探索しその位置に下層からの誤差を上書き
						if (outerror[k][Width * j1 + i1] == 1.0f) {
							outerror[k][Width * j1 + i1] = inerror[k][(Width / POOL) * (j / POOL) + (i / POOL)];
						}
					}
				}
			}
		}
	}
}

float *Pooling::GetError(unsigned int arrNum) {
	return outerror[arrNum];
}

float Pooling::GetErrorEl(unsigned int arrNum, unsigned int ElNum) {
	return outerror[arrNum][ElNum];
}

unsigned int Pooling::GetOutWidth() {
	return OutWid;
}

unsigned int Pooling::GetOutHeight() {
	return OutHei;
}