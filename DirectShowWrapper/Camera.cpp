//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　　　　  Cameraクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Camera.h"

Camera::Camera() {

	// SampleGrabber(Filter)を生成
	CoCreateInstance(CLSID_SampleGrabber,
		NULL,
		CLSCTX_INPROC,
		IID_IBaseFilter,
		(LPVOID *)&pSampleGrabberFilter);

	// FilterからISampleGrabberインターフェースを取得します
	pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber,
		(LPVOID *)&pSampleGrabber);

	// SampleGrabberを接続するフォーマットを指定。
	// ここの指定の仕方によりSampleGrabberの挿入箇所を
	// 決定できます。このサンプルのような指定をすると
	// 画面出力の寸前でサンプルを取得できます。
	ZeroMemory(&am_media_type, sizeof(am_media_type));
	am_media_type.majortype = MEDIATYPE_Video;
	am_media_type.subtype = MEDIASUBTYPE_RGB24;
	am_media_type.formattype = FORMAT_VideoInfo;
	pSampleGrabber->SetMediaType(&am_media_type);

	// GraphにSampleGrabber Filterを追加
	pGraphBuilder->AddFilter(pSampleGrabberFilter,
		L"Sample Grabber");

	//キャプチャ用デバイス取得

	// CaptureGraphBuilder2というキャプチャ用GraphBuilderを生成する
	CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
		IID_ICaptureGraphBuilder2,
		(LPVOID *)&pCaptureGraphBuilder2);

	// FilterGraphをセットする
	pCaptureGraphBuilder2->SetFiltergraph(pGraphBuilder);

	// MediaControlインターフェース取得
	pGraphBuilder->QueryInterface(IID_IMediaControl,
		(LPVOID *)&pMediaControl);

	// デバイスを列挙するためのCreateDevEnumを生成
	CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);

	// VideoInputDeviceを列挙するためのEnumMonikerを生成 
	pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumMoniker, 0);
	if (pEnumMoniker == NULL) {
		//エラー処理
	}

	// EnumMonikerをResetする
	// Resetすると、先頭から数えなおし
	pEnumMoniker->Reset();

	// 最初のMonikerを取得
	pEnumMoniker->Next(1, &pMoniker, &nFetched);

	// MonkierをFilterにBindする
	pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pDeviceFilter);

	// FilterGraphにデバイスフィルタを追加する
	pGraphBuilder->AddFilter(pDeviceFilter, L"Device Filter");

	pMoniker->Release();
	pEnumMoniker->Release();
	pCreateDevEnum->Release();

	// Graphを生成する
	pCaptureGraphBuilder2->RenderStream(&PIN_CATEGORY_PREVIEW,
		NULL, pDeviceFilter, NULL, pSampleGrabberFilter);

	// 接続情報取得。
	// この処理はRenderFileによりGraphが構成された後に実行
	pSampleGrabber->GetConnectedMediaType(&am_media_type);
	pVideoInfoHeader = (VIDEOINFOHEADER *)am_media_type.pbFormat;

	// Grab実行TRUE,実行しないFALSE  
	// SetBufferSamplesを行わないとバッファからデータを取得できない。
	pSampleGrabber->SetBufferSamples(TRUE);

	//自動表示しないようにする
	pVideoWindow->put_AutoShow(OAFALSE);

	// 再生開始
	pMediaControl->Run();

	// バッファを用意
	nBufferSize = am_media_type.lSampleSize;// データサイズ
	pBuffer = new BYTE[nBufferSize];

	//pBuffe x方向1ラインサイズ計算
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
