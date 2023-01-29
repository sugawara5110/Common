//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@       Dx_CommandManager                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_CommandManager_Header
#define Class_Dx_CommandManager_Header

#include "DxCommandQueue.h"
#include <mutex>

class Dx_CommandManager {

private:
	const static int COM_NO = 32;

	DxCommandQueue graphicsQueue = {};
	DxCommandQueue computeQueue = {};
	Dx_CommandListObj grList[COM_NO] = {};
	Dx_CommandListObj coList[COM_NO] = {};

	static std::mutex mtxGraphicsQueue;
	static std::mutex mtxComputeQueue;

	static Dx_CommandManager* com;

	Dx_CommandManager() {}//�O������̃I�u�W�F�N�g�����֎~
	Dx_CommandManager(const Dx_CommandManager& obj) = delete;   // �R�s�[�R���X�g���N�^�֎~
	void operator=(const Dx_CommandManager& obj) = delete;// ������Z�q�֎~

	Dx_CommandManager(ID3D12Device5* dev);
	~Dx_CommandManager();

public:
	static void setNumResourceBarrier(int numResourceBarrier);
	static void InstanceCreate();
	static Dx_CommandManager* GetInstance();
	static void DeleteInstance();

	Dx_CommandListObj* getGraphicsComListObj(int comIndex);
	void RunGpuNotLock();
	void RunGpu();
	void WaitFenceNotLock();
	void WaitFence();
	ID3D12CommandQueue* getGraphicsQueue();

	Dx_CommandListObj* getComputeComListObj(int comIndex);
	void RunGpuNotLockCom();
	void RunGpuCom();
	void WaitFenceNotLockCom();
	void WaitFenceCom();
};

#endif