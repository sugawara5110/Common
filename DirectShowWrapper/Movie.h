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
	VIDEOINFOHEADER* pVideoInfoHeader = NULL;//�\����,�r�f�I �C���[�W�̃r�b�g�}�b�v�ƐF���
	AM_MEDIA_TYPE am_media_type;     //���f�B�A �T���v���� ���W���[ �^�C�v���w�肷��O���[�o����ӎ��ʎq (GUID)

	long nBufferSize = 0;//�o�b�t�@�T�C�Y
	BYTE* pBuffer = NULL;  //�s�N�Z���f�[�^�o�b�t�@
	int linesize = 0;   //1���C���T�C�Y
	int xs, ys;    //�摜�T�C�Y
	int wid, hei; //�i�[���摜�T�C�Y 
	BYTE* pix = nullptr;
	UINT** m_pix = nullptr;

	UINT** getframe(int width, int height);
	BYTE* getframe1(int width, int height);

public:
	Movie() {}
	Movie(char* fileName);//�f�R�[�h��̃t�@�C���l�[��
	BYTE* GetFrame(int width, int height, BYTE Threshold,
		BYTE addR = 0, BYTE addG = 0, BYTE addB = 0, BYTE addA = 0);
	void sound(long volume);
	virtual ~Movie();
};

#endif
