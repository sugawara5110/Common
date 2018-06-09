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
	ID3DBlob                   *cs = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSOCom = nullptr;//パイプラインOBJ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	ConstantBuffer<CONSTANT_BUFFER_PostMosaic> *mObjectCB = nullptr;

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
