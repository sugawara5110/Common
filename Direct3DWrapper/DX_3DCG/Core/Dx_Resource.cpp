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

void Dx_Resource::CopyResource(uint32_t comIndex, Dx_Resource* src) {

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	d.ResourceBarrier(src->res.Get(), src->state, D3D12_RESOURCE_STATE_COPY_SOURCE);

	d.ResourceBarrier(res.Get(), state, D3D12_RESOURCE_STATE_COPY_DEST);

	d.getCommandList()->CopyResource(res.Get(), src->res.Get());

	d.ResourceBarrier(res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, state);

	d.ResourceBarrier(src->res.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, src->state);
}

void Dx_Resource::delayCopyResource(uint32_t comIndex, Dx_Resource* src) {

	Dx_CommandListObj& d = *Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
	d.delayResourceBarrierBefore(src->res.Get(), src->state, D3D12_RESOURCE_STATE_COPY_SOURCE);

	d.delayResourceBarrierBefore(res.Get(), state, D3D12_RESOURCE_STATE_COPY_DEST);

	d.delayCopyResource(res.Get(), src->res.Get());

	d.delayResourceBarrierAfter(res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, state);

	d.delayResourceBarrierAfter(src->res.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, src->state);
}