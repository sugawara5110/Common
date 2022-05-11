//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PostEffectクラス                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "../Direct3DWrapper/DX_3DCG/Dx_PolygonData.h"
#include "../Direct3DWrapper/DX_3DCG/Dx_SkinMesh.h"

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
	ComPtr<ID3D12PipelineState> mPSOCom = nullptr;
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
	bool ComCreateBloom(bool GaussianType = false, UINT num_gauss = 0, UINT* GaussSizeArr = nullptr);
	void ComputeMosaic(int com_no, bool On, int size);
	void ComputeBlur(int com_no, bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(int com_no, bool On, float blurLevel, float focusDepth, float focusRange);
	void ComputeMosaic(bool On, int size);
	void ComputeBlur(bool On, float blurX, float blurY, float blurLevel);
	void ComputeDepthOfField(bool On, float blurLevel, float focusDepth, float focusRange);
	void ComputeBloom(int com_no, bool On, float BloomStrength = 1.0f, float ThresholdLuminance = 0.7f);
};

struct BloomParameter {
	ComPtr<ID3D12Resource> mOneMeshDrawBuffer = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE BufferHandleGPU = {};
	float bloomStrength = 0.0f;
	float thresholdLuminance = 0.0f;

	void createBuffer();
};

class VariableBloom :public Common {

private:
	std::unique_ptr<BloomParameter* []> para = nullptr;
	int numPara = 0;
	ComPtr<ID3D12Resource> mMainBuffer = nullptr;
	ComPtr<ID3DBlob> cs[2] = {};
	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12PipelineState> mPSOCom[2] = {};
	ComPtr<ID3D12DescriptorHeap> mDescHeap = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE mMainHandleGPU = {};

public:
	void init(BloomParameter** arr, int numPara,
		bool GaussianType = false, UINT num_gauss = 0, UINT* GaussSizeArr = nullptr);

	void ComputeBloom(int com_no, bool dxr);
};

class PolygonDataBloom :public PolygonData {

private:
	BloomParameter bpara = {};
	void prevDraw(int com_no);

public:
	PolygonDataBloom();
	void setBloomParameter(float bloomStrength, float thresholdLuminance);
	void Draw(int com_no);
	void Draw();
	BloomParameter* getBloomParameter();
};

class SkinMeshBloom :public SkinMesh, public Common {

private:
	std::unique_ptr<BloomParameter[]> bpara = nullptr;
	void prevDraw(int com_no, int index);

public:
	void GetBuffer(int numMaxInstance, float end_frame, bool singleMesh = false, bool deformer = true);
	void setBloomParameter(int index, float bloomStrength, float thresholdLuminance);
	void Draw(int com_no);
	int getNumBloomParameter();
	BloomParameter* getBloomParameter(int index);

	void SetName(char* name);
	void SetCommandList(int no);
	void CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(int com_no, BYTE* frame, int index = 0);
	InternalTexture* getInternalTexture(int index);
	ID3D12Resource* getTextureResource(int index);
};

#endif
