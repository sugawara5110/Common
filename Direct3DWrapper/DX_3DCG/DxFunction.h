//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	         �ėp�֐�, �f�[�^�^.h                            **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef DxFunction_Header
#define DxFunction_Header

#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MATRIX {
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};
};

struct VECTOR4{
	float x;
	float y;
	float z;
	float w;

	void as(float x1, float y1, float z1, float w1){
		x = x1;
		y = y1;
		z = z1;
		w = w1;
	}
};

struct VECTOR3{
	float x;
	float y;
	float z;

	void as(float x1, float y1, float z1){
		x = x1;
		y = y1;
		z = z1;
	}
};

struct VECTOR2{
	float x;
	float y;

	void as(float x1, float y1){
		x = x1;
		y = y1;
	}
};

//�s�񏉊���
void MatrixIdentity(MATRIX *mat);
//�g��k��
void MatrixScaling(MATRIX *mat, float sizex, float sizey, float sizez);
//x����]
void MatrixRotationX(MATRIX *mat, float theta);
//y����]
void MatrixRotationY(MATRIX *mat, float theta);
//z����]
void MatrixRotationZ(MATRIX *mat, float theta);
//���s�ړ�
void MatrixTranslation(MATRIX *mat, float movx, float movy, float movz);
//�s�񑫂��Z
void MatrixAddition(MATRIX *mat, MATRIX *mat1, MATRIX *mat2);
//�s��|���Z
void MatrixMultiply(MATRIX *mat, MATRIX *mat1, MATRIX *mat2);
//�]�u
void MatrixTranspose(MATRIX *mat);
//�����ϊ��s�� 1:���_�ʒu�x�N�g�� 2:�����_�ʒu�x�N�g�� 3:�J����������x�N�g��
void MatrixLookAtLH(MATRIX *mat, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3);
//�ˉe�ϊ��s��  �J�����̉�p, �A�X�y�N�g��, near�v���[��, far�v���[��
void MatrixPerspectiveFovLH(MATRIX *mat, float theta, float aspect, float Near, float Far);
//�x�N�g��3, �s��|���Z
void VectorMatrixMultiply(VECTOR3 *v, MATRIX *mat);
//�x�N�g���|���Z
void VectorMultiply(VECTOR3 *v, float f);
//�x�N�g������Z
void VectorDivision(VECTOR3 *v, float f);
//�x�N�g�������Z
void VectorAddition(VECTOR3 *out, VECTOR3 *in1, VECTOR3 *in2);
//�t�s��
bool MatrixInverse(MATRIX *invm, MATRIX *m);
//�r���[�|�[�g�ϊ��s��
void MatrixViewPort(MATRIX *mat, int width, int height);
//���K��
void Normalize(float *x, float *y, float *z, float *w);
//���K��
void VectorNormalize(VECTOR3 *out, VECTOR3 *in);
//�O��
void VectorCross(VECTOR3 *out, VECTOR3 *in1, VECTOR3 *in2);
//����
float VectorDot(VECTOR3 *in1, VECTOR3 *in2);
//���`���
void StraightLinear(MATRIX *out, MATRIX *start, MATRIX *end, float t);

#endif