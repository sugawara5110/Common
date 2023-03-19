//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	      TextureLoader                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_TextureLoader_Header
#define Class_TextureLoader_Header

#include "../Direct3DWrapper/DX_3DCG/Core/Dx_TextureHolder.h"

struct Texture {
	char* binary_ch = nullptr; //デコード後バイナリ
	int   binary_size = 0;  //バイナリサイズ
	char* texName = nullptr; //ファイル名
	int   width = 512;
	int   height = 512;
};

class TextureLoader {

private:
	TextureLoader() {}

public:
	static bool GetTexture(int numTexArr, Texture* tex, Dx_TextureHolder* dx);//デコード済みのバイナリからリソースの生成
	static bool GetTexture2(int numTexArr, Texture* tex, Dx_TextureHolder* dx);//テスト中
};

#endif

