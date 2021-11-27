//*****************************************************************************************//
//**                                                                                     **//
//**                   �@  �@�@�@ SearchPixel                                            **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SearchPixel_Header
#define Class_SearchPixel_Header

#include "DxNNCommon.h"
#define SEA_SHADER_NUM 2

class SearchPixel :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[SEA_SHADER_NUM] = { nullptr };
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

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[SEA_SHADER_NUM] = { nullptr };
	int *shaderThreadNum = nullptr;

	CBSearchPixel cb;
	ConstantBuffer<CBSearchPixel> *mObjectCB = nullptr;
	SearchPixelData *sdata;
	float *outInd;

	UINT srcWidth;
	UINT srcHeight;
	float *srcPix = nullptr;
	UINT seaWid;//���o�͈�1�u���b�Nwidth
	UINT seaHei;//���o�͈�1�u���b�Nheight
	float *seaPix = nullptr;//���o�u���b�N������s�N�Z���z��
	UINT outWid;//������Swidth�T�C�Y
	UINT outHei;//������Sheight�T�C�Y
	UINT Step;
	UINT searchNum;//���o��
	UINT64 insize;
	UINT64 outsize;
	UINT outIndNum;
	UINT outIndW;
	UINT outIndH;
	UINT64 outIndSize;//������z��Index�T�C�Y
	UINT outNum;//���o��

	SearchPixel() {}

public:
	SearchPixel(UINT srcwid, UINT srchei, UINT seawid, UINT seahei, float outscale,
		UINT step, UINT outNum, float Threshold);//UINT outNum�͌���1�����Ή����ĂȂ�
	~SearchPixel();
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
