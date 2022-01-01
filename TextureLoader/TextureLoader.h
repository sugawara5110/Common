//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	      TextureLoader                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_TextureLoader_Header
#define Class_TextureLoader_Header

#include "../Direct3DWrapper/DX_3DCG/Core/Dx12ProcessCore.h"

struct Texture {
	char* binary_ch = nullptr; //�f�R�[�h��o�C�i��
	int   binary_size = 0;  //�o�C�i���T�C�Y
	char* texName = nullptr; //�t�@�C����
	int   width = 512;
	int   height = 512;
};

class TextureLoader {

private:
	TextureLoader() {}

public:
	static bool GetTexture(int numTexArr, Texture* tex, Dx12Process* dx);//�f�R�[�h�ς݂̃o�C�i�����烊�\�[�X�̐���
	static bool GetTexture2(int numTexArr, Texture* tex, Dx12Process* dx);//�e�X�g��
};

#endif

