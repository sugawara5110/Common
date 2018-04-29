//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@�@�@�@  Camera�N���X                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Camera.h"

Camera::Camera() {

	// SampleGrabber(Filter)�𐶐�
	CoCreateInstance(CLSID_SampleGrabber,
		NULL,
		CLSCTX_INPROC,
		IID_IBaseFilter,
		(LPVOID *)&pSampleGrabberFilter);

	// Filter����ISampleGrabber�C���^�[�t�F�[�X���擾���܂�
	pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber,
		(LPVOID *)&pSampleGrabber);

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
	pGraphBuilder->AddFilter(pSampleGrabberFilter,
		L"Sample Grabber");

	//�L���v�`���p�f�o�C�X�擾

	// CaptureGraphBuilder2�Ƃ����L���v�`���pGraphBuilder�𐶐�����
	CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
		IID_ICaptureGraphBuilder2,
		(LPVOID *)&pCaptureGraphBuilder2);

	// FilterGraph���Z�b�g����
	pCaptureGraphBuilder2->SetFiltergraph(pGraphBuilder);

	// MediaControl�C���^�[�t�F�[�X�擾
	pGraphBuilder->QueryInterface(IID_IMediaControl,
		(LPVOID *)&pMediaControl);

	// �f�o�C�X��񋓂��邽�߂�CreateDevEnum�𐶐�
	CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);

	// VideoInputDevice��񋓂��邽�߂�EnumMoniker�𐶐� 
	pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumMoniker, 0);
	if (pEnumMoniker == NULL) {
		//�G���[����
	}

	// EnumMoniker��Reset����
	// Reset����ƁA�擪���琔���Ȃ���
	pEnumMoniker->Reset();

	// �ŏ���Moniker���擾
	pEnumMoniker->Next(1, &pMoniker, &nFetched);

	// Monkier��Filter��Bind����
	pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pDeviceFilter);

	// FilterGraph�Ƀf�o�C�X�t�B���^��ǉ�����
	pGraphBuilder->AddFilter(pDeviceFilter, L"Device Filter");

	pMoniker->Release();
	pEnumMoniker->Release();
	pCreateDevEnum->Release();

	// Graph�𐶐�����
	pCaptureGraphBuilder2->RenderStream(&PIN_CATEGORY_PREVIEW,
		NULL, pDeviceFilter, NULL, pSampleGrabberFilter);

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
}

UINT **Camera::GetFrame(int width, int height) {
	return getframe(width, height);
}

BYTE *Camera::GetFrame1(int width, int height) {
	return getframe1(width, height);
}
