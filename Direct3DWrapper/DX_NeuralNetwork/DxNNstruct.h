//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         DxNNstruct.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNNstruct_Header
#define Class_DxNNstruct_Header

#include "../DX_3DCG/Dx12ProcessCore.h"

//NeuralNetwork
struct CONSTANT_BUFFER_NeuralNetwork {
	VECTOR4 Lear_Depth;//学習率:x, 処理中深さ:y, MaxDepth:z
	VECTOR4 NumNode[5];//各層のNode数:x, gNode,gError各層開始インデックス:y
	VECTOR4 NumWeight[4];//gWeight各層開始インデックス:x
	VECTOR4 Target[10];//target値:x
};

//Pooling
struct CONSTANT_BUFFER_Pooling {
	VECTOR4 WidHei;//Width:x, Height:y, MapPool:z
};

//Convolution
struct CONSTANT_BUFFER_Convolution {
	VECTOR4 WidHei;//MaxFilNum:z
	VECTOR4 filWid_filStep;
	VECTOR4 Lear;//学習率:x
};

//NN用textureコピー
struct NNCBTexture {
	VECTOR4 Wid_Hei;
};

//SearchPixel
struct CBSearchPixel {
	VECTOR4 InWH_OutWH;//x:入力W, y:入力h, z:InPixCS出力w, w:h
	VECTOR4 seaWH_step_PDNum;//x:探索ブロック1個w, y:h, z:step, w:SearchPixelData配列数
	VECTOR4 Threshold;//x:閾値
};

struct SearchPixelData {
	UINT stW;
	UINT stH;
	UINT enW;
	UINT enH;
};

#endif
