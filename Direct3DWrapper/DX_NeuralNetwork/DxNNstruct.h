//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@         DxNNstruct.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNNstruct_Header
#define Class_DxNNstruct_Header

#include "../DX_3DCG/Dx12ProcessCore.h"

#define MAX_DEPTH_NUM 32
#define MAX_OUTPUT_NUM 255

//Activation
struct CONSTANT_BUFFER_Activation {
	VECTOR4 Target[MAX_OUTPUT_NUM];//.x�̂�
	float ActivationAlpha;
	UINT NumNode;
};

//NeuralNetwork
struct CONSTANT_BUFFER_NeuralNetwork {
	VECTOR4 Lear_Depth_inputS;//�w�K��:x, �������[��:y, MaxDepth:z, inputSet��:w
	VECTOR4 NumNode[MAX_DEPTH_NUM];//�e�w��Node��:x, gNode,gError�e�w�J�n�C���f�b�N�X:y
	VECTOR4 NumWeight[MAX_DEPTH_NUM - 1];//gWeight�e�w�J�n�C���f�b�N�X:x
};

//Pooling
struct CONSTANT_BUFFER_Pooling {
	VECTOR4 WidHei;//Width:x, Height:y, MapPool:z
};

//Convolution
struct CONSTANT_BUFFER_Convolution {
	VECTOR4 WidHei;//MaxFilNum:z
	VECTOR4 filWid_filStep;
	VECTOR4 Lear_inputS;//�w�K��:x, inputSet��:y, bias�w�K��:z
};

//NN�ptexture�R�s�[
struct NNCBTexture {
	VECTOR4 Wid_Hei;
};

//���\�[�X�g��k��
struct CBResourceCopy {
	UINT NumNode;
};

//SearchPixel
struct CBSearchPixel {
	VECTOR4 InWH_OutWH;//x:����W, y:����h, z:InPixCS�o��(index�z��)w, w:h
	VECTOR4 seaWH_step_PDNum;//x:�T���u���b�N1��w, y:h, z:step, w:SearchPixelData�z��
	VECTOR4 Threshold;//x:臒l
};

struct SearchPixelData {
	UINT stW;
	UINT stH;
	UINT enW;
	UINT enH;
};

struct CBGradCAM {
	UINT NumFil;
	UINT SizeFeatureMapW;
	UINT SizeFeatureMapH;
	UINT NumConvFilElement;
	UINT NumFeatureMapW;
	UINT NumFeatureMapH;
	UINT srcWid;
	UINT srcHei;
	UINT srcConvMapW;
	UINT srcConvMapH;
	UINT MapSlide;
	float SignalStrength;
};

#endif
