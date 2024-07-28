//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DxrRenderer.h                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxrRenderer_Header
#define Class_DxrRenderer_Header

#include "../Core/ParameterDXR.h"
#include "../../MicroSoftLibrary/DXCAPI/dxcapi.use.h"

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> pScratch;
	ComPtr<ID3D12Resource> pResult;
	ComPtr<ID3D12Resource> pInstanceDesc;//top-level AS しか使わない
	bool firstSet = false;
};

struct DxrConstantBuffer
{
	CoordTf::MATRIX prevViewProjection;
	CoordTf::MATRIX projectionToWorld;
	CoordTf::VECTOR4 cameraPosition;
	CoordTf::VECTOR4 emissivePosition[LIGHT_PCS];//xyz:Pos, w:オンオフ
	CoordTf::VECTOR4 numEmissive;//x:Em, y:numInstance
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];//レンジ, 減衰1, 減衰2, 減衰3
	CoordTf::VECTOR4 GlobalAmbientColor;
	CoordTf::VECTOR4 emissiveNo[LIGHT_PCS];//x:emissiveNo
	CoordTf::VECTOR4 dDirection;
	CoordTf::VECTOR4 dLightColor;
	CoordTf::VECTOR4 dLightst;//x:オンオフ
	CoordTf::VECTOR4 TMin_TMax;//x, y
	CoordTf::VECTOR4 LightArea_RandNum;//x:乱数範囲area(2.0で全方向), y:乱数個数
	CoordTf::VECTOR4 frameReset_DepthRange_NorRange;//.x:フレームインデックスリセット(1.0でリセット), .y:深度レンジ, .z:法線レンジ
	UINT maxRecursion;
	UINT traceMode;
};

struct DxrMaterialCB {
	CoordTf::VECTOR4 vDiffuse = { 0.8f,0.8f,0.8f,1.0f };//ディフューズ色
	CoordTf::VECTOR4 vSpeculer = { 0.6f,0.6f,0.6f,1.0f };//スぺキュラ色
	CoordTf::VECTOR4 vAmbient = { 0.1f,0.1f,0.1f,0.0f };//アンビエント
	float shininess = 4.0f;//スペキュラ強さ
	float RefractiveIndex = 0.0f;//屈折率
	UINT materialNo = 0;
};

struct ASobj {
	ComPtr<ID3D12Resource> mpTopLevelAS;
	std::unique_ptr<AccelerationStructureBuffers[]> bottomLevelBuffers;
	AccelerationStructureBuffers topLevelBuffers;
};

struct CBobj {
	DxrConstantBuffer cb = {};
	std::unique_ptr<DxrMaterialCB[]> matCb = nullptr;
	std::unique_ptr<WVP_CB[]> wvpCb = nullptr;

	CBobj() {
		CoordTf::MatrixIdentity(&cb.prevViewProjection);
		CoordTf::MatrixIdentity(&cb.projectionToWorld);
	}
};

enum ShaderTestMode {
	Standard = 0,
	ID = 1,
	Normal = 2
};

enum TraceMode {
	ONE_RAY = 0,
	PathTracing = 1,
	NEE = 2
};

class DxrRenderer {

private:
	static const int numSwapIndex = 2;
	std::vector<ParameterDXR*> PD = {};
	ASobj asObj[numSwapIndex] = {};
	CBobj cbObj[numSwapIndex] = {};
	int buffSwap[numSwapIndex] = { 0,0 };

	float LightArea;
	int RandNum;
	UINT frameInd;
	float frameReset;
	float depthRange;
	float norRange;
	UINT traceMode = 0;

	ConstantBuffer<DxrConstantBuffer>* sCB;
	ConstantBuffer<DxrMaterialCB>* material;
	ConstantBuffer<WVP_CB>* wvp;

	ComPtr<ID3D12StateObject> mpPipelineState;
	ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	ComPtr<ID3D12Resource> mpShaderTable[numSwapIndex];
	Dx_Resource mpOutputResource = {};
	Dx_Resource mpDepthResource = {};
	Dx_Resource mpInstanceIdMapResource = {};
	Dx_Resource frameIndexMap = {};
	Dx_Resource normalMap = {};
	Dx_Resource mpPrevDepthResource = {};
	Dx_Resource prev_normalMap = {};

	ComPtr<ID3D12DescriptorHeap> mpSrvUavCbvHeap[numSwapIndex];

	uint32_t numLocalHeap = 0;
	uint32_t numGlobalHeap = 0;

	D3D12_GPU_DESCRIPTOR_HANDLE LocalHandleRay[numSwapIndex];
	D3D12_GPU_DESCRIPTOR_HANDLE LocalHandleHit[numSwapIndex];
	D3D12_GPU_DESCRIPTOR_HANDLE GlobalHandle[numSwapIndex];
	D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle;

	ComPtr<ID3D12DescriptorHeap> mpSamplerHeap;
	D3D12_DISPATCH_RAYS_DESC raytraceDesc[numSwapIndex] = {};

	UINT numMaterial = 0;//全マテリアル数
	UINT maxRecursion = 1;
	UINT maxNumInstancing = 0;
	float TMin = 0;
	float TMax = 0;

	void createBottomLevelAS1(Dx_CommandListObj* com, VertexView* vv,
		IndexView* iv, UINT currentIndexCount, UINT MaterialNo,
		bool update, bool tessellation, bool alphaTest);
	void createBottomLevelAS(Dx_CommandListObj* com);
	void createTopLevelAS(Dx_CommandListObj* com);
	ComPtr<ID3D12RootSignature> createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc);
	void createAccelerationStructures();
	void createRtPipelineState(ShaderTestMode Mode);
	void createShaderResources();
	void createShaderTable();

	void updateMaterial(CBobj* cbObj);
	void updateCB(CBobj* cbObj, UINT numRecursion);
	void updateAS(Dx_CommandListObj* com, UINT numRecursion);
	void setCB();
	void raytrace(Dx_CommandListObj* com);

public:
	~DxrRenderer();
	void initDXR(std::vector<ParameterDXR*>& pd, UINT maxRecursion, ShaderTestMode Mode = Standard);
	void setTMin_TMax(float TMin, float TMax);
	void update_g(int comNo, UINT numRecursion);
	void update_c(int comNo, UINT numRecursion);
	void raytrace_g(int comNo);
	void raytrace_c(int comNo);
	void copyBackBuffer(uint32_t comNo);
	void copyDepthBuffer(uint32_t comNo);
	void setASswapIndex(int index) { buffSwap[0] = index; }
	void setRaytraceSwapIndex(int index) { buffSwap[1] = index; }

	Dx_Resource* getInstanceIdMap();

	void allSwapIndex() {
		Dx_Device* dev = Dx_Device::GetInstance();
		int sync = dev->allSwapIndex();
		setASswapIndex(sync);
		setRaytraceSwapIndex(1 - sync);
	}

	void setGIparameter(float LightArea, int RandNum, TraceMode mode = ONE_RAY);
	void resetFrameIndex();
	void set_DepthRange_NorRange(float DepthRange, float NorRange);
};

#endif
