//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         Waveクラス                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Wave_Header
#define Class_Wave_Header

#include "../Direct3DWrapper/DX_3DCG/Core/Dx_BasicPolygon.h"

class Wave :public BasicPolygon {

protected:
	struct CONSTANT_BUFFER_WAVE {
		CoordTf::VECTOR4 wHei_mk012 = {};//x:waveHeight, y:mk0, z:mk1, w:mk2
		CoordTf::VECTOR2 gDisturbIndex = {};//波紋中心
		float gDisturbMag = 0.0f;
		float speed = 0.0f;
	};
	struct Step {
		float disturbStep = 0.0f;
		float updateStep = 0.0f;
		int waveNo = 0;
	};

	float time0 = 0.0f;
	float time1 = 0.0f;

	float mk[3] = {};
	ComPtr<ID3D12RootSignature> mRootSignatureCom = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDescHeapCom = nullptr;
	ConstantBuffer<CONSTANT_BUFFER_WAVE>* mObjectCB_WAVE = nullptr;
	CONSTANT_BUFFER_WAVE cbw[2] = {};
	Step step[2] = {};
	CONSTANT_BUFFER2 sg;
	ID3DBlob* cs[3] = {};

	int div;//分割数
	int width;
	float TimeStep;

	D3D12_CPU_DESCRIPTOR_HANDLE descHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mSinInputHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mInputHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mOutputHandleGPU;
	D3D12_GPU_DESCRIPTOR_HANDLE mPrevInputHandleGPU;
	ComPtr<ID3D12PipelineState> mPSOCom[3] = {};
	ComPtr<ID3D12Resource> mInputUploadBufferSin = nullptr;
	ComPtr<ID3D12Resource> mInputBufferSin = nullptr;
	ComPtr<ID3D12Resource> mInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mPrevInputBuffer = nullptr;
	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	ComPtr<ID3D12Resource> mInputBufferUp = nullptr;
	ComPtr<ID3D12Resource> mPrevInputBufferUp = nullptr;
	ComPtr<ID3D12Resource> mOutputBufferUp = nullptr;
	DivideArr divArr[16] = {};
	int numDiv;

	VertexM* ver = nullptr;
	UINT* index = nullptr;

	void createShader();
	void GetShaderByteCode(bool smooth);
	bool ComCreateSin(int comIndex);
	bool ComCreateRipples(int comIndex);
	bool ComCreate(int comIndex);
	bool setDescHeap(int comIndex, const int numSrvTex, const int numSrvTex2, const int numCbv);
	bool DrawCreate(int comIndex, int texNo, int nortNo, bool blend, bool alpha, bool smooth, float divideBufferMagnification);
	void Compute(int comIndex);

public:
	Wave();
	~Wave();
	void GetVBarray(int numMaxInstance);

	void SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
		float amR = 0.0f, float amG = 0.0f, float amB = 0.0f);

	void setMaterialType(MaterialType type);

	void setPointLight(int InstanceIndex, bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	void setPointLightAll(bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	bool Create(int comIndex, int texNo, bool blend, bool alpha, float waveHeight, int divide,
		bool smooth = true, float TimeStep = 0.03f);

	bool Create(int comIndex, int texNo, int nortNo, bool blend, bool alpha, float waveHeight, int divide,
		bool smooth = true, float TimeStep = 0.03f, float divideBufferMagnification = 1.0f);

	void SetVertex(Vertex* vertexArr, int numVer, UINT* ind, int numInd);

	void Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color);

	void InstancingUpdate(int waveNo, float speed, float disp, float SmoothRange,
		int DisturbX, int DisturbY,
		float gDisturbMag = 1.5f, float disturbStep = 0.00005f, float updateStep = 0.1f, float damping = 0.2f,
		float SmoothRatio = 0.999f, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void Update(int waveNo, float speed,
		CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
		float disp, float SmoothRange,
		int DisturbX, int DisturbY,
		float gDisturbMag = 1.5f, float disturbStep = 0.00005f, float updateStep = 0.1f, float damping = 0.2f,
		float SmoothRatio = 0.999f, float shininess = 4.0f,
		float px = 1.0f, float py = 1.0f, float mx = 1.0f, float my = 1.0f);

	void DrawOff();
	void Draw(int comIndex);
	void StreamOutput(int comIndex);
	void CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index = 0);
	void TextureInit(int width, int height, int index = 0);
	HRESULT SetTextureMPixel(int comIndex, BYTE* frame, int index = 0);
	void setDivideArr(DivideArr* arr, int numdiv) {
		numDiv = numdiv;
		memcpy(divArr, arr, sizeof(DivideArr) * numDiv);
	}
	ParameterDXR* getParameter() { return BasicPolygon::getParameter(); }
	void UpdateDxrDivideBuffer() {
		BasicPolygon::UpdateDxrDivideBuffer();
	}
	void setRefractiveIndex(float index) {
		BasicPolygon::setRefractiveIndex(index);
	}
	void SetName(char* name) { BasicPolygon::SetName(name); }
};

#endif
