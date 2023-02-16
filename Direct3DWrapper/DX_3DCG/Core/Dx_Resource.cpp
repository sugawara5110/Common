//*****************************************************************************************//
//**                                                                                     **//
//**                               Dx_Resource                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Resource.h"

ID3D12Resource* Dx_Resource::getResource() {
	return res.Get();
}

ID3D12Resource** Dx_Resource::getResourceAddress() {
	return res.GetAddressOf();
}

HRESULT Dx_Resource::createDefaultResourceTEXTURE2D(UINT64 width, UINT height,
	DXGI_FORMAT format, D3D12_RESOURCE_STATES firstState) {

	Width = width;
	Height = height;
	state = firstState;
	return Dx_Device::GetInstance()->createDefaultResourceTEXTURE2D(res.GetAddressOf(), width, height,
		format, firstState);
}

HRESULT Dx_Resource::createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(UINT64 width, UINT height,
	D3D12_RESOURCE_STATES firstState,
	DXGI_FORMAT format) {

	Width = width;
	Height = height;
	state = firstState;
	return Dx_Device::GetInstance()->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(res.GetAddressOf(), width, height,
		firstState,
		format);
}

HRESULT Dx_Resource::createDefaultResourceBuffer(UINT64 bufferSize,
	D3D12_RESOURCE_STATES firstState) {

	Width = bufferSize;
	Height = 1;
	state = firstState;
	return Dx_Device::GetInstance()->createDefaultResourceBuffer(res.GetAddressOf(), bufferSize,
		firstState);
}

HRESULT Dx_Resource::createDefaultResourceBuffer_UNORDERED_ACCESS(UINT64 bufferSize,
	D3D12_RESOURCE_STATES firstState) {

	Width = bufferSize;
	Height = 1;
	state = firstState;
	return Dx_Device::GetInstance()->createDefaultResourceBuffer_UNORDERED_ACCESS(res.GetAddressOf(), bufferSize,
		firstState);
}

HRESULT Dx_Resource::createUploadResource(UINT64 uploadBufferSize) {

	Width = uploadBufferSize;
	Height = 1;
	return Dx_Device::GetInstance()->createUploadResource(res.GetAddressOf(), uploadBufferSize);
}

HRESULT Dx_Resource::createReadBackResource(UINT64 BufferSize) {

	Width = BufferSize;
	Height = 1;
	return Dx_Device::GetInstance()->createReadBackResource(res.GetAddressOf(), BufferSize);
}

void Dx_Resource::CreateSrvTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor) {

	return Dx_Device::GetInstance()->CreateSrvTexture(hDescriptor, res.GetAddressOf(), 1);
}

void Dx_Resource::CreateSrvBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor, UINT StructureByteStride) {

	return Dx_Device::GetInstance()->CreateSrvBuffer(hDescriptor, res.GetAddressOf(), 1,
		&StructureByteStride);
}

void Dx_Resource::CreateUavBuffer(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor,
	UINT byteStride, UINT bufferSize) {

	return Dx_Device::GetInstance()->CreateUavBuffer(hDescriptor,
		res.GetAddressOf(), &byteStride, &bufferSize, 1);
}

void Dx_Resource::CreateUavTexture(D3D12_CPU_DESCRIPTOR_HANDLE& hDescriptor) {

	return Dx_Device::GetInstance()->CreateUavTexture(hDescriptor, res.GetAddressOf(), 1);
}

void Dx_Resource::ResourceBarrier(uint32_t comIndex, D3D12_RESOURCE_STATES after) {

	if (state == after)return;

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	d.ResourceBarrier(res.Get(), state, after);
	state = after;
}

void Dx_Resource::delayResourceBarrierBefore(uint32_t comIndex, D3D12_RESOURCE_STATES after) {

	if (state == after)return;

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	d.delayResourceBarrierBefore(res.Get(), state, after);
	state = after;
}

void Dx_Resource::delayResourceBarrierAfter(uint32_t comIndex, D3D12_RESOURCE_STATES after) {

	if (state == after)return;

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	d.delayResourceBarrierAfter(res.Get(), state, after);
	state = after;
}

static bool BarrierOn(Dx_Resource* res) {

	D3D12_RESOURCE_DIMENSION dim = res->getResource()->GetDesc().Dimension;

	return dim == D3D12_RESOURCE_DIMENSION_TEXTURE1D ||
		dim == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
		dim == D3D12_RESOURCE_DIMENSION_TEXTURE3D ||
		res->state == D3D12_RESOURCE_STATE_STREAM_OUT;
}

void Dx_Resource::CopyResource(uint32_t comIndex, Dx_Resource* src) {

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	if (BarrierOn(src))d.ResourceBarrier(src->res.Get(), src->state, D3D12_RESOURCE_STATE_COPY_SOURCE);

	if (BarrierOn(this))d.ResourceBarrier(res.Get(), state, D3D12_RESOURCE_STATE_COPY_DEST);

	d.getCommandList()->CopyResource(res.Get(), src->res.Get());

	if (BarrierOn(this))d.ResourceBarrier(res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, state);

	if (BarrierOn(src))d.ResourceBarrier(src->res.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, src->state);
}

void Dx_Resource::delayCopyResource(uint32_t comIndex, Dx_Resource* src) {

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	if (BarrierOn(src))d.delayResourceBarrierBefore(src->res.Get(), src->state, D3D12_RESOURCE_STATE_COPY_SOURCE);

	if (BarrierOn(this))d.delayResourceBarrierBefore(res.Get(), state, D3D12_RESOURCE_STATE_COPY_DEST);

	d.delayCopyResource(res.Get(), src->res.Get());

	if (BarrierOn(this))d.delayResourceBarrierAfter(res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, state);

	if (BarrierOn(src))d.delayResourceBarrierAfter(src->res.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, src->state);
}

void Dx_Resource::CreateDefaultBuffer(int comIndex, const void* initData, UINT64 byteSize, bool uav) {
	Width = byteSize;
	Height = 1;
	state = D3D12_RESOURCE_STATE_GENERIC_READ;
	if (uav)state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	res = Dx_CommandManager::GetInstance()->CreateDefaultBuffer(comIndex, initData, byteSize, up, uav);
}

D3D12_VERTEX_BUFFER_VIEW VertexView::VertexBufferView() {
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = VertexBufferGPU.getResource()->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;
	return vbv;
}

D3D12_INDEX_BUFFER_VIEW IndexView::IndexBufferView() {
	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = IndexBufferGPU.getResource()->GetGPUVirtualAddress();
	ibv.Format = IndexFormat;
	ibv.SizeInBytes = IndexBufferByteSize;
	return ibv;
}

void StreamView::createResetBuffer(int comIndex) {
	if (resetBuffer.getResource())return;
	UINT64 zero[1] = {};
	zero[0] = 0;
	resetBuffer.CreateDefaultBuffer(comIndex, zero, sizeof(UINT64), false);
}

StreamView::StreamView() {
	BufferFilledSizeBufferGPU.createDefaultResourceBuffer(sizeof(UINT64));
	BufferFilledSizeBufferGPU.state = D3D12_RESOURCE_STATE_STREAM_OUT;
	ReadBuffer.createReadBackResource(sizeof(UINT64));
}

void StreamView::ResetFilledSizeBuffer(int comIndex) {
	createResetBuffer(comIndex);
	BufferFilledSizeBufferGPU.delayCopyResource(comIndex, &resetBuffer);
}

void StreamView::outputReadBack(int comIndex) {
	ReadBuffer.CopyResource(comIndex, &BufferFilledSizeBufferGPU);
}

void StreamView::outputFilledSize() {
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = sizeof(UINT64);
	UINT* ba = nullptr;
	ReadBuffer.getResource()->Map(0, &range, reinterpret_cast<void**>(&ba));
	FilledSize = ba[0];
	ReadBuffer.getResource()->Unmap(0, nullptr);
}

D3D12_STREAM_OUTPUT_BUFFER_VIEW StreamView::StreamBufferView() {
	D3D12_STREAM_OUTPUT_BUFFER_VIEW sbv = {};
	sbv.BufferLocation = StreamBufferGPU.getResource()->GetGPUVirtualAddress();
	sbv.SizeInBytes = StreamBufferByteSize;
	sbv.BufferFilledSizeLocation = BufferFilledSizeBufferGPU.getResource()->GetGPUVirtualAddress();
	return sbv;
}