//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@       Dx_CommandManager                                    **//
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

	//�R�}���h�L���[����
	if (!graphicsQueue.Create(dev, false))Dx_Util::ErrorMessage("Dx_CommandManager: graphicsQueue.Create Error!!");
	if (!computeQueue.Create(dev, true))Dx_Util::ErrorMessage("Dx_CommandManager: computeQueue.Create Error!!");

	for (int i = 0; i < COM_NO; i++) {
		//�R�}���h�A���P�[�^,�R�}���h���X�g����
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
	//�N���[�Y�ナ�X�g�ɉ�����
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
