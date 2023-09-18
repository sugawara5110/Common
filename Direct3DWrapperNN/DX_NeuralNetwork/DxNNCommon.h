//*****************************************************************************************//
//**                                                                                     **//
//**                   �@  �@�@�@ DxNNCommon                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxNNCommon_Header
#define Class_DxNNCommon_Header

#include "../MicroSoftLibrary/d3dx12.h"
#include "DxNNstruct.h"
#define Copy_SHADER_NUM 2

class DxNNCommon :public DxCommon {

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTextureBuffer = nullptr;
	NNCBTexture cb2;
	ConstantBuffer<NNCBTexture>* mObjectCB2 = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS2 = nullptr;
	int* shaderThreadNum2 = nullptr;
	UINT maxThreadNum = 32;//�V�F�[�_�[�̐ݒ�X���b�h��
	UINT inputSetNum;
	UINT inputSetNumCur;

	UINT texWid;
	UINT texHei;
	bool created = false;

	//���\�[�X�g��k��
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom2Copy = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom2Copy[Copy_SHADER_NUM] = { nullptr };
	CBResourceCopy cb2Copy;
	ConstantBuffer<CBResourceCopy>* mObjectCB2Copy = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pCS2Copy[Copy_SHADER_NUM] = { nullptr };

	Dx_CommandManager* cMa;
	Dx_CommandListObj* d;
	ID3D12GraphicsCommandList* CList;

	DxNNCommon();//�O������̃I�u�W�F�N�g�����֎~
	DxNNCommon(const DxNNCommon& obj) {}     // �R�s�[�R���X�g���N�^�֎~
	void operator=(const DxNNCommon& obj) {}// ������Z�q�֎~

	void TextureCopy(ID3D12Resource* texture, int comNo);
	void CreateResourceDef(Microsoft::WRL::ComPtr<ID3D12Resource>& def, UINT64 size);
	void CreateResourceUp(Microsoft::WRL::ComPtr<ID3D12Resource>& up, UINT64 size);
	void CreateResourceRead(Microsoft::WRL::ComPtr<ID3D12Resource>& re, UINT64 size);
	void SubresourcesUp(void* pData, UINT num, Microsoft::WRL::ComPtr<ID3D12Resource>& def,
		Microsoft::WRL::ComPtr<ID3D12Resource>& up);
	void CreateReplaceArr(int** shaderThreadNum, char*** replaceArr, UINT arrNum, UINT* srcNumArr);
	//shaderThreadNum, replaceArr�͊֐��O���ŉ��
	void ReplaceString(char** destination, char* source, char placeholderStartPoint, char** replaceArr);
	//destination�͊֐��O���ŉ��
	void CopyResource(ID3D12Resource* dest, ID3D12Resource* src);

	D3D12_ROOT_PARAMETER setSlotRootParameter(
		UINT ShaderRegister,
		D3D12_ROOT_PARAMETER_TYPE type = D3D12_ROOT_PARAMETER_TYPE_UAV,
		D3D12_DESCRIPTOR_RANGE* uavTable = nullptr,
		UINT NumDescriptorRanges = 1);

public:
	void CreareNNTexture(UINT width, UINT height, UINT num);
	ID3D12Resource* GetNNTextureResource();
	D3D12_RESOURCE_STATES GetNNTextureResourceStates();
	void SetMaxThreadNum(UINT num);
	~DxNNCommon();
};

#endif
