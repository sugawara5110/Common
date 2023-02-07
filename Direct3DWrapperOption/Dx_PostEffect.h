//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PostEffectクラス                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "../Direct3DWrapper/DX_3DCG/Dx_PolygonData.h"

class PostEffect :public DxCommon {

protected:
	//ポストエフェクト
	struct CONSTANT_BUFFER_PostMosaic {
		CoordTf::VECTOR4 mosaicSize;//x:mosaicSize
		CoordTf::VECTOR4 blur;//xy:座標, z:強さ, w:ピントが合う深さ
		float focusRange;
	};

	ID3DBlob* cs = nullptr;

	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE mDepthHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mInputHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mOutputHandleGPU;

	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;

	ConstantBuffer<CONSTANT_BUFFER_PostMosaic>* mObjectCB = nullptr;

	void createShader();
	bool ComCreate(int comIndex, int no);
	void Compute(int comIndex, bool On, int size,
		float blurX, float blurY, float blurLevel,
		float focusDepth, float focusRange);

public:
	PostEffect();
	~PostEffect();
	bool ComCreateMosaic(int comIndex);
	bool ComCreateBlur(int comIndex);
	bool ComCreateDepthOfField(int comIndex);
	void ComputeMosaic(int comIndex, bool On, int size);
	void ComputeBlur(int comIndex, bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(int comIndex, bool On, float blurLevel, float focusDepth, float focusRange);
};

#endif
