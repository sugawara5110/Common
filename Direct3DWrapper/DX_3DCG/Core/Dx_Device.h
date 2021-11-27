//*****************************************************************************************//
//**                                                                                     **//
//**                               Dx_Device                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Device_Header
#define Class_Dx_Device_Header

#include "Dx_Util.h"
#include <d3d12.h>
#include <d3d10_1.h>

#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

class Dx_Device {

private:
	static Dx_Device* dev;

public:
	static void InstanceCreate();
	static Dx_Device* GetInstance();
	static void DeleteInstance();

	HRESULT createDevice();
	~Dx_Device();

	ID3D12Device5* getDevice();

	UINT64 getRequiredIntermediateSize(ID3D12Resource* res);

	ComPtr<ID3D12Resource> CreateStreamBuffer(UINT64 byteSize);

	HRESULT createDefaultResourceTEXTURE2D(ID3D12Resource** def, UINT64 width, UINT height,
		DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 width, UINT height,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

	HRESULT createDefaultResourceBuffer(ID3D12Resource** def, UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceBuffer_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createUploadResource(ID3D12Resource** up, UINT64 uploadBufferSize);

	HRESULT createReadBackResource(ID3D12Resource** ba, UINT64 BufferSize);

	HRESULT textureInit(int width, int height,
		ID3D12Resource** up, ID3D12Resource** def, DXGI_FORMAT format,
		D3D12_RESOURCE_STATES firstState);

	ComPtr<ID3D12RootSignature> CreateRsCommon(D3D12_ROOT_SIGNATURE_DESC* rootSigDesc);
	ComPtr<ID3D12DescriptorHeap> CreateDescHeap(int numDesc);
	ComPtr<ID3D12DescriptorHeap> CreateSamplerDescHeap(D3D12_SAMPLER_DESC& descSampler);

	void CreateSrvTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, ID3D12Resource** texture, int texNum);

	void CreateSrvBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, ID3D12Resource** buffer, int bufNum,
		UINT* StructureByteStride);

	void CreateCbv(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor,
		D3D12_GPU_VIRTUAL_ADDRESS* virtualAddress, UINT* sizeInBytes, int bufNum);

	void CreateUavBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor,
		ID3D12Resource** buffer, UINT* byteStride, UINT* bufferSize, int bufNum);
};

#endif
