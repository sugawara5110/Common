//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	        DxInput                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxInput.h"

DxInput* DxInput::di = nullptr;
bool DxInput::createF = false;
bool DxInput::windowMode = true;

DxInput* DxInput::GetInstance() {
	if (!di)di = new DxInput();
	return di;
}

void DxInput::DeleteInstance() {
	if (di) {
		delete di;
		di = nullptr;
		createF = false;
	}
}

bool DxInput::createDevice(const GUID id, IDirectInputDevice8** device, const DIDATAFORMAT format) {
	//�f�o�C�X���쐬
	HRESULT hr = DInput->CreateDevice(id, device, NULL);
	if (FAILED(hr))
	{
		return false;
	}
	IDirectInputDevice8* dev = *device;
	//�f�[�^�t�H�[�}�b�g�̐ݒ�
	hr = dev->SetDataFormat(&format);
	if (FAILED(hr))
	{
		return false;
	}
	//�o�b�t�@�T�C�Y�̐ݒ�
	DIPROPDWORD diprop;
	diprop.diph.dwSize = sizeof(diprop);
	diprop.diph.dwHeaderSize = sizeof(diprop.diph);
	diprop.diph.dwObj = 0;
	diprop.diph.dwHow = DIPH_DEVICE;
	diprop.dwData = 1000;
	hr = dev->SetProperty(DIPROP_BUFFERSIZE, &diprop.diph);
	if (FAILED(hr)) {
		return false;
	}

	//�������[�h�̐ݒ�
	hr = dev->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))
	{
		return false;
	}

	//���͂�������
	dev->Acquire();

	return true;
}

bool DxInput::create(HWND hwnd) {
	hWnd = hwnd;
	HRESULT hr = DirectInput8Create(::GetModuleHandle(NULL), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (void**)&DInput, NULL);
	if (FAILED(hr)) {
		return false;
	}

	// �L�[�{�[�h�f�o�C�X�쐬
	createDevice(GUID_SysKeyboard, &DIDevKB, c_dfDIKeyboard);

	ZeroMemory(Keystate, sizeof(Keystate));
	ZeroMemory(KeyAction, sizeof(KeyAction));

	//�}�E�X�f�o�C�X�쐬
	createDevice(GUID_SysMouse, &DIMouse, c_dfDIMouse2);

	RECT rect;
	GetClientRect(hWnd, &rect);
	wndWid = rect.right - rect.left;
	wndHgt = rect.bottom - rect.top;
	posX = wndWid / 2;
	posY = wndHgt / 2;

	createF = true;

	return true;
}

DxInput::~DxInput() {
	DIDevKB->Unacquire();//���͋֎~
	DIDevKB->Release();
	DIDevKB = nullptr;
	DIMouse->Unacquire();//���͋֎~
	DIMouse->Release();
	DIMouse = nullptr;
	ZeroMemory(Keystate, sizeof(Keystate));
	ZeroMemory(KeyAction, sizeof(KeyAction));
	DInput->Release();
	DInput = nullptr;
}

void DxInput::KeyboardUpdate() {

	if (!createF)return;

	ZeroMemory(KeyAction, sizeof(KeyAction));

	DIDEVICEOBJECTDATA od;
	DWORD dwItems = 1;
	HRESULT hr;
	while (true) {
		hr = DIDevKB->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwItems, 0);

		if (hr == DIERR_INPUTLOST) {
			DIDevKB->Acquire();
		}
		else
			if (dwItems == 0 || FAILED(hr)) {

				if (hr == DIERR_NOTACQUIRED) {
					DIDevKB->Acquire();
				}

				break;
			}
			else {

				Keystate[od.dwOfs] = (od.dwData & 0x80) ? true : false;

				if (Keystate[od.dwOfs]) {
					KeyAction[od.dwOfs] = true;
				}
			}
	}
}

int DxInput::checkKeyDownNo() {
	for (int i = 0; i < 256; i++) {
		if (Keystate[i])return i;
	}
	return -1;
}

int DxInput::checkKeyActionNo() {
	for (int i = 0; i < 256; i++) {
		if (KeyAction[i])return i;
	}
	return -1;
}

void DxInput::MouseUpdate() {

	if (!createF)return;

	LAction = false;
	RAction = false;
	MAction = false;

	DIDEVICEOBJECTDATA od;
	DWORD dwItems = 1;
	HRESULT hr;
	while (true) {
		hr = DIMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwItems, 0);

		if (hr == DIERR_INPUTLOST) {
			DIMouse->Acquire();
		}
		else
			if (dwItems == 0 || FAILED(hr)) {

				if (hr == DIERR_NOTACQUIRED) {
					DIMouse->Acquire();
				}

				break;
			}
			else {
				switch (od.dwOfs) {
				case DIMOFS_X:
					if (!windowMode) {
						posX += (int)od.dwData;
						posX = max(0, min(posX, wndWid));//�E�B���h�E����͂ݏo�Ȃ��悤����
					}
					break;
				case DIMOFS_Y:
					if (!windowMode) {
						posY += (int)od.dwData;
						posY = max(0, min(posY, wndHgt));
					}
					break;
				case DIMOFS_BUTTON0://���{�^��
					LDown = (od.dwData & 0x80) ? true : false;
					if (LDown) { LAction = true; }
					break;
				case DIMOFS_BUTTON1://�E�{�^��
					RDown = (od.dwData & 0x80) ? true : false;
					if (RDown) { RAction = true; }
					break;
				case DIMOFS_BUTTON2://���{�^��
					MDown = (od.dwData & 0x80) ? true : false;
					if (MDown) { MAction = true; }
					break;
				}
			}
	}
}