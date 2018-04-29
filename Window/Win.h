//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	         Winクラス (窓だけ)                              **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Win_Header
#define Class_Win_Header

#include <Windows.h>
#include "Control.h"

//-------------------------------------------------------------
// メッセージ処理用コールバック関数
// 引数
//		hWnd	: ウィンドウハンドル
//		msg		: メッセージ
//		wParam	: メッセージの最初のパラメータ(押されたキーの特定等に使用)
//		lParam	: メッセージの2番目のパラメータ
// 戻り値
//		メッセージ処理結果
//-------------------------------------------------------------
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int Createwindow(HWND *hWnd, HINSTANCE hInstance, int nCmdShow, UINT Width, UINT Height, TCHAR *clsname);

bool DispatchMSG(MSG *msg);

#endif

