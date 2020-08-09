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
	MATRIX projectionInv;
	MATRIX viewInv;
	VECTOR4 cameraUp;
	VECTOR4 cameraPosition;
	VECTOR4 lightPosition[LIGHT_PCS];//xyz:Pos, w:オンオフ
	VECTOR4 lightAmbientColor;
	VECTOR4 lightDiffuseColor;
};

struct DxrTransformCB {
	MATRIX m;
};

class DXR_Basic {

private:
	std::unique_ptr<VertexView[]> pVertexBuffer;
	std::unique_ptr<IndexView[]> pIndexBuffer;

	VECTOR4 lightAmbientColor = { 0.1f,0.1f,0.1f,0.0f };
	VECTOR4 lightDiffuseColor = { 0.5f,0.5f,0.5f,0.0f };

	DxrConstantBuffer cb = {};
	ConstantBuffer<DxrConstantBuffer>* sCB;
	ConstantBuffer<DxrTransformCB>* world;
	ComPtr<ID3D12Resource> mpTopLevelAS;
	std::unique_ptr<ComPtr<ID3D12Resource>[]> mpBottomLevelAS;
	uint64_t mTlasSize = 0;
	ComPtr<ID3D12StateObject> mpPipelineState;
	ComPtr<ID3D12RootSignature> mpGlobalRootSig;
	ComPtr<ID3D12Resource> mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;
	ComPtr<ID3D12Resource> mpOutputResource;
	ComPtr<ID3D12DescriptorHeap> mpSrvUavCbvHeap;
	uint32_t numSkipLocalHeap = 0;
	ComPtr<ID3D12DescriptorHeap> mpSamplerHeap;

	std::unique_ptr<AccelerationStructureBuffers[]> bottomLevelBuffers;
	AccelerationStructureBuffers topLevelBuffers;

	UINT numInstance = 0;
	std::unique_ptr<MATRIX[]> Transform = nullptr;

	template<typename T>
	void createTriangleVB(int comNo, T** vertices, UINT* numVertices,
		uint32_t** index, UINT* numIndex) {

		Dx12Process* dx = Dx12Process::GetInstance();
		world = new ConstantBuffer<DxrTransformCB>(numInstance);
		pVertexBuffer = std::make_unique<VertexView[]>(numInstance);
		pIndexBuffer = std::make_unique<IndexView[]>(numInstance);

		for (UINT i = 0; i < numInstance; i++) {
			const uint32_t byteSize = sizeof(T) * numVertices[i];
			pVertexBuffer[i].VertexByteStride = sizeof(T);
			pVertexBuffer[i].VertexBufferByteSize = byteSize;
			pVertexBuffer[i].VertexBufferGPU = dx->CreateDefaultBuffer(comNo, vertices[i],
				pVertexBuffer[i].VertexBufferByteSize,
				pVertexBuffer[i].VertexBufferUploader);

			pIndexBuffer[i].IndexFormat = DXGI_FORMAT_R32_UINT;
			pIndexBuffer[i].IndexBufferByteSize = sizeof(uint32_t) * numIndex[i];
			pIndexBuffer[i].IndexCount = numIndex[i];
			pIndexBuffer[i].IndexBufferGPU = dx->CreateDefaultBuffer(comNo, index[i],
				pIndexBuffer[i].IndexBufferByteSize,
				pIndexBuffer[i].IndexBufferUploader);
		}
	}

	AccelerationStructureBuffers createBottomLevelAS(int comNo, UINT InstanceID);

	void createTopLevelAS(int comNo, uint64_t& tlasSize, bool update);

	ComPtr<ID3D12RootSignature> createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc);

	void createAccelerationStructures(int comNo);
	void createRtPipelineState();
	void createShaderResources(ID3D12Resource** difTexture, ID3D12Resource** norTexture);
	void createShaderTable();

	void setCB();

public:
	template<typename T>
	void initDXR(int comNo, UINT numinstance,
		T** vertices, UINT* numVertices,
		uint32_t** index, UINT* numIndex,
		ID3D12Resource** difTexture, ID3D12Resource** norTexture) {

		sCB = new ConstantBuffer<DxrConstantBuffer>(1);
		Dx12Process* dx = Dx12Process::GetInstance();

		if (dx->DXR_ON) {
			numInstance = numinstance;
			dx->dx_sub[comNo].Bigin();
			createTriangleVB(comNo, vertices, numVertices, index, numIndex);
			createAccelerationStructures(comNo);
			dx->dx_sub[comNo].End();
			dx->WaitFenceCurrent();
			for (UINT i = 0; i < numInstance; i++)
				mpBottomLevelAS[i] = bottomLevelBuffers[i].pResult;
			mpTopLevelAS = topLevelBuffers.pResult;

			createRtPipelineState();
			createShaderResources(difTexture, norTexture);
			createShaderTable();
		}
	}

	void updateTransform(UINT InstanceNo, VECTOR3 pos, VECTOR3 theta, VECTOR3 size);
	void raytrace(int comNo);
	~DXR_Basic() { S_DELETE(sCB); S_DELETE(world); }
};

#endif
