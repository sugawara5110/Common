//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          DsProcess�N���X                                   **//
//**                                   DsProcess�֐�                                     **//
//*****************************************************************************************//

#include "DsProcess.h"

bool DsProcess::fileDelF = FALSE;

DsProcess::DsProcess(){

	pGraphBuilder = NULL;
	pCaptureGraphBuilder2 = NULL;
	pSampleGrabberFilter = NULL;
	pDeviceFilter = NULL;
	pSampleGrabber = NULL;
	pVideoWindow = NULL;
	pMediaControl = NULL;
	pMediaPosition = NULL;
	pBasicAudio = NULL;

	// FilterGraph�𐶐�
	CoCreateInstance(CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC,
		IID_IGraphBuilder,
		(LPVOID *)&pGraphBuilder);

	// MediaControl�C���^�[�t�F�[�X�擾
	pGraphBuilder->QueryInterface(IID_IMediaControl,
		(LPVOID *)&pMediaControl);

	// IVideoWindow�C���^�[�t�F�[�X�擾
	pGraphBuilder->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow);

	//MediaPosition�C���^�[�t�F�[�X�擾
	pGraphBuilder->QueryInterface(IID_IMediaPosition,
		(LPVOID *)&pMediaPosition);

	//BasicAudio�C���^�[�t�F�[�X�擾
	pGraphBuilder->QueryInterface(IID_IBasicAudio,
		(LPVOID *)&pBasicAudio);
}

void DsProcess::BSTR_Convert(char *fname, BSTR *bstr){
	LPSTR lstr = fname;
	//BSTR�ɕK�v�ȃo�b�t�@�T�C�Y�����߂�(directshow�p)
	int bstrlen = (int)MultiByteToWideChar(
		CP_ACP,		 // �R�[�h�y�[�W ANSI �R�[�h�y�[�W
		0,			// �����̎�ނ��w�肷��t���O
		lstr,	   // �}�b�v��������̃A�h���X
		strlen(lstr), // �}�b�v��������̃o�C�g��
		NULL,		 // �}�b�v�惏�C�h�����������o�b�t�@�̃A�h���X
		0			// �o�b�t�@�̃T�C�Y
		);

	//�o�b�t�@���m�ۂ���
	*bstr = SysAllocStringLen(NULL, bstrlen);

	//BSTR�ɕ���
	MultiByteToWideChar(
		CP_ACP,
		0,
		lstr,
		strlen(lstr),
		*bstr,      //RenderFile�̈����Ɏg��
		bstrlen
		);
}

DsProcess::~DsProcess() {
	D_RELEASE(pBasicAudio);
	D_RELEASE(pMediaPosition);
	D_RELEASE(pMediaControl);
	D_RELEASE(pVideoWindow);
	//����̏��Ԃ̊֌W�ł����ɋL�q
	D_RELEASE(pSampleGrabber);
	D_RELEASE(pSampleGrabberFilter);
	D_RELEASE(pDeviceFilter);
	D_RELEASE(pCaptureGraphBuilder2);
	D_RELEASE(pGraphBuilder);
}
