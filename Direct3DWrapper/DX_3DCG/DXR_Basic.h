//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DXR_Basic.h                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DXR_Basic_Header
#define Class_DXR_Basic_Header

#include "Dx12ProcessCore.h"
#include "../MicroSoftLibrary/DXCAPI/dxcapi.use.h"

struct AccelerationStructureBuffers
{
	ComPtr<ID3D12Resource> pScratch;
	ComPtr<ID3D12Resource> pResult;
	ComPtr<ID3D12Resource> pInstanceDesc;//top-level AS しか使わない
};

struct DxrConstantBuffer
{
	MATRIX projectionToWorld;
	VECTOR4 cameraUp;
	VECTOR4 cameraPosition;
	VECTOR4 emissivePosition[LIGHT_PCS];//xyz:Pos, w:オンオフ
	VECTOR4 numEmissive;//.xのみ
	VECTOR4 Lightst[LIGHT_PCS];//レンジ, 減衰1, 減衰2, 減衰3
	VECTOR4 GlobalAmbientColor;
};

struct DxrMaterialCB {
	VECTOR4 vDiffuse = { 0.8f,0.8f,0.8f,1.0f };//ディフューズ色
	VECTOR4 vSpeculer = { 0.6f,0.6f,0.6f,1.0f };//スぺキュラ色
	VECTOR4 vAmbient = { 0.1f,0.1f,0.1f,0.0f };//アンビエント
	float shininess = 4.0f;//スペキュラ強さ
	float alphaTest = 0.0f;//1.0f:on, 0.0f:off 
	UINT materialNo = 0;//0:metallic, 1:emissive
};

enum MaterialType {
	METALLIC,
	EMISSIVE,
	NONREFLECTION
};

struct DxrInstanceCB {
	MATRIX world = {};
};

struct VertexObj {
	ID3D12Resource* VertexBufferGPU = nullptr;
	//バッファのサイズ等
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
};

struct IndexObj {
	ID3D12Resource* IndexBufferGPU = nullptr;
	//バッファのサイズ等
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT IndexBufferByteSize = 0;
	//DrawIndexedInstancedの引数で使用
	UINT IndexCount = 0;
};

class DXR_Basic {

private:
	ParameterDXR** PD;
	std::unique_ptr<VertexObj[]> pVertexBuffer;
	std::unique_ptr<IndexObj[]> pIndexBuffer;

	DxrConstantBuffer cb = {};
	ConstantBuffer<DxrConstantBuffer>* sCB;
	std::unique_ptr<DxrInstanceCB[]> insCb;
	ConstantBuffer<DxrInstanceCB>* instance;
	std::unique_ptr<DxrMaterialCB[]> matCb;
	ConstantBuffer<DxrMaterialCB>* material;

	std::unique_ptr<ComPtr<ID3D12Resource>[]> mpBottomLevelAS;
	ComPtr<ID3D12Resource> mpTopLevelAS;
	uint64_t mTlasSize = 0;
	std::unique_ptr<AccelerationStructureBuffers[]> bottomLevelBuffers;
	AccelerationStructureBuffers topLevelBuffers;

	ComPtr<ID3D12StateObject> mpPipelineState;
	ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	ComPtr<ID3D12Resource> mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;
	ComPtr<ID3D12Resource> mpOutputResource;
	ComPtr<ID3D12DescriptorHeap> mpSrvUavCbvHeap;
	uint32_t numSkipLocalHeap = 0;
	ComPtr<ID3D12DescriptorHeap> mpSamplerHeap;

	UINT numParameter = 0;//PD数
	UINT numMaterial = 0;//全マテリアル数
	UINT maxNumInstancing = 0;//INSTANCE_PCS_3D(256) * numParameter

	void createTriangleVB(int comNo, UINT numMaterial);
	void createBottomLevelAS(int comNo, UINT MaterialNo, bool update);
	void createTopLevelAS(int comNo, uint64_t& tlasSize, bool update);
	ComPtr<ID3D12RootSignature> createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc);
	void createAccelerationStructures(int comNo);
	void createRtPipelineState();
	void createShaderResources();
	void createShaderTable();

	void updateMaterial();
	void setCB();

public:
	void initDXR(int comNo, UINT numParameter, ParameterDXR** pd, MaterialType* type);
	void raytrace(int comNo);
	~DXR_Basic() { S_DELETE(sCB); S_DELETE(instance); S_DELETE(material); }
};

#endif
