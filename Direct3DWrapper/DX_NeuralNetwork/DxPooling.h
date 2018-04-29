//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　  DxPooling                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxPooling_Header
#define Class_DxPooling_Header

#include "DxNNCommon.h"
#define PONUM 2

class DxPooling :public DxNNCommon {

protected:
	int                        com_no = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[2] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorReadBuffer = nullptr;

	CONSTANT_BUFFER_Pooling cb;
	UploadBuffer<CONSTANT_BUFFER_Pooling> *mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[2] = { nullptr };

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
	UINT detectionNum = 1;

	DxPooling() {}
	void ForwardPropagation(UINT detectionnum);
	void BackPropagation();
	void InputResourse();
	void InputErrResourse();
	void CopyOutputResourse();
	void CopyOutputErrResourse();

public:
	DxPooling(UINT width, UINT height, UINT poolNum, UINT detectionnum = 1);
	~DxPooling();
	void SetCommandList(int no);
	void ComCreate();
	void FirstInput(float el, UINT ElNum, UINT detectionInd = 0);
	void Input(float *inArr, UINT arrNum, UINT detectionInd = 0);
	void InputEl(float el, UINT arrNum, UINT ElNum, UINT detectionInd = 0);
	void InputError(float *inArr, UINT arrNum);
	void InputErrorEl(float el, UINT arrNum, UINT ElNum);
	void Query();
	void Training();
	void Detection(UINT detectionnum);
	float *Output(UINT arrNum, UINT detectionInd = 0);
	float OutputEl(UINT arrNum, UINT ElNum, UINT detectionInd = 0);
	float *GetError(UINT arrNum);
	float GetErrorEl(UINT arrNum, UINT ElNum);
	void SetInputResource(ID3D12Resource *res);
	void SetInErrorResource(ID3D12Resource *res);
	ID3D12Resource *GetOutErrorResource();
	ID3D12Resource *GetOutputResource();
	UINT GetOutWidth();
	UINT GetOutHeight();
};

#endif