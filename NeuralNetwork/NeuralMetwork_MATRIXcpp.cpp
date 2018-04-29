//*****************************************************************************************//
//**                                                                                     **//
//**                   　    NeuralNetwork_MATRIX                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "NeuralNetwork.h"

MATRIX_NN::~MATRIX_NN() {
	ARRAY_DELETE(m[0]);
	ARRAY_DELETE(m[1]);
}

void MATRIX_NN::Input(float *in, unsigned int x, unsigned int y) {
	X[0] = x;
	Y[0] = y;
	X[1] = y;
	Y[1] = x;
	m[0] = in;
	m[1] = new float[X[1] * Y[1]];
}

void MATRIX_NN::InputElement(float e, unsigned int x, unsigned int y) {
	m[0][X[0] * y + x] = e;
}

float *MATRIX_NN::GetMatrix() {
	return m[ind];
}

void MATRIX_NN::MatrixTransposeUpdate() {

	for (unsigned int j = 0; j < Y[0]; j++) {
		for (unsigned int i = 0; i < X[0]; i++) {
			m[1][X[1] * i + j] = m[0][X[0] * j + i];
		}
	}
}

void MATRIX_NN::Transpose(bool f) {
	ind = 0;
	if (f)ind = 1;
}

/*
内積
行列 と  縦ベクトル  =  縦ベクトル
[a11  a12] [b1]     [a11b1 + a12b2]
[        ] [  ]  =  [             ]
[a21  a22] [b2]     [a21b1 + a22b2]
*/
void MatrixVectorDot(float *OutVec, MATRIX_NN *mat, float *vec) {

	unsigned int y = mat->GetY();
	unsigned int x = mat->GetX();

	for (unsigned int j = 0; j < y; j++) {
		OutVec[j] = 0.0f;
		for (unsigned int i = 0; i < x; i++) {
			OutVec[j] += mat->GetMatrix()[x * j + i] * vec[i];
		}
	}
}

/*
内積
縦(列Column)ベクトルと横(行row)ベクトル  = 行列
[a1]             [a1b1  a1b2]
[  ] [b1 b2]  =  [          ]
[a2]             [a2b1  a2b2]
*/
void VectorDot(float *OutMat, float *ColVec, unsigned int colNum, float *RowVec, unsigned int rowNum) {

	for (unsigned int i = 0; i < colNum; i++) {
		for (unsigned int j = 0; j < rowNum; j++) {
			OutMat[rowNum * i + j] = ColVec[i] * RowVec[j];
		}
	}
}