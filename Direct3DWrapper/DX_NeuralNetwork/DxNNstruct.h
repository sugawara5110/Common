//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         DxNNstruct.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNNstruct_Header
#define Class_DxNNstruct_Header

#include "../DX_3DCG/Dx12ProcessCore.h"

#define MAX_DEPTH_NUM 32
#define MAX_OUTPUT_NUM 255

//Activation
struct CONSTANT_BUFFER_Activation {
	VECTOR4 Target[MAX_OUTPUT_NUM];//.xのみ
	float ActivationAlpha;
	UINT NumNode;
};

//NeuralNetwork
struct CONSTANT_BUFFER_NeuralNetwork {
	VECTOR4 Lear_Depth_inputS;//学習率:x, 処理中深さ:y, MaxDepth:z, inputSet数:w
	VECTOR4 NumNode[MAX_DEPTH_NUM];//各層のNode数:x, gNode,gError各層開始インデックス:y
	VECTOR4 NumWeight[MAX_DEPTH_NUM - 1];//gWeight各層開始インデックス:x
};

//Pooling
struct CONSTANT_BUFFER_Pooling {
	VECTOR4 WidHei;//Width:x, Height:y, MapPool:z
};

//Convolution
struct CONSTANT_BUFFER_Convolution {
	VECTOR4 WidHei;//MaxFilNum:z
	VECTOR4 filWid_filStep;
	VECTOR4 Lear_inputS;//学習率:x, inputSet数:y, bias学習率:z
};

//NN用textureコピー
struct NNCBTexture {
	VECTOR4 Wid_Hei;
};

//リソース拡大縮小
struct CBResourceCopy {
	UINT NumNode;
};

//SearchPixel
struct CBSearchPixel {
	VECTOR4 InWH_OutWH;//x:入力W, y:入力h, z:InPixCS出力(index配列数)w, w:h
	VECTOR4 seaWH_step_PDNum;//x:探索ブロック1個w, y:h, z:step, w:SearchPixelData配列数
	VECTOR4 Threshold;//x:閾値
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
