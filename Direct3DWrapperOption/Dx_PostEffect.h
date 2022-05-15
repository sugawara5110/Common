//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PostEffectクラス                                  **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PostEffect_Header
#define Class_PostEffect_Header

#include "../Direct3DWrapper/DX_3DCG/Dx_PolygonData.h"
#include "../Direct3DWrapper/DX_3DCG/Dx_SkinMesh.h"

namespace BloomFunction {

	template <typename T1, typename T2>
	void mergeSort(bool ascending, T1* srcArr, int srcSize, T2* srcArr2) {
		//ソートする配列を分割
		int topSize = (int)(srcSize * 0.5f);
		int halfSize = srcSize - topSize;
		T1* topArr = new T1[topSize];
		T1* halfArr = new T1[halfSize];
		memcpy(topArr, srcArr, topSize * sizeof(T1));
		memcpy(halfArr, &srcArr[topSize], halfSize * sizeof(T1));

		T2* topArr2 = nullptr;
		T2* halfArr2 = nullptr;
		if (srcArr2) {
			topArr2 = new T2[topSize];
			halfArr2 = new T2[halfSize];
			memcpy(topArr2, srcArr2, topSize * sizeof(T2));
			memcpy(halfArr2, &srcArr2[topSize], halfSize * sizeof(T2));
		}
		//要素1になるまで再帰
		if (topSize > 1)mergeSort(ascending, topArr, topSize, topArr2);
		if (halfSize > 1)mergeSort(ascending, halfArr, halfSize, halfArr2);

		int topIndex = 0;
		int halfIndex = 0;
		int srcIndex = 0;
		//分割した配列の比較
		//それぞれの配列は整列済みの為, 先頭から比較するだけ
		for (int i = 0; i < srcSize; i++) {
			bool sw = false;
			if (ascending) {
				sw = topArr[topIndex] < halfArr[halfIndex];//昇順
			}
			else {
				sw = topArr[topIndex] > halfArr[halfIndex];//降順
			}
			if (sw) {
				srcArr[i] = topArr[topIndex];
				if (srcArr2)srcArr2[i] = topArr2[topIndex];
				topIndex++;
				if (topSize <= topIndex) {
					srcIndex = i + 1;
					break;
				}
			}
			else {
				srcArr[i] = halfArr[halfIndex];
				if (srcArr2)srcArr2[i] = halfArr2[halfIndex];
				halfIndex++;
				if (halfSize <= halfIndex) {
					srcIndex = i + 1;
					break;
				}
			}
		}

		//余った要素を格納
		if (topSize > topIndex) {
			for (int i = srcIndex; i < srcSize; i++) {
				srcArr[i] = topArr[topIndex];
				if (srcArr2)srcArr2[i] = topArr2[topIndex];
				topIndex++;
			}
		}
		if (halfSize > halfIndex) {
			for (int i = srcIndex; i < srcSize; i++) {
				srcArr[i] = halfArr[halfIndex];
				if (srcArr2)srcArr2[i] = halfArr2[halfIndex];
				halfIndex++;
			}
		}

		delete[] topArr;
		delete[] halfArr;
		if (srcArr2) {
			delete[] topArr2;
			delete[] halfArr2;
		}
	}
}

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
	enum BloomType {
		polygonData,
		skinMesh
	};
	BloomType bType = polygonData;
	ComPtr<ID3D12Resource> mOneMeshDrawBuffer = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE BufferHandleGPU = {};
	float bloomStrength = 0.0f;
	float thresholdLuminance = 0.0f;
	void* obj = nullptr;
	int meshIndex = 0;

	void createBuffer();
	void Draw(int com_no);
};

class VariableBloom :public Common {

private:
	std::unique_ptr<BloomParameter* []> para = nullptr;
	std::unique_ptr<int[]> drawIndex = nullptr;
	std::unique_ptr<float[]> bloomStrengthArr = nullptr;
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

	void Draw(int com_no);
	void ComputeBloom(int com_no, bool dxr);
};

class PolygonDataBloom :public PolygonData {

private:
	friend BloomParameter;
	BloomParameter bpara = {};
	void prevDraw(int com_no);
	void Draw(int com_no);

public:
	PolygonDataBloom();
	void setBloomParameter(float bloomStrength, float thresholdLuminance);
	void DrawPreparation();
	BloomParameter* getBloomParameter();
};

class SkinMeshBloom :public SkinMesh, public Common {

private:
	friend BloomParameter;
	std::unique_ptr<BloomParameter[]> bpara = nullptr;
	void prevDraw(int com_no, int index);
	void Draw(int com_no, int index);

public:
	void GetBuffer(int numMaxInstance, float end_frame, bool singleMesh = false, bool deformer = true);
	void setBloomParameter(int index, float bloomStrength, float thresholdLuminance);
	void DrawPreparation();
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
