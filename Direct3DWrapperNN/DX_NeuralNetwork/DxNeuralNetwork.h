//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　DxNeuralNetwork                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNeuralNetwork_Header
#define Class_DxNeuralNetwork_Header

#include "DxNNCommon.h"
#include "DxActivation.h"
#include "DxOptimizer.h"
#define NN_SHADER_NUM 3

class DxNeuralNetwork :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[NN_SHADER_NUM][MAX_DEPTH_NUM - 1] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeBuffer[MAX_DEPTH_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFUpBuffer[MAX_DEPTH_NUM - 1] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFBuffer[MAX_DEPTH_NUM - 1] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientBuffer = nullptr;//勾配
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorBuffer[MAX_DEPTH_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorReadBuffer = nullptr;

	CONSTANT_BUFFER_NeuralNetwork cb;
	ConstantBuffer<CONSTANT_BUFFER_NeuralNetwork>* mObjectCB = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS[NN_SHADER_NUM][MAX_DEPTH_NUM - 1] = { nullptr };
	int* shaderThreadNum[MAX_DEPTH_NUM - 1] = { nullptr };

	bool firstIn = false;
	float* input = nullptr;
	float* weight = nullptr;
	float* error = nullptr;
	float** dropout = nullptr;
	float* dropThreshold = nullptr;//各層閾値
	float crossEntropyError = 0.0f;
	float crossEntropyErrorTest = 0.0f;

	UINT Split = 1;
	UINT* NumNode = nullptr;//各層のノードの数
	UINT* NumNodeStIndex = nullptr;//各層のスタートindex
	UINT* NumWeight;//各層のweight数
	UINT* NumWeightStIndex = nullptr;//各層のスタートindex
	UINT weightNumAll = 0;//全数
	UINT64 weight_byteSize = 0;
	int Depth;
	DxActivation* topAc = nullptr;
	DxActivation** ac = nullptr;
	DxOptimizer* opt = nullptr;

	DxNeuralNetwork() {}
	void InputResourse();
	void ForwardPropagation();
	void BackPropagationNoWeightUpdate();
	void BackPropagation();
	void CopyWeightResourse();
	void CopyErrorResourse();
	void SetDropOut();
	void SetWeightInitXavier(float rate);
	void SetWeightInitHe(float rate);

public:
	DxNeuralNetwork(UINT* numNode, int depth, UINT split, UINT inputsetnum = 1);
	~DxNeuralNetwork();
	void ComCreate(ActivationName node, OptimizerName optName = SGD,
		ActivationName topNode = CrossEntropySigmoid, float wInit = 1.0f);
	void SetActivationAlpha(float alpha);
	void setOptimizerParameter(float LearningRate = 0.001f, float AttenuationRate1 = 0.9f,
		float AttenuationRate2 = 0.999f, float DivergencePrevention = 0.00000001f);
	void SetdropThreshold(float* ThresholdArr);//検出時は0.0f設定にする
	void SetTarget(float* target);
	void SetTargetEl(float el, UINT ElNum);
	void FirstInput(float el, UINT ElNum, UINT inputsetInd = 0);
	void InputArray(float* inArr, UINT arrNum, UINT inputsetInd = 0);
	void InputArrayEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void Query(UINT inputsetnum);
	void QueryAndBackPropagation(UINT inputsetnum);
	void Training();
	void TrainingFp();
	void TrainingBp();
	void TrainingBpNoWeightUpdate();
	float GetcrossEntropyError();
	float GetcrossEntropyErrorTest();
	void Test();
	void GetOutput(float* out, UINT inputsetInd = 0);
	float GetOutputEl(UINT ElNum, UINT inputsetInd = 0);
	float* GetError(UINT arrNum, UINT inputsetInd = 0);
	float GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void SetInputResource(ID3D12Resource* res);
	void SetInErrorResource(ID3D12Resource* res);
	float* getWeightArr();
	float getWeight(int depthIndex, int elIndex);
	ID3D12Resource* GetOutErrorResource();
	ID3D12Resource* GetOutputResource();
	void SaveData(char* pass = "save/save.da");
	void LoadData(char* pass = "save/save.da");
};

#endif