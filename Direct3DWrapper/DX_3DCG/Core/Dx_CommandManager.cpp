//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　       Dx_CommandManager                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_CommandManager.h"

std::mutex Dx_CommandManager::mtxGraphicsQueue = {};
std::mutex Dx_CommandManager::mtxComputeQueue = {};
Dx_CommandManager* Dx_CommandManager::com = nullptr;

void Dx_CommandManager::setNumResourceBarrier(int numResourceBarrier) {
	Dx_CommandListObj::NumResourceBarrier = numResourceBarrier;
}

void Dx_CommandManager::InstanceCreate() {
	if (!com) {
		com = NEW Dx_CommandManager(Dx_Device::GetInstance()->getDevice());
	}
}

Dx_CommandManager* Dx_CommandManager::GetInstance() {
	return com;
}

void Dx_CommandManager::DeleteInstance() {
	S_DELETE(com);
}

Dx_CommandManager::Dx_CommandManager(ID3D12Device5* dev) {

	//コマンドキュー生成
	if (!graphicsQueue.Create(dev, false))throw std::runtime_error("Dx_CommandManager: graphicsQueue.Create Error!!");
	if (!computeQueue.Create(dev, true))throw std::runtime_error("Dx_CommandManager: computeQueue.Create Error!!");

	for (int i = 0; i < COM_NO; i++) {
		//コマンドアロケータ,コマンドリスト生成
		if (!grList[i].ListCreate(false, dev))throw std::runtime_error("Dx_CommandManager: ListCreate.Create Error!!");
		grList[i].createResourceBarrierList();
		grList[i].createUavResourceBarrierList();
		if (!coList[i].ListCreate(true, dev))throw std::runtime_error("Dx_CommandManager: ListCreate.Create Error!!");
		coList[i].createUavResourceBarrierList();
	}
}

Dx_CommandManager::~Dx_CommandManager() {

}

ID3D12CommandQueue* Dx_CommandManager::getGraphicsQueue() {
	return graphicsQueue.mCommandQueue.Get();
}

Dx_CommandListObj* Dx_CommandManager::getGraphicsComListObj(int comIndex) {
	return &grList[comIndex];
}

void Dx_CommandManager::RunGpuNotLock() {
	//クローズ後リストに加える
	static ID3D12CommandList* cmdsLists[COM_NO] = {};
	UINT cnt = 0;
	for (int i = 0; i < COM_NO; i++) {
		if (grList[i].mComState != Dx_CommandListObj::CLOSE)continue;
		cmdsLists[cnt++] = grList[i].mCommandList.Get();
		grList[i].mComState = Dx_CommandListObj::USED;
	}

	graphicsQueue.setCommandList(cnt, cmdsLists);
}

void Dx_CommandManager::RunGpu() {
	mtxGraphicsQueue.lock();
	RunGpuNotLock();
	mtxGraphicsQueue.unlock();
}

void Dx_CommandManager::WaitFenceNotLock() {
	graphicsQueue.waitFence();
}

void Dx_CommandManager::WaitFence() {
	mtxGraphicsQueue.lock();
	WaitFenceNotLock();
	mtxGraphicsQueue.unlock();
}

Dx_CommandListObj* Dx_CommandManager::getComputeComListObj(int comIndex) {
	return &coList[comIndex];
}

void Dx_CommandManager::RunGpuNotLockCom() {
	static ID3D12CommandList* cmdsLists[COM_NO] = {};
	UINT cnt = 0;
	for (int i = 0; i < COM_NO; i++) {
		if (coList[i].mComState != Dx_CommandListObj::CLOSE)continue;
		cmdsLists[cnt++] = coList[i].mCommandList.Get();
		coList[i].mComState = Dx_CommandListObj::USED;
	}

	computeQueue.setCommandList(cnt, cmdsLists);
}

void Dx_CommandManager::RunGpuCom() {
	mtxComputeQueue.lock();
	RunGpuNotLockCom();
	mtxComputeQueue.unlock();
}

void Dx_CommandManager::WaitFenceNotLockCom() {
	computeQueue.waitFence();
}

void Dx_CommandManager::WaitFenceCom() {
	mtxComputeQueue.lock();
	WaitFenceNotLockCom();
	mtxComputeQueue.unlock();
}

ComPtr<ID3D12Resource> Dx_CommandManager::CreateDefaultBuffer(
	int comIndex,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer, bool uav)
{
	ComPtr<ID3D12Resource> defaultBuffer;
	HRESULT hr;
	D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_GENERIC_READ;
	Dx_Device* device = Dx_Device::GetInstance();
	if (uav) {
		hr = device->createDefaultResourceBuffer_UNORDERED_ACCESS(defaultBuffer.GetAddressOf(), byteSize);
		after = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else {
		hr = device->createDefaultResourceBuffer(defaultBuffer.GetAddressOf(), byteSize);
	}
	if (FAILED(hr)) {
		return nullptr;
	}

	UINT64 uploadBufferSize = device->getRequiredIntermediateSize(defaultBuffer.Get());
	hr = device->createUploadResource(uploadBuffer.GetAddressOf(), uploadBufferSize);
	if (FAILED(hr)) {
		return nullptr;
	}

	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);

	cObj->ResourceBarrier(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	hr = cObj->CopyResourcesToGPU(uploadBuffer.Get(), defaultBuffer.Get(), initData, byteSize);
	if (FAILED(hr)) {
		return nullptr;
	}
	cObj->ResourceBarrier(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, after);

	return defaultBuffer;
}

HRESULT Dx_CommandManager::createTexture(int comIndex, const void* byteArr, DXGI_FORMAT format,
	ID3D12Resource** up, ID3D12Resource** def,
	int width, LONG_PTR RowPitch, int height, bool uav) {

	HRESULT hr = Dx_Device::GetInstance()->textureInit(width, height, up, def, format,
		D3D12_RESOURCE_STATE_COMMON, uav);
	if (FAILED(hr)) {
		return hr;
	}

	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);

	D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_GENERIC_READ;
	if (uav)afterState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	cObj->ResourceBarrier(*def, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	hr = cObj->CopyResourcesToGPU(*up, *def, byteArr, RowPitch);
	cObj->ResourceBarrier(*def, D3D12_RESOURCE_STATE_COPY_DEST, afterState);

	return hr;
}
