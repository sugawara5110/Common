//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DsProcessクラス                                   **//
//**                                   DsProcess関数                                     **//
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

	// FilterGraphを生成
	CoCreateInstance(CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC,
		IID_IGraphBuilder,
		(LPVOID *)&pGraphBuilder);

	// MediaControlインターフェース取得
	pGraphBuilder->QueryInterface(IID_IMediaControl,
		(LPVOID *)&pMediaControl);

	// IVideoWindowインターフェース取得
	pGraphBuilder->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow);

	//MediaPositionインターフェース取得
	pGraphBuilder->QueryInterface(IID_IMediaPosition,
		(LPVOID *)&pMediaPosition);

	//BasicAudioインターフェース取得
	pGraphBuilder->QueryInterface(IID_IBasicAudio,
		(LPVOID *)&pBasicAudio);
}

void DsProcess::BSTR_Convert(char *fname, BSTR *bstr){
	LPSTR lstr = fname;
	//BSTRに必要なバッファサイズを求める(directshow用)
	int bstrlen = (int)MultiByteToWideChar(
		CP_ACP,		 // コードページ ANSI コードページ
		0,			// 文字の種類を指定するフラグ
		lstr,	   // マップ元文字列のアドレス
		strlen(lstr), // マップ元文字列のバイト数
		NULL,		 // マップ先ワイド文字列を入れるバッファのアドレス
		0			// バッファのサイズ
		);

	//バッファを確保する
	*bstr = SysAllocStringLen(NULL, bstrlen);

	//BSTRに複製
	MultiByteToWideChar(
		CP_ACP,
		0,
		lstr,
		strlen(lstr),
		*bstr,      //RenderFileの引数に使う
		bstrlen
		);
}

DsProcess::~DsProcess() {
	D_RELEASE(pBasicAudio);
	D_RELEASE(pMediaPosition);
	D_RELEASE(pMediaControl);
	D_RELEASE(pVideoWindow);
	//解放の順番の関係でここに記述
	D_RELEASE(pSampleGrabber);
	D_RELEASE(pSampleGrabberFilter);
	D_RELEASE(pDeviceFilter);
	D_RELEASE(pCaptureGraphBuilder2);
	D_RELEASE(pGraphBuilder);
}
