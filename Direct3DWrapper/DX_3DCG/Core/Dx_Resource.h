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
	ComPtr<ID3D12Resource> up = nullptr;
	ComPtr<ID3D12Resource> res = nullptr;

public:
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
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

	void CreateDefaultBuffer(int comIndex, const void* initData, UINT64 byteSize, bool uav);
};

struct VertexView {

	Dx_Resource VertexBufferGPU = {};

	//バッファのサイズ等
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView();
};

struct IndexView {

	Dx_Resource IndexBufferGPU = {};

	//バッファのサイズ等
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT IndexBufferByteSize = 0;
	//DrawIndexedInstancedの引数で使用
	UINT IndexCount = 0;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView();
};

struct StreamView {

private:
	static Dx_Resource resetBuffer;
	Dx_Resource BufferFilledSizeBufferGPU = {};
	Dx_Resource ReadBuffer = {};

public:
	Dx_Resource StreamBufferGPU = {};

	//バッファのサイズ等
	UINT StreamByteStride = 0;
	UINT StreamBufferByteSize = 0;
	UINT FilledSize = 0;

	static void createResetBuffer(int comIndex);

	StreamView();

	void ResetFilledSizeBuffer(int comIndex);

	void outputReadBack(int comIndex);

	void outputFilledSize();

	D3D12_STREAM_OUTPUT_BUFFER_VIEW StreamBufferView();
};

template<class T>
class ConstantBuffer {

private:
	ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;
	UINT mElementByteSize = 0;

public:
	ConstantBuffer(UINT elementCount) {

		//コンスタントバッファサイズは256バイト単位でアライメント(例: 0→0, 10→256, 300→512)
		mElementByteSize = ALIGNMENT_TO(256, sizeof(T));

		if (FAILED(Dx_Device::GetInstance()->createUploadResource(&mUploadBuffer, (UINT64)mElementByteSize * elementCount))) {
			Dx_Util::ErrorMessage("ConstantBufferエラー");
		}

		mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
	}

	ConstantBuffer(const ConstantBuffer& rhs) = delete;
	ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;

	~ConstantBuffer() {

		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource()const {
		return mUploadBuffer.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress(UINT elementIndex) {
		return mUploadBuffer.Get()->GetGPUVirtualAddress() + mElementByteSize * elementIndex;
	}

	void CopyData(int elementIndex, const T& data) {
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

	UINT getSizeInBytes() {
		return mElementByteSize;
	}
};

#endif