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
	bool ReportLiveDeviceObjectsOn = false;

	int cBuffSwap[2] = { 0,0 };
	int dxrBuffSwap[2] = { 0,0 };
	int sync = 0;

	bool wireframe = false;

	bool DXR_CreateResource = false;

	Dx_Device() {}//外部からのオブジェクト生成禁止
	Dx_Device(const Dx_Device& obj) = delete;   // コピーコンストラクタ禁止
	void operator=(const Dx_Device& obj) = delete;// 代入演算子禁止
	~Dx_Device();

public:
	static void InstanceCreate();
	static Dx_Device* GetInstance();
	static void DeleteInstance();

	void wireFrameTest(bool f) { wireframe = f; }
	void dxrCreateResource() { DXR_CreateResource = true; }
	void reportLiveDeviceObjectsOn() { ReportLiveDeviceObjectsOn = true; }

	HRESULT createDevice();

	ID3D12Device5* getDevice();

	IDXGIFactory4* getFactory();

	UINT64 getRequiredIntermediateSize(ID3D12Resource* res);

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

	void CreateUavTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, ID3D12Resource** tex, int texNum);

	bool getDxrCreateResourceState() { return DXR_CreateResource; }
	bool getWireFrameState() { return wireframe; }

	UINT getCbvSrvUavDescriptorSize();

	void setUpSwapIndex(int index) { cBuffSwap[0] = index; }
	void setDrawSwapIndex(int index) { cBuffSwap[1] = index; }
	void setStreamOutputSwapIndex(int index) { dxrBuffSwap[0] = index; }
	void setRaytraceSwapIndex(int index) { dxrBuffSwap[1] = index; }

	int allSwapIndex() {
		sync = 1 - sync;
		setUpSwapIndex(sync);
		setDrawSwapIndex(1 - sync);
		setStreamOutputSwapIndex(sync);
		setRaytraceSwapIndex(1 - sync);
		return sync;
	}

	int cBuffSwapUpdateIndex() { return cBuffSwap[0]; }
	int cBuffSwapDrawOrStreamoutputIndex() { return cBuffSwap[1]; }
	int dxrBuffSwapIndex() { return dxrBuffSwap[0]; }
	int dxrBuffSwapRaytraceIndex() { return dxrBuffSwap[1]; }
};

#endif
