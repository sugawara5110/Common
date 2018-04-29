//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	         Win�N���X (������)                              **//
//**                                         ������                                      **//
//*****************************************************************************************//

#include "Win.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	Control *co = Control::GetInstance();
	co->Input(msg, wParam);
	return DefWindowProc(hWnd, msg, wParam, lParam);//�A�v���P�[�V�������������Ȃ��E�B���h�E���b�Z�[�W�ɑ΂��Ă̊���̏������s
}

int Createwindow(HWND *hWnd, HINSTANCE hInstance, int nCmdShow, UINT Width, UINT Height, TCHAR *clsname) {

	//�E�C���h�E�N���X�̏�����
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); //���̍\���̂̃T�C�Y
	wcex.style = NULL;               //�E�C���h�E�X�^�C��(default)
	wcex.lpfnWndProc = WindowProc;  //���b�Z�[�W�����֐��̓o�^
	wcex.cbClsExtra = 0;       //�ʏ��0	                
	wcex.cbWndExtra = 0;      //�ʏ��0					
	wcex.hInstance = hInstance; //�C���X�^���X�ւ̃n���h��				
	wcex.hIcon = NULL;         //�A�C�R�� (����)				
	wcex.hCursor = NULL;      //�J�[�\���̌`				
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); //�w�i				
	wcex.lpszMenuName = NULL;               //���j���[����				
	wcex.lpszClassName = clsname;          //�N���X��               
	wcex.hIconSm = NULL;                  //���A�C�R��			   

	//�E�C���h�E�N���X�̓o�^(RegisterClassEx�֐�)
	if (!RegisterClassEx(&wcex))return -1;

	//�E�C���h�E�����E�C���h�E���[�h
	if (!(*hWnd = CreateWindow(clsname, //�o�^�N���X��
		clsname,                      //�E�C���h�E��
		WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE,//�E�C���h�E�X�^�C��
		CW_USEDEFAULT, //�E�C���h�E���ʒu
		0,            //�E�C���h�E�c�ʒu
		Width,         //�E�C���h�E��
		Height,            //�E�C���h�E����
		NULL,          //�e�E�C���h�E�n���h��
		NULL,         //���j���[,�q�E�C���h�E�n���h��
		hInstance,   //�A�v���P�[�V�����C���X�^���X�n���h��
		NULL)))     //�E�C���h�E�쐬�f�[�^
		return -1;

	// �E�B���h�E�̕\��
	ShowWindow(*hWnd, nCmdShow);
	// WM_PAINT���Ă΂�Ȃ��悤�ɂ���
	ValidateRect(*hWnd, 0);

	return 0;
}

bool DispatchMSG(MSG *msg) {
	if (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) {
		if ((*msg).message == WM_QUIT) {  // PostQuitMessage()���Ă΂ꂽ(�~�����ꂽ)
			return FALSE;	//�A�v���I��
		}
		else {
			// ���b�Z�[�W�̖|��ƃf�B�X�p�b�`WindowProc�Ăяo��
			TranslateMessage(msg);
			DispatchMessage(msg);
		}
	}
	return TRUE;
}



