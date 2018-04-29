//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@         DxNNstruct.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNNstruct_Header
#define Class_DxNNstruct_Header

#include "../DX_3DCG/Dx12ProcessCore.h"

//NeuralNetwork
struct CONSTANT_BUFFER_NeuralNetwork {
	VECTOR4 Lear_Depth;//�w�K��:x, �������[��:y, MaxDepth:z
	VECTOR4 NumNode[5];//�e�w��Node��:x, gNode,gError�e�w�J�n�C���f�b�N�X:y
	VECTOR4 NumWeight[4];//gWeight�e�w�J�n�C���f�b�N�X:x
	VECTOR4 Target[10];//target�l:x
};

//Pooling
struct CONSTANT_BUFFER_Pooling {
	VECTOR4 WidHei;//Width:x, Height:y, MapPool:z
};

//Convolution
struct CONSTANT_BUFFER_Convolution {
	VECTOR4 WidHei;//MaxFilNum:z
	VECTOR4 filWid_filStep;
	VECTOR4 Lear;//�w�K��:x
};

//NN�ptexture�R�s�[
struct NNCBTexture {
	VECTOR4 Wid_Hei;
};

//SearchPixel
struct CBSearchPixel {
	VECTOR4 InWH_OutWH;//x:����W, y:����h, z:InPixCS�o��w, w:h
	VECTOR4 seaWH_step_PDNum;//x:�T���u���b�N1��w, y:h, z:step, w:SearchPixelData�z��
	VECTOR4 Threshold;//x:臒l
};

struct SearchPixelData {
	UINT stW;
	UINT stH;
	UINT enW;
	UINT enH;
};

#endif
