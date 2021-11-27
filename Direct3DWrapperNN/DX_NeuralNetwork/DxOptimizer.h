//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　DxOptimizer                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxOptimizer_Header
#define Class_DxOptimizer_Header

#include "DxNNCommon.h"
#define OP_SHADER_NUM 1

enum OptimizerName {
	SGD,
	ADAM
};

class DxOptimizer :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[OP_SHADER_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientMovAve1UPBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientMovAve2UPBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientMovAve1Buffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientMovAve2Buffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS[OP_SHADER_NUM] = { nullptr };

	CONSTANT_BUFFER_Optimizer cb;
	ConstantBuffer<CONSTANT_BUFFER_Optimizer>* mObjectCB = nullptr;

	UINT NumNode = 0;
	int* shaderThreadNum = nullptr;

public:
	DxOptimizer(UINT numNode);
	~DxOptimizer();
	void ComCreate(OptimizerName name = SGD);
	void setOptimizerParameter(float LearningRate = 0.001f, float AttenuationRate1 = 0.9f,
		float AttenuationRate2 = 0.999f, float DivergencePrevention = 0.00000001f);
	void SetInputGradientBuffer(ID3D12Resource* res);
	void SetInputWeightBuffer(ID3D12Resource* res);
	ID3D12Resource* GetOutputWeightBuffer();
	void comOptimizer();
};

#endif
