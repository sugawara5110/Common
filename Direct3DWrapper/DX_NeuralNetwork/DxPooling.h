//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　  DxPooling                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxPooling_Header
#define Class_DxPooling_Header

#include "DxNNCommon.h"
#define PONUM 2
#define PO_SHADER_NUM 2

class DxPooling :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[PO_SHADER_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorReadBuffer = nullptr;

	CONSTANT_BUFFER_Pooling cb;
	ConstantBuffer<CONSTANT_BUFFER_Pooling> *mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[PO_SHADER_NUM] = { nullptr };
	int *shaderThreadNum = nullptr;

	UINT Width;
	UINT Height;
	UINT OutWid;
	UINT OutHei;
	UINT OddNumWid = 0;
	UINT OddNumHei = 0;
	bool firstIn = false;
	float *input = nullptr;
	float *output = nullptr;
	float *inerror = nullptr;
	float *outerror = nullptr;

	UINT input_outerrOneNum = 0;
	UINT output_inerrOneNum = 0;
	UINT64 input_outerrOneSize = 0;
	UINT64 output_inerrOneSize = 0;
	UINT PoolNum = 1;
	UINT inputSetNum = 1;

	DxPooling() {}
	void ForwardPropagation(UINT inputsetnum);
	void BackPropagation();
	void InputResourse();
	void InputErrResourse();
	void CopyOutputResourse();
	void CopyOutputErrResourse();

public:
	DxPooling(UINT width, UINT height, UINT poolNum, UINT inputsetnum = 1);
	~DxPooling();
	void SetCommandList(int no);
	void ComCreate();
	void FirstInput(float el, UINT ElNum, UINT inputsetInd = 0);
	void Input(float *inArr, UINT arrNum, UINT inputsetInd = 0);
	void InputEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void InputError(float *inArr, UINT arrNum, UINT inputsetInd = 0);
	void InputErrorEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void Query();
	void Training();
	void Detection(UINT inputsetnum);
	void Test();
	float *Output(UINT arrNum, UINT inputsetInd = 0);
	float OutputEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	float *GetError(UINT arrNum, UINT inputsetInd = 0);
	float GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void SetInputResource(ID3D12Resource *res);
	void SetInErrorResource(ID3D12Resource *res);
	ID3D12Resource *GetOutErrorResource();
	ID3D12Resource *GetOutputResource();
	UINT GetOutWidth();
	UINT GetOutHeight();
};

#endif