//*****************************************************************************************//
//**                                                                                     **//
//**                              DxConvolution                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxConvolution_Header
#define Class_DxConvolution_Header

#include "DxNNCommon.h"
#define CN_SHADER_NUM 5

class DxConvolution :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[CN_SHADER_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDropOutFBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutErrorReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mFilterUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mFilterBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mFilterReadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBiasBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBiasUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBiasReadBuffer = nullptr;

	CONSTANT_BUFFER_Convolution cb;
	ConstantBuffer<CONSTANT_BUFFER_Convolution> *mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[CN_SHADER_NUM] = { nullptr };
	int *shaderThreadNum = nullptr;

	UINT Width; //���͉摜�T�C�Y
	UINT Height;//���͉摜�T�C�Y
	UINT OutWid;//�o�͉摜�T�C�Y
	UINT OutHei;//�o�͉摜�T�C�Y
	bool firstIn = false;

	UINT elNumWid = 3;//��̂�Max7
	UINT ElNum = 9;  //elNumWid * elNumWid
	UINT FilNum = 1;
	UINT filterStep = 1;//2�̗ݏ�̂�,Max8�܂�
	UINT inputSetNum = 1;

	UINT filSize = 0;
	UINT input_outerrOneNum = 0;
	UINT output_inerrOneNum = 0;
	UINT64 input_outerrOneSize = 0;
	UINT64 output_inerrOneSize = 0;

	float *dropout = nullptr;
	float dropThreshold = 0.5f;//臒l

	//�􍞂݃t�B���^�[
	float *fil = nullptr;
	//�􍞂ݑO
	float *input = nullptr;
	//�􍞂݌�
	float *output = nullptr;
	//���w����̌덷
	float *inputError = nullptr;
	//���w����̌덷�E�G�C�g�v�Z���w�ɑ��鎞�Ɏg�p
	float *outputError = nullptr;
	//�o�C�A�X  �t�B���^�[�Ɠ���
	float *bias = nullptr;

	float learningRate = 0.1f;
	float learningBiasRate = 0.1f;

	DxConvolution() {}
	void ForwardPropagation(UINT inputsetnum);
	void BackPropagation();
	void InputResourse();
	void InputErrResourse();
	void CopyOutputResourse();
	void CopyOutputErrResourse();
	void CopyFilterResourse();
	void CopyBiasResourse();
	void ComCreate(bool sigon);
	void SetDropOut();

	void TestFilter();
	void TestInput();
	void TestInErr();
	void TestOutput();
	void TestOutErr();

public:
	DxConvolution(UINT width, UINT height, UINT filNum, UINT inputsetnum = 1, UINT elnumwid = 3, UINT filstep = 1);
	~DxConvolution();
	void SetCommandList(int no);
	void ComCreateSigmoid();
	void ComCreateReLU();
	void SetdropThreshold(float Threshold);//���o����0.0f�ݒ�ɂ���
	void Query();
	void Training();
	void Detection(UINT inputsetnum);
	void Test();
	void SetLearningLate(float rate, float biasrate);
	void SetWeightInit(float rate);
	void FirstInput(float el, UINT ElNum, UINT inputsetInd = 0);
	void Input(float *inArr, UINT arrNum, UINT inputsetInd = 0);
	void InputEl(float el, UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void InputError(float *inArr, UINT arrNum, UINT inputsetInd = 0);
	float *Output(UINT arrNum, UINT inputsetInd = 0);
	float OutputEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	float OutputFilter(UINT arrNum, UINT ElNum);
	float *GetError(UINT arrNum, UINT inputsetInd = 0);
	float GetErrorEl(UINT arrNum, UINT ElNum, UINT inputsetInd = 0);
	void SetInputResource(ID3D12Resource *res);
	void SetInErrorResource(ID3D12Resource *res);
	ID3D12Resource *GetOutErrorResource();
	ID3D12Resource *GetOutputResource();
	ID3D12Resource *GetFilter();
	UINT GetOutWidth();
	UINT GetOutHeight();
	void SaveData(UINT Nnm);
	void LoadData(UINT Num);
};

#endif