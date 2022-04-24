//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PostEffectクラス                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "../Direct3DWrapper/DX_3DCG/Core/Dx12ProcessCore.h"

class PostEffect :public Common {

protected:
	//ポストエフェクト
	struct CONSTANT_BUFFER_PostMosaic {
		CoordTf::VECTOR4 mosaicSize;//x:mosaicSize
		CoordTf::VECTOR4 blur;//xy:座標, z:強さ, w:ピントが合う深さ
		float focusRange;
	};

	ID3DBlob* cs = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12PipelineState> mPSOCom[3] = {};
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE mDepthHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mInputHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mOutputHandleGPU;

	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	ConstantBuffer<CONSTANT_BUFFER_PostMosaic>* mObjectCB = nullptr;

	void createShader();
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
	bool ComCreateBloom(float GaussianType = false);
	void ComputeMosaic(int com_no, bool On, int size);
	void ComputeBlur(int com_no, bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(int com_no, bool On, float blurLevel, float focusDepth, float focusRange);
	void ComputeMosaic(bool On, int size);
	void ComputeBlur(bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(bool On, float blurLevel, float focusDepth, float focusRange);
	void ComputeBloom(int com_no, bool On, float BloomStrength = 1.0f, float ThresholdLuminance = 0.7f);
};

#endif
