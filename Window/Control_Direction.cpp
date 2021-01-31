//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　            Controlクラス                                   **//
//**                                     Direction関数                                   **//
//*****************************************************************************************//

#include "Control.h"

Control *Control::co = NULL;

Control *Control::GetInstance() {
	if (co == NULL)co = new Control();
	return co;
}

Control::Control() {

}

void Control::DeleteInstance() {
	delete co;
}

void Control::Input(UINT msg, WPARAM wParam, LPARAM lParam) {

	DxInput* di = DxInput::GetInstance();

	switch (msg) {
	case WM_CLOSE:			//×ボタン
		PostQuitMessage(0);//アプリケーション終了処理,メッセージキューにWM_QUITをポスト
		break;
	case WM_MOUSEMOVE:
		di->SetCursorPos(LOWORD(lParam), HIWORD(lParam));
		break;
	}
}

Directionkey Control::Direction() {

	DxInput* di = DxInput::GetInstance();
	switch (di->checkKeyActionNo()) {

	case DIK_LEFT:
		return LEFT;
	case DIK_RIGHT:
		return RIGHT;
	case DIK_UP:
		return UP;
	case DIK_DOWN:
		return DOWN;
	case DIK_RCONTROL:
		return ENTER;
	case DIK_DELETE:
		return CANCEL;
		break;
	}
	return NOTPRESS;
}

Directionkey Control::Direction(bool f) {

	DxInput* di = DxInput::GetInstance();
	switch (di->checkKeyDownNo()) {

	case DIK_LEFT:
		return LEFT;
	case DIK_RIGHT:
		return RIGHT;
	case DIK_UP:
		return UP;
	case DIK_DOWN:
		return DOWN;
	case DIK_RCONTROL:
		return ENTER;
	case DIK_DELETE:
		return CANCEL;
		break;
	}
	return NOTPRESS;
}