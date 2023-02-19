//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DxrRenderer.h                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxrRenderer_Header
#define Class_DxrRenderer_Header

#include "../Core/Dx12ProcessCore.h"
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
	CoordTf::MATRIX projectionToWorld;
	CoordTf::VECTOR4 cameraPosition;
	CoordTf::VECTOR4 emissivePosition[LIGHT_PCS];//xyz:Pos, w:オンオフ
	CoordTf::VECTOR4 numEmissive;//x:Em, y:numInstance
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];//レンジ, 減衰1, 減衰2, 減衰3
	CoordTf::VECTOR4 GlobalAmbientColor;
	CoordTf::VECTOR4 emissiveNo[LIGHT_PCS];//.xのみ
	CoordTf::VECTOR4 dDirection;
	CoordTf::VECTOR4 dLightColor;
	CoordTf::VECTOR4 dLightst;//x:オンオフ
	CoordTf::VECTOR4 TMin_TMax;//x, y
	UINT maxRecursion;
};

struct DxrMaterialCB {
	CoordTf::VECTOR4 vDiffuse = { 0.8f,0.8f,0.8f,1.0f };//ディフューズ色
	CoordTf::VECTOR4 vSpeculer = { 0.6f,0.6f,0.6f,1.0f };//スぺキュラ色
	CoordTf::VECTOR4 vAmbient = { 0.1f,0.1f,0.1f,0.0f };//アンビエント
	float shininess = 4.0f;//スペキュラ強さ
	float RefractiveIndex = 0.0f;//屈折率
	float AlphaBlend = 0.0f;//on:1.0f, off:0.0f
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
};

enum ShaderTestMode {
	Standard = 0,
	ID = 1,
	Normal = 2
};

class DxrRenderer {

private:
	static const int numSwapIndex = 2;
	std::vector<ParameterDXR*> PD = {};
	ASobj asObj[numSwapIndex] = {};
	CBobj cbObj[numSwapIndex] = {};
	int buffSwap[numSwapIndex] = { 0,0 };

	ConstantBuffer<DxrConstantBuffer>* sCB;
	ConstantBuffer<DxrMaterialCB>* material;
	ConstantBuffer<WVP_CB>* wvp;

	ComPtr<ID3D12StateObject> mpPipelineState;
	ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	ComPtr<ID3D12Resource> mpShaderTable[numSwapIndex];
	Dx_Resource mpOutputResource = {};
	Dx_Resource mpDepthResource = {};
	Dx_Resource mpInstanceIdMapResource = {};

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
		Dx12Process* dx = Dx12Process::GetInstance();
		int sync = dx->allSwapIndex();
		setASswapIndex(sync);
		setRaytraceSwapIndex(1 - sync);
	}
};

#endif
