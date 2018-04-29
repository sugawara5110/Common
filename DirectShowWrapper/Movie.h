//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@           Movie�N���X                                      **//
//**                                                                                     **//
//*****************************************************************************************//
#ifndef Class_Movie_Header
#define Class_Movie_Header

#include "DsProcess.h"

class Movie :public DsProcess {

protected:
	VIDEOINFOHEADER *pVideoInfoHeader = NULL;//�\����,�r�f�I �C���[�W�̃r�b�g�}�b�v�ƐF���
	AM_MEDIA_TYPE am_media_type;     //���f�B�A �T���v���� ���W���[ �^�C�v���w�肷��O���[�o����ӎ��ʎq (GUID)

	long nBufferSize = 0;//�o�b�t�@�T�C�Y
	BYTE *pBuffer = NULL;  //�s�N�Z���f�[�^�o�b�t�@
	int linesize = 0;   //1���C���T�C�Y
	int xs, ys;    //�摜�T�C�Y
	int wid, hei; //�i�[���摜�T�C�Y 
	UINT **m_pix = NULL; //�󂯓n���p�s�N�Z���f�[�^(1�v�f1�s�N�Z��)
	BYTE *pix1 = nullptr;

	UINT **getframe(int width, int height);
	BYTE *getframe1(int width, int height);

public:
	Movie() {}
	Movie(char *fileName);//�f�R�[�h��̃t�@�C���l�[��
	UINT **GetFrame(int width, int height);
	BYTE *GetFrame1(int width, int height);
	virtual ~Movie();
};

#endif
