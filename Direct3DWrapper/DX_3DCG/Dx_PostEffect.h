//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          PostEffect�N���X                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "Dx12ProcessCore.h"

class PostEffect :public Common {

protected:
	int                        com_no = 0;
	ID3DBlob                   *cs = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom = nullptr;//�p�C�v���C��OBJ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	UploadBuffer<CONSTANT_BUFFER_PostMosaic> *mObjectCB = nullptr;

	void ComCreate(int no);
	void Compute(bool On, int size, float blurX, float blurY, float blurLevel);

public:
	PostEffect();
	~PostEffect();
	void SetCommandList(int no);
	void ComCreateMosaic();
	void ComCreateBlur();
	void ComputeMosaic(bool On, int size);
	void ComputeBlur(bool On, float blurX, float blurY, float blurLevel);
};

#endif
