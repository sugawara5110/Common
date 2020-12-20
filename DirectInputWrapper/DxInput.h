//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	        DxInput                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxInput_Header
#define Class_DxInput_Header

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

class DxInput {

private:
	static DxInput* di;
	static bool createF;
	static bool windowMode;

	HWND hWnd = nullptr;
	IDirectInput8* DInput = nullptr;
	//キーボード
	IDirectInputDevice8* DIDevKB = nullptr;
	bool Keystate[256];//更新時に押してたらON(押しっぱなしを拾う)
	bool KeyAction[256];//押した直後だけON(押しっぱなしはOFFになる)

	//マウス
	IDirectInputDevice8* DIMouse = nullptr;
	int wndWid = 0;
	int wndHgt = 0;

	long posX = 0;//マウス座標
	long posY = 0;

	bool LDown = false;//Update時点のボタン状態
	bool RDown = false;
	bool MDown = false;

	bool LAction = false;//ボタン押した直後だけONになる(クリックを拾うときに)
	bool RAction = false;
	bool MAction = false;

	DxInput() {};
	bool createDevice(const GUID id, IDirectInputDevice8** device, const DIDATAFORMAT format);

public:
	static DxInput* GetInstance();
	static void DeleteInstance();

	bool create(HWND hWnd);
	~DxInput();

	void KeyboardUpdate();
	int checkKeyDownNo();
	int checkKeyActionNo();
	bool checkKeyDown(int key) { return Keystate[key]; }
	bool checkKeyAction(int key) { return KeyAction[key]; }

	void MouseUpdate();
	long PosX() { return posX; }
	long PosY() { return posY; }

	bool isLAction() { return LAction; }
	bool isRAction() { return RAction; }
	bool isMAction() { return MAction; }

	bool isLDown() { return LDown; }
	bool isRDown() { return RDown; }
	bool isMDown() { return MDown; }

	void SetWindowMode(bool isWindowmode) { windowMode = isWindowmode; }//true:ウィンドウモード false:フルスクリーン

	void SetCursorPos(int x, int y)
	{
		if (!createF || !windowMode)return;
		posX = x;
		posY = y;
	}
};

#endif