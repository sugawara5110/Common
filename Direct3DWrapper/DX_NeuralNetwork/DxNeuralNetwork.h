//*****************************************************************************************//
//**                                                                                     **//
//**                   　  　　　DxNeuralNetwork                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNeuralNetwork_Header
#define Class_DxNeuralNetwork_Header

#include "DxNNCommon.h"

class DxNeuralNetwork :public DxNNCommon {

protected:
	int                        com_no = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[5] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeBuffer[5] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mNodeReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mWeightReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorBuffer[5] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mErrorReadBuffer = nullptr;

	CONSTANT_BUFFER_NeuralNetwork cb;
	ConstantBuffer<CONSTANT_BUFFER_NeuralNetwork> *mObjectCB = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS[5] = { nullptr };

	bool firstIn = false;
	float *input = nullptr;
	float *weight = nullptr;
	float *error = nullptr;
	float *output = nullptr;

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
	UINT detectionNum = 1;

	DxNeuralNetwork() {}
	void InputResourse();
	void ForwardPropagation(UINT detectionnum);
	void InverseQuery();
	void BackPropagation();
	void CopyOutputResourse();
	void CopyWeightResourse();
	void CopyErrorResourse();

public:
	DxNeuralNetwork(UINT *numNode, int depth, UINT split, UINT detectionnum = 1);
	~DxNeuralNetwork();
	void SetCommandList(int no);
	void ComCreate();
	void SetLearningLate(float rate);
	void SetTarget(float *target);
	void SetTargetEl(float el, UINT ElNum);
	void FirstInput(float el, UINT ElNum, UINT detectionInd = 0);
	void InputArray(float *inArr, UINT arrNum, UINT detectionInd = 0);
	void InputArrayEl(float el, UINT arrNum, UINT ElNum, UINT detectionInd = 0);
	void Query(UINT detectionnum);
	void Training();
	void GetOutput(float *out, UINT detectionInd = 0);
	float GetOutputEl(UINT ElNum, UINT detectionInd = 0);
	float *GetError(UINT arrNum);
	float GetErrorEl(UINT arrNum, UINT ElNum);
	void SetInputResource(ID3D12Resource *res);
	ID3D12Resource *GetOutErrorResource();
	ID3D12Resource *GetOutputResource();
	void SaveData();
	void LoadData();
};

#endif