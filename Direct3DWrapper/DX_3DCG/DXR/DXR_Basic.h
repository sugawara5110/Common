//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DXR_Basic.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DXR_Basic_Header
#define Class_DXR_Basic_Header

#include "../Core/Dx12ProcessCore.h"
#include "../../MicroSoftLibrary/DXCAPI/dxcapi.use.h"

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> pScratch;
	ComPtr<ID3D12Resource> pResult;
	ComPtr<ID3D12Resource> pInstanceDesc;//top-level AS しか使わない
	bool firstSet = false;
};

struct WVP_CB {
	CoordTf::MATRIX wvp;
	CoordTf::MATRIX world;
};

struct DxrConstantBuffer
{
	CoordTf::MATRIX projectionToWorld;
	CoordTf::VECTOR4 cameraUp;
	CoordTf::VECTOR4 cameraPosition;
	CoordTf::VECTOR4 emissivePosition[LIGHT_PCS];//xyz:Pos, w:オンオフ
	CoordTf::VECTOR4 numEmissive;//.xのみ
	CoordTf::VECTOR4 Lightst[LIGHT_PCS];//レンジ, 減衰1, 減衰2, 減衰3
	CoordTf::VECTOR4 GlobalAmbientColor;
	CoordTf::VECTOR4 emissiveNo[LIGHT_PCS];//.xのみ
	CoordTf::VECTOR4 dDirection;
	CoordTf::VECTOR4 dLightColor;
	CoordTf::VECTOR4 dLightst;//x:オンオフ
	UINT maxRecursion;
};

struct DxrMaterialCB {
	CoordTf::VECTOR4 vDiffuse = { 0.8f,0.8f,0.8f,1.0f };//ディフューズ色
	CoordTf::VECTOR4 vSpeculer = { 0.6f,0.6f,0.6f,1.0f };//スぺキュラ色
	CoordTf::VECTOR4 vAmbient = { 0.1f,0.1f,0.1f,0.0f };//アンビエント
	CoordTf::VECTOR4 AddObjColor = {};//オブジェクトの色変化用
	float shininess = 4.0f;//スペキュラ強さ
	float RefractiveIndex = 0.0f;//屈折率
	float AlphaBlend = 0.0f;//on:1.0f, off:0.0f
	UINT materialNo = 0;
};

enum MaterialType {
	METALLIC,
	NONREFLECTION,
	EMISSIVE,
	DIRECTIONLIGHT_METALLIC,
	DIRECTIONLIGHT_NONREFLECTION,
	TRANSLUCENCE
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

class DXR_Basic {

private:
	ParameterDXR** PD;
	ASobj asObj[2] = {};
	CBobj cbObj[2] = {};
	int buffSwap[2] = { 0,0 };

	ConstantBuffer<DxrConstantBuffer>* sCB;
	ConstantBuffer<DxrMaterialCB>* material;
	ConstantBuffer<WVP_CB>* wvp;

	ComPtr<ID3D12StateObject> mpPipelineState;
	ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	ComPtr<ID3D12Resource> mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;
	ComPtr<ID3D12Resource> mpOutputResource;
	ComPtr<ID3D12Resource> mpDepthResource;
	ComPtr<ID3D12DescriptorHeap> mpSrvUavCbvHeap[2];
	uint32_t numSkipLocalHeap = 0;
	ComPtr<ID3D12DescriptorHeap> mpSamplerHeap;

	UINT numParameter = 0;//PD数
	UINT numMaterial = 0;//全マテリアル数
	UINT maxRecursion = 1;
	UINT maxNumInstancing = 0;

	void createInstanceIdBuffer(UINT numMaterial);
	void createBottomLevelAS1(Dx_CommandListObj* com, VertexView* vv,
		IndexView* iv, UINT currentIndexCount, UINT MaterialNo,
		bool update, bool tessellation, bool alphaTest);
	void createBottomLevelAS(Dx_CommandListObj* com);
	void createTopLevelAS(Dx_CommandListObj* com);
	ComPtr<ID3D12RootSignature> createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc);
	void createAccelerationStructures();
	void createRtPipelineState();
	void createShaderResources();
	void createShaderTable();

	void updateMaterial(CBobj* cbObj);
	void updateCB(CBobj* cbObj, UINT numRecursion);
	void updateAS(Dx_CommandListObj* com, UINT numRecursion);
	void setCB();
	void swapSrvUavCbvHeap();
	void raytrace(Dx_CommandListObj* com);

public:
	void initDXR(UINT numParameter, ParameterDXR** pd, MaterialType* type, UINT maxRecursion);
	void update_g(int comNo, UINT numRecursion);
	void update_c(int comNo, UINT numRecursion);
	void raytrace_g(int comNo);
	void raytrace_c(int comNo);
	void copyBackBuffer(int comNo);
	void copyDepthBuffer(int comNo);
	void setASswapIndex(int index) { buffSwap[0] = index; }
	void setRaytraceSwapIndex(int index) { buffSwap[1] = index; }
	~DXR_Basic();
};

#endif
