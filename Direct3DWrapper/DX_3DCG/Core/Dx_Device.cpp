//*****************************************************************************************//
//**                                                                                     **//
//**                               Dx_Device                                             **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Device.h"

namespace {

	IDXGIFactory4* mdxgiFactory = nullptr;
	ID3D12Device5* md3dDevice = nullptr;
	UINT mCbvSrvUavDescriptorSize = 0;

	HRESULT createDefaultResourceCommon(ID3D12Resource** def,
		D3D12_RESOURCE_DIMENSION dimension, UINT64 width, UINT height,
		DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES firstState) {

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = dimension;
		desc.Width = width;
		desc.Height = height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = flags;

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		//デフォルトバッファ生成
		return md3dDevice->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			firstState,
			nullptr,
			IID_PPV_ARGS(def));
	}
}

Dx_Device* Dx_Device::dev = nullptr;

void Dx_Device::InstanceCreate() {
	if (!dev)dev = NEW Dx_Device();
}

Dx_Device* Dx_Device::GetInstance() {
	if (dev)return dev;
	return nullptr;
}

void Dx_Device::DeleteInstance() {
	S_DELETE(dev);
}

void Dx_Device::createDevice() {

#if defined(DEBUG) || defined(_DEBUG) 
		//デバッグ中はデバッグレイヤーを有効化する
	{
		ComPtr<ID3D12Debug> debugController;
		HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()));
		if (FAILED(hr)) {
			throw std::runtime_error("D3D12GetDebugInterface Error");
		}
		debugController->EnableDebugLayer();
	}
#endif

		//ファクトリ生成
		//アダプターの列挙、スワップ チェーンの作成、
		//および全画面表示モードとの間の切り替えに使用される Alt + 
		//Enter キー シーケンスとのウィンドウの関連付けを行うオブジェクトを生成するために使用
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory));
	if (FAILED(hr)) {
		throw std::runtime_error("CreateDXGIFactory1 Error");
	}

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,   //default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	//↑ハードウエア処理不可の場合ソフトウエア処理にする,ソフトウエア処理デバイス生成
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		if (FAILED(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(pWarpAdapter.GetAddressOf())))) {

			throw std::runtime_error("EnumWarpAdapter Error");
		}

		if (FAILED(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)))) {

			throw std::runtime_error("D3D12CreateDevice Error");
		}
	}

	if (DXR_CreateResource) {
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
		HRESULT hr = md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
			&features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

		if (FAILED(hr) || features5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			DXR_CreateResource = false;
			throw std::runtime_error("DXR not supported");
		}
	}

	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

Dx_Device::~Dx_Device() {

#if defined(DEBUG) || defined(_DEBUG) 
	//ReportLiveDeviceObjectsを呼び出したタイミングでの
	//生存オブジェクトを調べる
	if (ReportLiveDeviceObjectsOn) {
		ID3D12DebugDevice* debugInterface;
		if (SUCCEEDED(Dx_Device::GetInstance()->getDevice()->QueryInterface(&debugInterface)))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
			RELEASE(debugInterface);
		}
	}
#endif

	RELEASE(md3dDevice);
	RELEASE(mdxgiFactory);
}

ID3D12Device5* Dx_Device::getDevice() {
	return md3dDevice;
}

IDXGIFactory4* Dx_Device::getFactory() {
	return mdxgiFactory;
}

UINT64 Dx_Device::getRequiredIntermediateSize(ID3D12Resource* res) {
	D3D12_RESOURCE_DESC desc = res->GetDesc();
	UINT64  total_bytes = 0;
	md3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &total_bytes);
	return total_bytes;
}

HRESULT Dx_Device::createDefaultResourceTEXTURE2D(ID3D12Resource** def, UINT64 width, UINT height,
	DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState) {

	return createDefaultResourceCommon(def,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height,
		format, D3D12_RESOURCE_FLAG_NONE, firstState);
}

HRESULT Dx_Device::createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 width, UINT height,
	D3D12_RESOURCE_STATES firstState,
	DXGI_FORMAT format) {

	return createDefaultResourceCommon(def,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height,
		format, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		firstState);
}

HRESULT Dx_Device::createDefaultResourceBuffer(ID3D12Resource** def, UINT64 bufferSize,
	D3D12_RESOURCE_STATES firstState) {

	return createDefaultResourceCommon(def,
		D3D12_RESOURCE_DIMENSION_BUFFER, bufferSize, 1,
		DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE,
		firstState);
}

HRESULT Dx_Device::createDefaultResourceBuffer_UNORDERED_ACCESS(ID3D12Resource** def, UINT64 bufferSize,
	D3D12_RESOURCE_STATES firstState) {

	return createDefaultResourceCommon(def,
		D3D12_RESOURCE_DIMENSION_BUFFER, bufferSize, 1,
		DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		firstState);
}

HRESULT Dx_Device::createUploadResource(ID3D12Resource** up, UINT64 uploadBufferSize) {
	D3D12_HEAP_PROPERTIES HeapPropsUp = {};
	HeapPropsUp.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapPropsUp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapPropsUp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapPropsUp.CreationNodeMask = 1;
	HeapPropsUp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc = {};
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = uploadBufferSize;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	return md3dDevice->CreateCommittedResource(&HeapPropsUp, D3D12_HEAP_FLAG_NONE,
		&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(up));
}

HRESULT Dx_Device::createReadBackResource(ID3D12Resource** ba, UINT64 BufferSize) {
	D3D12_HEAP_PROPERTIES heap = {};
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = BufferSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	return md3dDevice->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(ba));
}

HRESULT Dx_Device::textureInit(int width, int height,
	ID3D12Resource** up, ID3D12Resource** def, DXGI_FORMAT format,
	D3D12_RESOURCE_STATES firstState, bool uav) {

	HRESULT hr;

	if (uav) {
		hr = createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(def, width, height, firstState, format);
		if (FAILED(hr)) {
			return hr;
		}
	}
	else {
		hr = createDefaultResourceTEXTURE2D(def, width, height, format, firstState);
		if (FAILED(hr)) {
			return hr;
		}
	}

	//upload
	UINT64 uploadBufferSize = getRequiredIntermediateSize(*def);
	hr = createUploadResource(up, uploadBufferSize);
	if (FAILED(hr)) {
		return hr;
	}
	return S_OK;
}

ComPtr<ID3D12RootSignature> Dx_Device::CreateRsCommon(D3D12_ROOT_SIGNATURE_DESC* rootSigDesc)
{
	ComPtr<ID3D12RootSignature>rs;

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Dx_Device::CreateRsCommon Error!!");
		Dx_Util::ErrorMessage((char*)errorBlob.Get()->GetBufferPointer());
		return nullptr;
	}

	//RootSignature生成
	hr = md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(rs.GetAddressOf()));

	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Dx_Device::CreateRsCommon Error!!"); return nullptr;
	}

	return rs;
}

ComPtr <ID3D12DescriptorHeap> Dx_Device::CreateDescHeap(int numDesc) {
	ComPtr <ID3D12DescriptorHeap>heap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDesc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr;
	hr = md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Dx_Device::CreateDescHeap Error!!"); return nullptr;
	}
	return heap;
}

ComPtr<ID3D12DescriptorHeap> Dx_Device::CreateSamplerDescHeap(D3D12_SAMPLER_DESC& descSampler) {
	ComPtr <ID3D12DescriptorHeap>heap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr;
	hr = md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Dx_Device::CreateSamplerDescHeap Error!!"); return nullptr;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.Get()->GetCPUDescriptorHandleForHeapStart();
	md3dDevice->CreateSampler(&descSampler, handle);
	return heap;
}

void Dx_Device::CreateSrvTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, ID3D12Resource** tex, int texNum) {

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	for (int i = 0; i < texNum; i++) {
		srvDesc.Format = tex[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex[i]->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(tex[i], &srvDesc, hDescriptor);
		hDescriptor.ptr += mCbvSrvUavDescriptorSize;
	}
}

void Dx_Device::CreateSrvBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, ID3D12Resource** buffer, int bufNum,
	UINT* StructureByteStride) {

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	for (int i = 0; i < bufNum; i++) {
		srvDesc.Buffer.StructureByteStride = StructureByteStride[i];
		srvDesc.Buffer.NumElements = (UINT)buffer[i]->GetDesc().Width / StructureByteStride[i];
		md3dDevice->CreateShaderResourceView(buffer[i], &srvDesc, hDescriptor);
		hDescriptor.ptr += mCbvSrvUavDescriptorSize;
	}
}

void Dx_Device::CreateCbv(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor,
	D3D12_GPU_VIRTUAL_ADDRESS* virtualAddress, UINT* sizeInBytes, int bufNum) {

	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	for (int i = 0; i < bufNum; i++) {
		bufferDesc.SizeInBytes = sizeInBytes[i];
		bufferDesc.BufferLocation = virtualAddress[i];
		md3dDevice->CreateConstantBufferView(&bufferDesc, hDescriptor);
		hDescriptor.ptr += mCbvSrvUavDescriptorSize;
	}
}

void Dx_Device::CreateUavBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor,
	ID3D12Resource** buffer, UINT* byteStride, UINT* bufferSize, int bufNum) {

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.CounterOffsetInBytes = 0;//pCounterResourceがnullptrの場合0
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	for (int i = 0; i < bufNum; i++) {
		uavDesc.Buffer.StructureByteStride = byteStride[i];
		uavDesc.Buffer.NumElements = bufferSize[i];
		md3dDevice->CreateUnorderedAccessView(buffer[i], nullptr, &uavDesc, hDescriptor);
		hDescriptor.ptr += mCbvSrvUavDescriptorSize;
	}
}

void Dx_Device::CreateUavTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, ID3D12Resource** tex, int texNum)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	for (int i = 0; i < texNum; i++) {
		uavDesc.Format = tex[i]->GetDesc().Format;
		md3dDevice->CreateUnorderedAccessView(tex[i], nullptr, &uavDesc, hDescriptor);
		hDescriptor.ptr += mCbvSrvUavDescriptorSize;
	}
}

UINT Dx_Device::getCbvSrvUavDescriptorSize() {
	return mCbvSrvUavDescriptorSize;
}