//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	         Winクラス (窓だけ)                              **//
//**                                         実装部                                      **//
//*****************************************************************************************//

#include "Win.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	Control *co = Control::GetInstance();
	co->Input(msg, wParam);
	return DefWindowProc(hWnd, msg, wParam, lParam);//アプリケーションが処理しないウィンドウメッセージに対しての既定の処理実行
}

int Createwindow(HWND *hWnd, HINSTANCE hInstance, int nCmdShow, UINT Width, UINT Height, TCHAR *clsname) {

	//ウインドウクラスの初期化
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); //この構造体のサイズ
	wcex.style = NULL;               //ウインドウスタイル(default)
	wcex.lpfnWndProc = WindowProc;  //メッセージ処理関数の登録
	wcex.cbClsExtra = 0;       //通常は0	                
	wcex.cbWndExtra = 0;      //通常は0					
	wcex.hInstance = hInstance; //インスタンスへのハンドル				
	wcex.hIcon = NULL;         //アイコン (無し)				
	wcex.hCursor = NULL;      //カーソルの形				
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); //背景				
	wcex.lpszMenuName = NULL;               //メニュー無し				
	wcex.lpszClassName = clsname;          //クラス名               
	wcex.hIconSm = NULL;                  //小アイコン			   

	//ウインドウクラスの登録(RegisterClassEx関数)
	if (!RegisterClassEx(&wcex))return -1;

	//ウインドウ生成ウインドウモード
	if (!(*hWnd = CreateWindow(clsname, //登録クラス名
		clsname,                      //ウインドウ名
		WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE,//ウインドウスタイル
		CW_USEDEFAULT, //ウインドウ横位置
		0,            //ウインドウ縦位置
		Width,         //ウインドウ幅
		Height,            //ウインドウ高さ
		NULL,          //親ウインドウハンドル
		NULL,         //メニュー,子ウインドウハンドル
		hInstance,   //アプリケーションインスタンスハンドル
		NULL)))     //ウインドウ作成データ
		return -1;

	// ウィンドウの表示
	ShowWindow(*hWnd, nCmdShow);
	// WM_PAINTが呼ばれないようにする
	ValidateRect(*hWnd, 0);

	return 0;
}

bool DispatchMSG(MSG *msg) {
	if (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) {
		if ((*msg).message == WM_QUIT) {  // PostQuitMessage()が呼ばれた(×押された)
			return FALSE;	//アプリ終了
		}
		else {
			// メッセージの翻訳とディスパッチWindowProc呼び出し
			TranslateMessage(msg);
			DispatchMessage(msg);
		}
	}
	return TRUE;
}



