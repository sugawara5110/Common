//*****************************************************************************************//
//**                                                                                     **//
//**                               Dx_Resource                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Resource_Header
#define Class_Dx_Resource_Header

#include "Dx_CommandManager.h"

class Dx_Resource {

private:
	ComPtr<ID3D12Resource> res = nullptr;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

public:
	UINT64 Width = 0;
	UINT Height = 0;

	ID3D12Resource* getResource();

	ID3D12Resource** getResourceAddress();

	HRESULT createDefaultResourceTEXTURE2D(UINT64 width, UINT height,
		DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(UINT64 width, UINT height,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

	HRESULT createDefaultResourceBuffer(UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createDefaultResourceBuffer_UNORDERED_ACCESS(UINT64 bufferSize,
		D3D12_RESOURCE_STATES firstState = D3D12_RESOURCE_STATE_COMMON);

	HRESULT createUploadResource(UINT64 uploadBufferSize);

	HRESULT createReadBackResource(UINT64 BufferSize);

	void CreateSrvTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor);

	void CreateSrvBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, UINT StructureByteStride);

	void CreateUavBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor,
		UINT byteStride, UINT bufferSize);

	void CreateUavTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor);

	void ResourceBarrier(uint32_t comIndex, D3D12_RESOURCE_STATES after);

	void delayResourceBarrierBefore(uint32_t comIndex, D3D12_RESOURCE_STATES after);

	void delayResourceBarrierAfter(uint32_t comIndex, D3D12_RESOURCE_STATES after);

	void CopyResource(uint32_t comIndex, Dx_Resource* src);

	void delayCopyResource(uint32_t comIndex, Dx_Resource* src);
};

#endif