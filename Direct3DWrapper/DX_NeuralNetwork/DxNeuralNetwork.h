//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　DxNeuralNetwork                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNeuralNetwork_Header
#define Class_DxNeuralNetwork_Header

#include "DxNNCommon.h"
#define NN_SHADER_NUM 5

class DxNeuralNetwork :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[NN_SHADER_NUM][MAX_DEPTH_NUM - 1] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeBuffer[MAX_DEPTH_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFUpBuffer[MAX_DEPTH_NUM - 1] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFBuffer[MAX_DEPTH_NUM - 1] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorBuffer[MAX_DEPTH_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorReadBuffer = nullptr;

	CONSTANT_BUFFER_NeuralNetwork cb;
	ConstantBuffer<CONSTANT_BUFFER_NeuralNetwork> *mObjectCB = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS[NN_SHADER_NUM][MAX_DEPTH_NUM - 1] = { nullptr };
	int *shaderThreadNum[MAX_DEPTH_NUM - 1] = { nullptr };

	bool firstIn = false;
	float *input = nullptr;
	float *weight = nullptr;
	float *error = nullptr;
	float *output = nullptr;
	float **dropout = nullptr;
	float *dropThreshold = nullptr;//各層閾値

	UINT Split = 1;

	float *target;
	float learningRate = 0.1f;

	UINT *NumNode = nullptr;//各層のノードの数
	UINT *NumNodeStIndex = nullptr;//各層のスタートindex
	UINT *NumWeight;//各層のweight数
	UINT *NumWeightStIndex = nullptr;//各層のスタートindex
	UINT weightNumAll = 0;//全数
	UINT64 weight_byteSize = 0;
	int Depth;
	UINT inputSetNum = 1;

	DxNeuralNetwork() {}
	void InputResourse();
	void ForwardPropagation(UINT inputsetnum);
	void InverseQuery();
	void BackPropagation();
	void CopyOutputResourse();
	void CopyWeightResourse();
	void CopyErrorResourse();
	void ComCreate(bool sigon);
	void SetDropOut();

public:
	DxNeuralNetwork(UINT *numNode, int depth, UINT split, UINT inputsetnum = 1);
	~DxNeuralNetwork();
	void SetCommandList(int no);
	void ComCreateSigmoid();
	void ComCreateReLU();
	void SetLearningLate(float rate);
	void SetdropThreshold(float *ThresholdArr);//検出時は0.0f設定にする
	void SetWeightInit(float rate);
	void SetTarget(float *target);
	void SetTargetEl(float el, UINT ElNum);
	void FirstInput(float el, UINT ElNum, UINT inputsetInd = 0);
	void InputArray(float *inArr, UINT arrNum, UINT inputsetInd = 0);
	void InputArrayEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void Query(UINT inputsetnum);
	void Training();
	void Test();
	void GetOutput(float *out, UINT inputsetInd = 0);
	float GetOutputEl(UINT ElNum, UINT inputsetInd = 0);
	float *GetError(UINT arrNum, UINT inputsetInd = 0);
	float GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void SetInputResource(ID3D12Resource *res);
	ID3D12Resource *GetOutErrorResource();
	ID3D12Resource *GetOutputResource();
	void SaveData();
	void LoadData();
};

#endif