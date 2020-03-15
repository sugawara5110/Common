//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           Movieクラス                                      **//
//**                                    GetFrame関数                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Movie.h"

Movie::Movie(char* fname) {

	// SampleGrabber(Filter)を生成
	CoCreateInstance(CLSID_SampleGrabber,
		NULL,
		CLSCTX_INPROC,
		IID_IBaseFilter,
		(LPVOID*)&pSampleGrabberFilter);

	// FilterからISampleGrabberインターフェースを取得します
	pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (LPVOID*)&pSampleGrabber);

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
	pGraphBuilder->AddFilter(pSampleGrabberFilter, L"Sample Grabber");

	BSTR bstr;
	BSTR_Convert(fname, &bstr);

	// Graphを生成。
	// ここでSampleGrabberを含んだGraphが自動的に作成される
	pMediaControl->RenderFile(bstr);
	SysFreeString(bstr);//bstr解放

	// 接続情報取得。
	// この処理はRenderFileによりGraphが構成された後に実行
	pSampleGrabber->GetConnectedMediaType(&am_media_type);
	pVideoInfoHeader = (VIDEOINFOHEADER*)am_media_type.pbFormat;

	// Grab実行TRUE,実行しないFALSE  
	// SetBufferSamplesを行わないとバッファからデータを取得できない。
	pSampleGrabber->SetBufferSamples(TRUE);

	//自動表示しないようにする
	pVideoWindow->put_AutoShow(OAFALSE);

	// 再生開始
	pMediaControl->Run();

	pBasicAudio->put_Volume(-10000);//音量調整(-10000 ～ 0)

	// バッファを用意
	nBufferSize = am_media_type.lSampleSize;// データサイズ
	pBuffer = new BYTE[nBufferSize];

	//pBuffe x方向1ラインサイズ計算
	linesize = pVideoInfoHeader->bmiHeader.biWidth * 3;
	if (linesize % sizeof(LONG) != 0)
		linesize += sizeof(LONG) - (linesize % sizeof(LONG));

	//ストリームの時間幅を取得(最初に1回取得すればok)
	pMediaPosition->get_Duration(&time2);

	if (fileDelF)remove(fname);//削除フラグOn:ファイル削除。解放されると削除されてる
}

UINT** Movie::getframe(int width, int height) {
	hei = height;
	wid = width;
	//最新画像データをpBufferに格納
	pSampleGrabber->GetCurrentBuffer(&nBufferSize, (long*)(pBuffer));

	if (m_pix == NULL) {
		m_pix = (UINT**)malloc(sizeof(UINT*) * hei);
		for (int i = 0; i < hei; i++) {
			m_pix[i] = (UINT*)malloc(sizeof(UINT) * wid);
		}
	}

	//フレームサイズ
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

BYTE* Movie::getframe1(int width, int height) {
	hei = height;
	wid = width;
	//最新画像データをpBufferに格納
	pSampleGrabber->GetCurrentBuffer(&nBufferSize, (long*)(pBuffer));

	int pitch = 4;
	int Width = wid * pitch;
	if (pix == nullptr) {
		pix = new BYTE[hei * Width];
	}

	//フレームサイズ
	xs = pVideoInfoHeader->bmiHeader.biWidth;
	ys = pVideoInfoHeader->bmiHeader.biHeight;
	for (int j = 0; j < hei; j++) {
		for (int i = 0; i < wid; i++) {
			int offset = (j * ys / hei) * linesize + (i * xs / wid) * 3;

			int i1 = i * pitch;
			pix[Width * (hei - j - 1) + i1 + 2] = pBuffer[offset + 0];
			pix[Width * (hei - j - 1) + i1 + 1] = pBuffer[offset + 1];
			pix[Width * (hei - j - 1) + i1 + 0] = pBuffer[offset + 2];
			pix[Width * (hei - j - 1) + i1 + 3] = 255;
		}
	}
	return pix;
}

BYTE* Movie::GetFrame(int width, int height, BYTE Threshold,
	BYTE addR, BYTE addG, BYTE addB, BYTE addA) {

	//再生速度1.0標準 有効範囲0.1～4.0 
	pMediaPosition->put_Rate(1.0);

	//ストリームの合計時間幅を基準とする、現在の位置を取得する。
	pMediaPosition->get_CurrentPosition(&time1);

	//現位置と終了位置が同じ場合スタート位置にセット
	if (time1 == time2)pMediaPosition->put_CurrentPosition(0);

	hei = height;
	wid = width;
	//最新画像データをpBufferに格納
	pSampleGrabber->GetCurrentBuffer(&nBufferSize, (long*)(pBuffer));

	int pitch = 4;
	int Width = wid * pitch;
	if (pix == nullptr) {
		pix = new BYTE[hei * Width];
	}

	//フレームサイズ
	xs = pVideoInfoHeader->bmiHeader.biWidth;
	ys = pVideoInfoHeader->bmiHeader.biHeight;
	for (int j = 0; j < hei; j++) {
		for (int i = 0; i < wid; i++) {
			int offset = (j * ys / hei) * linesize + (i * xs / wid) * 3;

			int i1 = i * pitch;
			pix[Width * (hei - j - 1) + i1 + 2] = pBuffer[offset + 0] + addR;
			pix[Width * (hei - j - 1) + i1 + 1] = pBuffer[offset + 1] + addG;
			pix[Width * (hei - j - 1) + i1 + 0] = pBuffer[offset + 2] + addB;
			pix[Width * (hei - j - 1) + i1 + 3] = 255 + addA;

			if (pBuffer[offset + 0] < Threshold &&
				pBuffer[offset + 1] < Threshold &&
				pBuffer[offset + 2] < Threshold) {
				pix[Width * (hei - j - 1) + i1 + 3] = 0;
			}
		}
	}
	return pix;
}

void Movie::sound(long volume) {
	pBasicAudio->put_Volume(volume);//音量調整(-10000 ～ 0)
}

Movie::~Movie() {

	if (m_pix != NULL) {
		for (int i = 0; i < hei; i++)free(m_pix[i]);
		free(m_pix);
		m_pix = NULL;
	}

	if (pix) {
		delete[] pix;
		pix = nullptr;
	}

	if (pBuffer != NULL) {
		free(pBuffer);//メモリ解放
		pBuffer = NULL;
	}
}