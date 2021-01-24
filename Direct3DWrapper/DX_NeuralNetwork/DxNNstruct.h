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

//Optimizer
struct CONSTANT_BUFFER_Optimizer {
	CoordTf::VECTOR4 Lear_b1_b2_eps;//x:学習率, y:減衰率b1. z:減衰率b2, w:発散防止パラeps
};

//Activation
struct CONSTANT_BUFFER_Activation {
	CoordTf::VECTOR4 Target[MAX_OUTPUT_NUM];//.xのみ
	float ActivationAlpha;
	UINT NumNode;
};

//NeuralNetwork
struct CONSTANT_BUFFER_NeuralNetwork {
	CoordTf::VECTOR4 Lear_Depth_inputS;//未使用:x, 処理中深さ:y, MaxDepth:z, inputSet数:w
	CoordTf::VECTOR4 NumNode[MAX_DEPTH_NUM];//各層のNode数:x, gNode,gError各層開始インデックス:y
	CoordTf::VECTOR4 NumWeight[MAX_DEPTH_NUM - 1];//gWeight各層開始インデックス:x
};

//Pooling
struct CONSTANT_BUFFER_Pooling {
	CoordTf::VECTOR4 WidHei;//Width:x, Height:y, MapPool:z
};

//Convolution
struct CONSTANT_BUFFER_Convolution {
	CoordTf::VECTOR4 WidHei;//入力w:x, 入力h:y ,FilNum:z
	CoordTf::VECTOR4 filWid_filStep;//Filwid数:x, Filstep数:y
	CoordTf::VECTOR4 Lear_inputS;//未使用:x, inputSet数:y, 未使用:z
};

//NN用textureコピー
struct NNCBTexture {
	CoordTf::VECTOR4 Wid_Hei;
};

//リソース拡大縮小
struct CBResourceCopy {
	UINT NumNode;
};

//SearchPixel
struct CBSearchPixel {
	CoordTf::VECTOR4 InWH_OutWH;//x:入力W, y:入力h, z:InPixCS出力(index配列数)w, w:h
	CoordTf::VECTOR4 seaWH_step_PDNum;//x:探索ブロック1個w, y:h, z:step, w:SearchPixelData配列数
	CoordTf::VECTOR4 Threshold;//x:閾値
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
