//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	         Controlクラス                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Control_Header
#define Class_Control_Header

#include "../Direct3DWrapper/Dx12Process.h"
#include "../Direct3DWrapper//DxText.h"

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
	static Control *co;
	Directionkey keydownhistory;
	Directionkey directionkey;
	Directionkey directionkey2[2];
	int keyOffCount;

	Control();

public:
	static Control *GetInstance();
	static void DeleteInstance();

	void Input(UINT msg, WPARAM wParam);
	Directionkey Direction();
	Directionkey Direction(bool f);
};

#endif