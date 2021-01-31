//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	         Win�N���X (������)                              **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Win_Header
#define Class_Win_Header

#include <Windows.h>
#include "Control.h"
#pragma comment(lib,"winmm.lib")

//-------------------------------------------------------------
// ���b�Z�[�W�����p�R�[���o�b�N�֐�
// ����
//		hWnd	: �E�B���h�E�n���h��
//		msg		: ���b�Z�[�W
//		wParam	: ���b�Z�[�W�̍ŏ��̃p�����[�^(�����ꂽ�L�[�̓��蓙�Ɏg�p)
//		lParam	: ���b�Z�[�W��2�Ԗڂ̃p�����[�^
// �߂�l
//		���b�Z�[�W��������
//-------------------------------------------------------------
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int Createwindow(HWND* hWnd, HINSTANCE hInstance, int nCmdShow,
	UINT Width, UINT Height, TCHAR* clsname);

bool DispatchMSG(MSG *msg);

#endif

