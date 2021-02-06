//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@       DxCommandQueue                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxCommandQueue.h"

bool DxCommandQueue::Create(ID3D12Device5* dev, bool Compute) {
	//�t�F���X����
	//Command Queue�ɑ��M����Command List�̊��������m���邽�߂Ɏg�p
	if (FAILED(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)))) {
		ErrorMessage("CreateFence Error");
		return false;
	}
	//�R�}���h�L���[����
	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (Compute)type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; //GPU�^�C���A�E�g���L��
	if (FAILED(dev->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)))) {
		ErrorMessage("CreateCommandQueue Error");
		return false;
	}
	return true;
}

void DxCommandQueue::setCommandList(UINT numList, ID3D12CommandList* const* cmdsLists) {
	mCommandQueue->ExecuteCommandLists(numList, cmdsLists);
}

void DxCommandQueue::waitFence() {
	//�C���N�������g���ꂽ���Ƃŕ`�抮���Ɣ��f
	mCurrentFence++;
	//GPU��Ŏ��s����Ă���R�}���h������,�����I�Ƀt�F���X��mCurrentFence����������
	//(mFence->GetCompletedValue()�œ�����l��mCurrentFence�Ɠ����ɂȂ�)
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);//���̖��߈ȑO�̃R�}���h���X�g���҂ΏۂɂȂ�
	//�����܂łŃR�}���h�L���[���I�����Ă��܂���
	//���̃C�x���g����������������Ȃ��\���L��ׁ�if�Ń`�F�b�N���Ă���
	//GetCompletedValue():Fence����UINT64�̃J�E���^�擾(�����l0)
	if (mFence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		//����Fence�ɂ�����,mCurrentFence �̒l�ɂȂ�����C�x���g�𔭉΂�����
		mFence->SetEventOnCompletion(mCurrentFence, eventHandle);
		//�C�x���g�����΂���܂ő҂�(GPU�̏����҂�)����ɂ��GPU��̑S�R�}���h���s�I���܂ő҂�����
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}