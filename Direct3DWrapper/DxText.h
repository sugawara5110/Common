//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	         DxText�N���X                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxText_Header
#define Class_DxText_Header

#include "DX_3DCG\Dx12ProcessCore.h"
#define STRTEX_MAX_PCS 40
#define STR_MAX_LENGTH 40
#define VAL_PCS 10

class DxText {

private:
	Dx12Process *dx;
	PolygonData2D text[STRTEX_MAX_PCS];       //�����`��p
	PolygonData2D value[VAL_PCS];              //�ϐ����p
	TCHAR str[STRTEX_MAX_PCS][STR_MAX_LENGTH]; //�o�^�e�L�X�g
	float f_size[STRTEX_MAX_PCS];              //�o�^�e�L�X�g�̃t�H���g�T�C�Y
	int strcnt[STRTEX_MAX_PCS];       //�o�^�e�L�X�g������ 
	HFONT hFont, oldFont;           //�t�H���g
	HDC hdc;                       //�f�o�C�X�R���e�L�X�g
	int CreateTextNo;              //�e�L�X�g�쐬�^�[�Q�b�g
	BYTE *ptr;                    //�r�b�g�}�b�v�z��
	TEXTMETRIC *TM;    //�������C�A�E�g
	GLYPHMETRICS *GM; //�������C�A�E�g
	DWORD *allsize;   //�e�v�f�܂ł̍��v�T�C�Y
	bool draw_f = FALSE;

	struct InsDataSub {
		float x, y;
		float r, g, b, a;
		float sizeX, sizeY;
	};
	struct InstanceData {
		int pcs = 0;
		InsDataSub s[INSTANCE_PCS_2D];
	};
	InstanceData textInsData[STRTEX_MAX_PCS];
	InstanceData valueInsData[VAL_PCS];

	static DxText *textobj;

	DxText(const DxText &obj);     // �R�s�[�R���X�g���N�^�֎~
	void operator=(const DxText& obj);// ������Z�q�֎~
	DxText();
	~DxText();
	int CreateText(PolygonData2D *p2, TCHAR *c, int texNo, float fontsize);
	TCHAR *CreateTextValue(int val);

public:
	static void InstanceCreate();
	static DxText *GetInstance();
	static void DeleteInstance();
	void UpDateText(TCHAR *c, float x, float y, float fontsize, VECTOR4 cl);
	void UpDateValue(int val, float x, float y, float fontsize, int pcs, VECTOR4 cl);
	void UpDate();
	void Draw(int com_no);
};

#endif