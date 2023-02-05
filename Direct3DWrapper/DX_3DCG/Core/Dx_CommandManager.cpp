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
		com = new Dx_CommandManager(Dx_Device::GetInstance()->getDevice());
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
	if (!graphicsQueue.Create(dev, false))Dx_Util::ErrorMessage("Dx_CommandManager: graphicsQueue.Create Error!!");
	if (!computeQueue.Create(dev, true))Dx_Util::ErrorMessage("Dx_CommandManager: computeQueue.Create Error!!");

	for (int i = 0; i < COM_NO; i++) {
		//コマンドアロケータ,コマンドリスト生成
		if (!grList[i].ListCreate(false, dev))Dx_Util::ErrorMessage("Dx_CommandManager: ListCreate.Create Error!!");
		grList[i].createResourceBarrierList();
		grList[i].createUavResourceBarrierList();
		if (!coList[i].ListCreate(true, dev))Dx_Util::ErrorMessage("Dx_CommandManager: ListCreate.Create Error!!");
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
		if (grList[i].mComState != CLOSE)continue;
		cmdsLists[cnt++] = grList[i].mCommandList.Get();
		grList[i].mComState = USED;
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
		if (coList[i].mComState != CLOSE)continue;
		cmdsLists[cnt++] = coList[i].mCommandList.Get();
		coList[i].mComState = USED;
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
