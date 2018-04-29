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

	keydownhistory = NOTPRESS;
	directionkey = NOTPRESS;
	directionkey2[2] = { NOTPRESS };
	keyOffCount = 0;
}

void Control::DeleteInstance() {
	delete co;
}

void Control::Input(UINT msg, WPARAM wParam) {
	switch (msg) {
	case WM_CLOSE:			//×ボタン
		PostQuitMessage(0);//アプリケーション終了処理,メッセージキューにWM_QUITをポスト
		break;
	case WM_KEYDOWN:
		switch ((CHAR)wParam) {
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		case VK_LEFT:
			directionkey = LEFT;
			break;
		case VK_RIGHT:
			directionkey = RIGHT;
			break;
		case VK_UP:
			directionkey = UP;
			break;
		case VK_DOWN:
			directionkey = DOWN;
			break;
		case VK_CONTROL:
			directionkey = ENTER;
			break;
		case VK_DELETE:
			directionkey = CANCEL;
			break;
		default:
			directionkey = NOTPRESS;
			break;
		}
		break;
	default:
		directionkey = NOTPRESS;
		break;
	}
}

Directionkey Control::Direction() {

	directionkey2[0] = directionkey;
	switch (directionkey2[0]) {

	case LEFT:
		if (keydownhistory == LEFT)directionkey2[0] = TWOPRESS;
		else keydownhistory = LEFT;
		break;
	case RIGHT:
		if (keydownhistory == RIGHT)directionkey2[0] = TWOPRESS;
		else keydownhistory = RIGHT;
		break;
	case UP:
		if (keydownhistory == UP)directionkey2[0] = TWOPRESS;
		else keydownhistory = UP;
		break;
	case DOWN:
		if (keydownhistory == DOWN)directionkey2[0] = TWOPRESS;
		else keydownhistory = DOWN;
		break;
	case ENTER:
		if (keydownhistory == ENTER)directionkey2[0] = TWOPRESS;
		else keydownhistory = ENTER;
		break;
	case CANCEL:
		if (keydownhistory == CANCEL)directionkey2[0] = TWOPRESS;
		else keydownhistory = CANCEL;
		break;
	case NOTPRESS:
		keydownhistory = NOTPRESS;
		break;
	}
	return directionkey2[0];
}

Directionkey Control::Direction(bool f) {

	//DxText::GetInstance()->UpDateValue(directionkey2[1], 400, 400, 30.0f, 5, { 0.3f, 1.0f, 0.3f, 1.0f });

	Directionkey dir = directionkey;

	if (dir == NOTPRESS) { keyOffCount++; }
	else {
		directionkey2[1] = dir; keyOffCount = 0;
	}

	if (keyOffCount >= T_float::GetUps() / 10) {
		directionkey2[1] = NOTPRESS;
		keyOffCount = 0;
	}

	return directionkey2[1];
}