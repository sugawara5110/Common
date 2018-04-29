//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@           Movie�N���X                                      **//
//**                                    GetFrame�֐�                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Movie.h"

Movie::Movie(char *fname) {

	// SampleGrabber(Filter)�𐶐�
	CoCreateInstance(CLSID_SampleGrabber,
		NULL,
		CLSCTX_INPROC,
		IID_IBaseFilter,
		(LPVOID *)&pSampleGrabberFilter);

	// Filter����ISampleGrabber�C���^�[�t�F�[�X���擾���܂�
	pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (LPVOID *)&pSampleGrabber);

	// SampleGrabber��ڑ�����t�H�[�}�b�g���w��B
	// �����̎w��̎d���ɂ��SampleGrabber�̑}���ӏ���
	// ����ł��܂��B���̃T���v���̂悤�Ȏw��������
	// ��ʏo�͂̐��O�ŃT���v�����擾�ł��܂��B
	ZeroMemory(&am_media_type, sizeof(am_media_type));
	am_media_type.majortype = MEDIATYPE_Video;
	am_media_type.subtype = MEDIASUBTYPE_RGB24;
	am_media_type.formattype = FORMAT_VideoInfo;
	pSampleGrabber->SetMediaType(&am_media_type);

	// Graph��SampleGrabber Filter��ǉ�
	pGraphBuilder->AddFilter(pSampleGrabberFilter, L"Sample Grabber");

	BSTR bstr;
	BSTR_Convert(fname, &bstr);

	// Graph�𐶐��B
	// ������SampleGrabber���܂�Graph�������I�ɍ쐬�����
	pMediaControl->RenderFile(bstr);
	SysFreeString(bstr);//bstr���

	// �ڑ����擾�B
	// ���̏�����RenderFile�ɂ��Graph���\�����ꂽ��Ɏ��s
	pSampleGrabber->GetConnectedMediaType(&am_media_type);
	pVideoInfoHeader = (VIDEOINFOHEADER *)am_media_type.pbFormat;

	// Grab���sTRUE,���s���Ȃ�FALSE  
	// SetBufferSamples���s��Ȃ��ƃo�b�t�@����f�[�^���擾�ł��Ȃ��B
	pSampleGrabber->SetBufferSamples(TRUE);

	//�����\�����Ȃ��悤�ɂ���
	pVideoWindow->put_AutoShow(OAFALSE);

	// �Đ��J�n
	pMediaControl->Run();

	// �o�b�t�@��p��
	nBufferSize = am_media_type.lSampleSize;// �f�[�^�T�C�Y
	pBuffer = new BYTE[nBufferSize];

	//pBuffe x����1���C���T�C�Y�v�Z
	linesize = pVideoInfoHeader->bmiHeader.biWidth * 3;
	if (linesize % sizeof(LONG) != 0)
		linesize += sizeof(LONG) - (linesize % sizeof(LONG));

	//�X�g���[���̎��ԕ����擾(�ŏ���1��擾�����ok)
	pMediaPosition->get_Duration(&time2);

	if (fileDelF)remove(fname);//�폜�t���OOn:�t�@�C���폜�B��������ƍ폜����Ă�
}

UINT **Movie::getframe(int width, int height) {
	hei = height;
	wid = width;
	//�ŐV�摜�f�[�^��pBuffer�Ɋi�[
	pSampleGrabber->GetCurrentBuffer(&nBufferSize, (long *)(pBuffer));

	if (m_pix == NULL) {
		m_pix = (UINT**)malloc(sizeof(UINT*) * hei);
		for (int i = 0; i < hei; i++) {
			m_pix[i] = (UINT*)malloc(sizeof(UINT) * wid);
		}
	}

	//�t���[���T�C�Y
	xs = pVideoInfoHeader->bmiHeader.biWidth;
	ys = pVideoInfoHeader->bmiHeader.biHeight;
	for (int j = 0; j < hei; j++) {
		for (int i = 0; i < wid; i++) {
			int offset = (j * ys / hei) * linesize + (i * xs / wid) * 3;

			m_pix[hei - j - 1][i] =
				(pBuffer[offset + 0] << 16) +
				(pBuffer[offset + 1] << 8) +
				pBuffer[offset + 2];
		}
	}
	return m_pix;
}

BYTE *Movie::getframe1(int width, int height) {
	hei = height;
	wid = width;
	//�ŐV�摜�f�[�^��pBuffer�Ɋi�[
	pSampleGrabber->GetCurrentBuffer(&nBufferSize, (long *)(pBuffer));

	int pitch = 4;
	int Width = wid * pitch;
	if (pix1 == nullptr) {
		pix1 = new BYTE[hei * Width];
	}

	//�t���[���T�C�Y
	xs = pVideoInfoHeader->bmiHeader.biWidth;
	ys = pVideoInfoHeader->bmiHeader.biHeight;
	for (int j = 0; j < hei; j++) {
		for (int i = 0; i < wid; i++) {
			int offset = (j * ys / hei) * linesize + (i * xs / wid) * 3;

			int i1 = i * pitch;
			pix1[Width * (hei - j - 1) + i1 + 2] = pBuffer[offset + 0];
			pix1[Width * (hei - j - 1) + i1 + 1] = pBuffer[offset + 1];
			pix1[Width * (hei - j - 1) + i1 + 0] = pBuffer[offset + 2];
			pix1[Width * (hei - j - 1) + i1 + 3] = 255;
		}
	}
	return pix1;
}

UINT **Movie::GetFrame(int width, int height) {

	//�Đ����x1.0�W�� �L���͈�0.1�`4.0 
	pMediaPosition->put_Rate(1.0);

	//�X�g���[���̍��v���ԕ�����Ƃ���A���݂̈ʒu���擾����B
	pMediaPosition->get_CurrentPosition(&time1);

	//���ʒu�ƏI���ʒu�������ꍇ�X�^�[�g�ʒu�ɃZ�b�g
	if (time1 == time2)pMediaPosition->put_CurrentPosition(0);

	return getframe(width, height);
}

BYTE *Movie::GetFrame1(int width, int height) {

	//�Đ����x1.0�W�� �L���͈�0.1�`4.0 
	pMediaPosition->put_Rate(1.0);

	//�X�g���[���̍��v���ԕ�����Ƃ���A���݂̈ʒu���擾����B
	pMediaPosition->get_CurrentPosition(&time1);

	//���ʒu�ƏI���ʒu�������ꍇ�X�^�[�g�ʒu�ɃZ�b�g
	if (time1 == time2)pMediaPosition->put_CurrentPosition(0);

	return getframe1(width, height);
}

Movie::~Movie() {

	if (m_pix != NULL) {
		for (int i = 0; i < hei; i++)free(m_pix[i]);
		free(m_pix);
		m_pix = NULL;
	}

	if (pix1) {
		delete[] pix1;
		pix1 = nullptr;
	}

	if (pBuffer != NULL) {
		free(pBuffer);//���������
		pBuffer = NULL;
	}
}