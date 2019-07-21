//*****************************************************************************************//
//**                                                                                     **//
//**                              DxConvolution                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxConvolution_Header
#define Class_DxConvolution_Header

#include "DxNNCommon.h"
#include "DxActivation.h"
#include "DxOptimizer.h"
#define CN_SHADER_NUM 7

class DxConvolution :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[CN_SHADER_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorBuffer2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mFilterUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mFilterBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mFilterReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradientBuffer = nullptr;//勾配
	Microsoft::WRL::ComPtr<ID3D12Resource> mBiasBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBiasUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBiasReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGradBiasBuffer = nullptr;//bias勾配

	CONSTANT_BUFFER_Convolution cb;
	ConstantBuffer<CONSTANT_BUFFER_Convolution>* mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[CN_SHADER_NUM] = { nullptr };
	int* shaderThreadNum = nullptr;
	bool DeconvolutionMode = false;
	struct shaderStep {
		UINT step = 1;
		UINT width = 0;
		UINT height = 0;
	};
	shaderStep sstep[CN_SHADER_NUM + 1];

	UINT Width; //入力画像サイズ
	UINT Height;//入力画像サイズ
	UINT OutWid;//出力画像サイズ
	UINT OutHei;//出力画像サイズ
	bool firstIn = false;

	UINT elNumWid = 3;//奇数のみMax7
	UINT ElNum = 9;  //elNumWid * elNumWid
	UINT FilNum = 1;
	UINT filterStep = 1;//2の累乗のみ,Max8まで

	UINT filSize = 0;
	UINT input_outerrOneNum = 0;
	UINT output_inerrOneNum = 0;
	UINT64 input_outerrOneSize = 0;
	UINT64 output_inerrOneSize = 0;
	UINT Numdrop = 0;
	float* dropout = nullptr;
	float dropThreshold = 0.0f;//閾値

	//畳込みフィルター
	float* fil = nullptr;
	//畳込み前
	float* input = nullptr;
	//畳込み後
	float* output = nullptr;
	//下層からの誤差
	float* inputError = nullptr;
	//下層からの誤差ウエイト計算後上層に送る時に使用
	float* outputError = nullptr;
	//バイアス  フィルターと同数
	float* bias = nullptr;

	DxActivation* ac = nullptr;
	DxOptimizer* optFil = nullptr;
	DxOptimizer* optBias = nullptr;

	DxConvolution() {}
	void setshaderStep(UINT index);
	void SetGPUVirtualAddressExpIn();
	void SetGPUVirtualAddressExpErr();
	void SetGPUVirtualAddress();
	void ForwardPropagation();
	void BackPropagation0();
	void BackPropagation();
	void InputResourse();
	void InputErrResourse();
	void CopyOutputResourse();
	void CopyOutputErrResourse();
	void CopyFilterResourse();
	void CopyBiasResourse();
	void SetDropOut();
	void SetWeightInitXavier(float rate);
	void SetWeightInitHe(float rate);

	void TestFilter();
	void TestInput();
	void TestInErr();
	void TestOutput();
	void TestOutErr();

public:
	DxConvolution(UINT width, UINT height, UINT filNum, bool DeconvolutionMode = false, UINT inputsetnum = 1, UINT elnumwid = 3, UINT filstep = 1);
	~DxConvolution();
	void ComCreate(ActivationName node, OptimizerName optName = SGD, float wInit = 1.0f);
	void SetActivationAlpha(float alpha);
	void SetdropThreshold(float Threshold);//検出時は0.0f設定にする
	void Query();
	void BackPropagationNoWeightUpdate();
	void Training();
	void Detection(UINT inputsetnum);
	void Test();
	void setOptimizerParameterFil(float LearningRate = 0.001f, float AttenuationRate1 = 0.9f,
		float AttenuationRate2 = 0.999f, float DivergencePrevention = 0.00000001f);
	void setOptimizerParameterBias(float LearningRate = 0.001f, float AttenuationRate1 = 0.9f,
		float AttenuationRate2 = 0.999f, float DivergencePrevention = 0.00000001f);
	void FirstInput(float el, UINT ElNum, UINT inputsetInd = 0);
	void Input(float* inArr, UINT arrNum, UINT inputsetInd = 0);
	void InputEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void InputError(float* inArr, UINT arrNum, UINT inputsetInd = 0);
	float* Output(UINT arrNum, UINT inputsetInd = 0);
	float OutputEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	float OutputFilter(UINT arrNum, UINT ElNum);
	float* GetError(UINT arrNum, UINT inputsetInd = 0);
	float GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void SetInputResource(ID3D12Resource* res);
	void SetInErrorResource(ID3D12Resource* res);
	ID3D12Resource* GetOutErrorResource();
	ID3D12Resource* GetOutputResource();
	ID3D12Resource* GetFilter();
	ID3D12Resource* GetGradient();
	UINT GetOutWidth();
	UINT GetOutHeight();
	void SaveData(UINT Num, char* pass = "save/save%d.da");
	void LoadData(UINT Num, char* pass = "save/save%d.da");
	void SaveData(char* pass);
	void LoadData(char* pass);
};

#endif