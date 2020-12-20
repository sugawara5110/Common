//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	         Controlクラス                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Control_Header
#define Class_Control_Header

#include "../DirectInputWrapper/DxInput.h"

enum Directionkey {
	TWOPRESS,
	NOTPRESS,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	ENTER,
	CANCEL,
	CAMPOS //視点座標用
};

class Control {

private:
	static Control* co;

	Control();

public:
	static Control* GetInstance();
	static void DeleteInstance();

	void Input(UINT msg, WPARAM wParam, LPARAM lParam);
	Directionkey Direction();
	Directionkey Direction(bool f);
};

#endif