//*****************************************************************************************//
//**                                                                                     **//
//**                   �@  �@�@�@ DxNNCommon                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNNCommon_Header
#define Class_DxNNCommon_Header

#include "DxNNstruct.h"

class DxNNCommon :public Common {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom2 = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTextureBuffer = nullptr;
	NNCBTexture cb2;
	UploadBuffer<NNCBTexture> *mObjectCB2 = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS2 = nullptr;
	UINT texWid;
	UINT texHei;
	bool created = false;

	DxNNCommon() {}//�O������̃I�u�W�F�N�g�����֎~
	DxNNCommon(const DxNNCommon &obj) {}     // �R�s�[�R���X�g���N�^�֎~
	void operator=(const DxNNCommon& obj) {}// ������Z�q�֎~

	void TextureCopy(ID3D12Resource *texture, int comNo);

public:
	void CreareNNTexture(UINT width, UINT height, UINT num);
	ID3D12Resource *GetNNTextureResource();
	D3D12_RESOURCE_STATES GetNNTextureResourceStates();
	~DxNNCommon();
};

#endif
