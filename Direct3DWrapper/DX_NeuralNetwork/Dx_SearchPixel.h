//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　 SearchPixel                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SearchPixel_Header
#define Class_SearchPixel_Header

#include "DxNNCommon.h"

class SearchPixel :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[2] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInPixPosUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInPixPosBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutIndUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutIndBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNNOutputUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNNOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputColUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputColBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[2] = { nullptr };

	CBSearchPixel cb;
	ConstantBuffer<CBSearchPixel> *mObjectCB = nullptr;
	SearchPixelData *sdata;
	float *outInd;

	UINT srcWidth;
	UINT srcHeight;
	float *srcPix = nullptr;
	UINT seaWid;//検出範囲1ブロックwidth
	UINT seaHei;//検出範囲1ブロックheight
	float *seaPix = nullptr;//検出ブロック分割後ピクセル配列
	UINT outWid;//分割後全widthサイズ
	UINT outHei;//分割後全heightサイズ
	UINT Step;
	UINT searchNum;//検出数
	UINT64 insize;
	UINT64 outsize;
	UINT outIndNum;
	UINT outIndW;
	UINT outIndH;
	UINT64 outIndSize;//分割後配列Indexサイズ
	UINT outNum;//検出数

	SearchPixel() {}

public:
	SearchPixel(UINT srcwid, UINT srchei, UINT seawid, UINT seahei, float outscale, UINT step, UINT outNum, float Threshold);//UINT outNumは現状1しか対応してない
	~SearchPixel();
	void SetCommandList(int no);
	UINT GetSearchNum();
	void ComCreate();
	void SetPixel(float *pi);
	void SetPixel3ch(ID3D12Resource *pi);
	void SetPixel3ch(BYTE *pi);
	void SetNNoutput(float *in);
	void SetNNoutput(ID3D12Resource *in);
	void SeparationTexture();
	void TextureDraw();
	ID3D12Resource *GetOutputResource();
	UINT GetOutWid();
	UINT GetOutHei();
	float GetOutputEl(UINT Num);
};

#endif
