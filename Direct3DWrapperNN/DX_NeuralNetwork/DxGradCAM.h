//*****************************************************************************************//
//**                                                                                     **//
//**                   �@  �@�@�@  DxGradCAM                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxGradCAM_Header
#define Class_DxGradCAM_Header

#include "DxNNCommon.h"
#define GC_SHADER_NUM 5

class DxGradCAM :public DxNNCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom[GC_SHADER_NUM] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputFeatureMapBuffer = nullptr;//�Ώ�ConvOutPut�v�f���~�t�B���^�[��
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputGradientBuffer = nullptr;//�Ώ�Conv�t�B���^�[�v�f���~�t�B���^�[��
	//�e�t�B���^�[���Ɍ��z�̕��ς��v�Z
	Microsoft::WRL::ComPtr<ID3D12Resource> mGlobalAveragePoolingBuffer = nullptr;//�t�B���^�[��
	//FeatureMap�ɑΉ�����GAP���|���������킹��
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutGradCAMBuffer = nullptr;//�Ώ�ConvOutPut�v�f��
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutGradCAMSynthesisBuffer = nullptr;//���o�Ώۉ摜pixel��
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputColUpBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputColBuffer = nullptr;//���o�Ώۉ摜
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap = nullptr;

	CBGradCAM cb;
	ConstantBuffer<CBGradCAM>* mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> pCS[GC_SHADER_NUM] = { nullptr };

	DxGradCAM() {}

public:
	DxGradCAM(UINT SizeFeatureMapW, UINT SizeFeatureMapH, UINT NumGradientEl, UINT NumFil, UINT inputsetnum);
	~DxGradCAM();
	void ComCreate(UINT srcWid, UINT srcHei, float SignalStrength = 10.0f);
	void SetFeatureMap(ID3D12Resource* res);
	void SetGradient(ID3D12Resource* res);
	void ComGAP();
	void ComGradCAM(UINT inputsetnum);
	void SetPixel3ch(ID3D12Resource* pi);
	void SetPixel3ch(BYTE* pi);
	void GradCAMSynthesis(UINT srcConvMapW, UINT srcConvMapH, UINT MapSlide);
	ID3D12Resource* GetPixel();
};

#endif