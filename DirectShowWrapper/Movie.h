//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           Movieクラス                                      **//
//**                                                                                     **//
//*****************************************************************************************//
#ifndef Class_Movie_Header
#define Class_Movie_Header

#include "DsProcess.h"

class Movie :public DsProcess {

protected:
	VIDEOINFOHEADER *pVideoInfoHeader = NULL;//構造体,ビデオ イメージのビットマップと色情報
	AM_MEDIA_TYPE am_media_type;     //メディア サンプルの メジャー タイプを指定するグローバル一意識別子 (GUID)

	long nBufferSize = 0;//バッファサイズ
	BYTE *pBuffer = NULL;  //ピクセルデータバッファ
	int linesize = 0;   //1ラインサイズ
	int xs, ys;    //画像サイズ
	int wid, hei; //格納時画像サイズ 
	UINT **m_pix = NULL; //受け渡し用ピクセルデータ(1要素1ピクセル)
	BYTE *pix1 = nullptr;

	UINT **getframe(int width, int height);
	BYTE *getframe1(int width, int height);

public:
	Movie() {}
	Movie(char *fileName);//デコード後のファイルネーム
	UINT **GetFrame(int width, int height);
	BYTE *GetFrame1(int width, int height);
	virtual ~Movie();
};

#endif
