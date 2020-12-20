//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	        DxInput                                          **//
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
	//�L�[�{�[�h
	IDirectInputDevice8* DIDevKB = nullptr;
	bool Keystate[256];//�X�V���ɉ����Ă���ON(�������ςȂ����E��)
	bool KeyAction[256];//���������ゾ��ON(�������ςȂ���OFF�ɂȂ�)

	//�}�E�X
	IDirectInputDevice8* DIMouse = nullptr;
	int wndWid = 0;
	int wndHgt = 0;

	long posX = 0;//�}�E�X���W
	long posY = 0;

	bool LDown = false;//Update���_�̃{�^�����
	bool RDown = false;
	bool MDown = false;

	bool LAction = false;//�{�^�����������ゾ��ON�ɂȂ�(�N���b�N���E���Ƃ���)
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

	void SetWindowMode(bool isWindowmode) { windowMode = isWindowmode; }//true:�E�B���h�E���[�h false:�t���X�N���[��

	void SetCursorPos(int x, int y)
	{
		if (!createF || !windowMode)return;
		posX = x;
		posY = y;
	}
};

#endif