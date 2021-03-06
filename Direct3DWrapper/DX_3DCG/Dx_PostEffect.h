//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PostEffectクラス                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "Core/Dx12ProcessCore.h"

class PostEffect :public Common {

protected:
	ID3DBlob* cs = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;//パイプラインOBJ
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;
	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	ConstantBuffer<CONSTANT_BUFFER_PostMosaic>* mObjectCB = nullptr;

	bool ComCreate(int no);
	void Compute(int com_no, bool On, int size,
		float blurX, float blurY, float blurLevel,
		float focusDepth, float focusRange);

public:
	PostEffect();
	~PostEffect();
	bool ComCreateMosaic();
	bool ComCreateBlur();
	bool ComCreateDepthOfField();
	void ComputeMosaic(int com_no, bool On, int size);
	void ComputeBlur(int com_no, bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(int com_no, bool On, float blurLevel, float focusDepth, float focusRange);
	void ComputeMosaic(bool On, int size);
	void ComputeBlur(bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(bool On, float blurLevel, float focusDepth, float focusRange);
};

#endif
