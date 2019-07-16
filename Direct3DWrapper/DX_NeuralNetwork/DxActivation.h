//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　  DxActivation                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxActivation_Header
#define Class_DxActivation_Header

#include "DxNNCommon.h"
#define AC_SHADER_NUM 2

enum ActivationName {
	CrossEntropySigmoid,
	Sigmoid,
	ReLU,
	ELU,
	Tanh
};

class DxActivation :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[AC_SHADER_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS[AC_SHADER_NUM] = { nullptr };

	CONSTANT_BUFFER_Activation cb;
	ConstantBuffer<CONSTANT_BUFFER_Activation>* mObjectCB = nullptr;

	UINT NumNode = 0;
	float* output = nullptr;
	float* outerr = nullptr;
	float crossEntropyError = 0.0f;
	int* shaderThreadNum = nullptr;

	DxActivation() {};
	void CopyOutputResourse();
	void CopyOutErrResourse();

public:
	DxActivation(UINT numNode, UINT inputsetnum = 1);
	~DxActivation();
	void ComCreate(ActivationName name);
	void SetActivationAlpha(float alpha);
	void SetTarget(float* target);
	void SetTargetEl(float el, UINT ElNum);
	void ForwardPropagation(UINT inputsetnum);
	void comCrossEntropyError();
	float GetcrossEntropyError();
	void BackPropagation();
	void SetInputResource(ID3D12Resource* res);
	void SetInErrorResource(ID3D12Resource* res);
	ID3D12Resource* GetOutputResource();
	ID3D12Resource* GetOutErrorResource();
	void GetOutput(float* out, UINT inputsetInd = 0);
	float GetOutputEl(UINT ElNum, UINT inputsetInd = 0);
	void GetOutErr(float* out, UINT inputsetInd = 0);
	float GetOutErrEl(UINT ElNum, UINT inputsetInd = 0);
};

#endif