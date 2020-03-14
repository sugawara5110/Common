//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PostEffectクラス                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "Dx12ProcessCore.h"

class PostEffect :public Common {

protected:
	ID3DBlob *cs = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;//パイプラインOBJ
	ComPtr<ID3D12DescriptorHeap> mUavHeap = nullptr;
	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	ConstantBuffer<CONSTANT_BUFFER_PostMosaic> *mObjectCB = nullptr;

	bool ComCreate(int no);
	void Compute(bool On, int size, float blurX, float blurY, float blurLevel);

public:
	PostEffect();
	~PostEffect();
	bool ComCreateMosaic();
	bool ComCreateBlur();
	void ComputeMosaic(bool On, int size);
	void ComputeBlur(bool On, float blurX, float blurY, float blurLevel);
};

#endif
