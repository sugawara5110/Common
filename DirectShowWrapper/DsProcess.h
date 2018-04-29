//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DsProcessクラス                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DsProcess_Header
#define Class_DsProcess_Header

#define _CRT_SECURE_NO_WARNINGS
#include <dshow.h>
#include <qedit.h>

#define D_RELEASE(p)    if(p){p->Release();  p=NULL;}

#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"quartz.lib")

class DsProcess{

protected:
	IGraphBuilder *pGraphBuilder;      //インターフェース,グラフへのフィルタの追加、2 つのピンの接続等
	IBaseFilter *pSampleGrabberFilter;//インターフェース,フィルタを制御(Movie使用時)
	IBaseFilter *pDeviceFilter;      //カメラ使用時
	ICaptureGraphBuilder2 *pCaptureGraphBuilder2; //キャプチャ用
	ISampleGrabber *pSampleGrabber;  //インターフェース,フィルタ グラフ内を通る個々のメディア サンプルを取得(Movie使用時)
	IVideoWindow *pVideoWindow;     //インターフェース,ビデオ ウィンドウのプロパティを設定
	IMediaControl *pMediaControl;  //インターフェース,フィルタ グラフを通るデータ フローを制御
	IMediaPosition *pMediaPosition;  //インターフェース,ストリーム内の位置をシーク
	REFTIME time2;                   //動画の全再生時間
	REFTIME time1;                  //動画の現再生位置
	IBasicAudio *pBasicAudio;      //インターフェース,オーディオ ストリームのボリュームとバランスを制御
	static bool fileDelF;

	DsProcess();
	void BSTR_Convert(char *fname, BSTR *bstr);

public:
	virtual ~DsProcess();
	
	static void ComInitialize(){ CoInitialize(NULL); }//DirectX使わない時使用
	static void ComUninitialize(){ CoUninitialize(); }
	static void FileDeleteOnReleaseAfter() { fileDelF = TRUE; }//ファイル使用後ファイルを削除したい場合
};

#endif