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
	VIDEOINFOHEADER* pVideoInfoHeader = NULL;//構造体,ビデオ イメージのビットマップと色情報
	AM_MEDIA_TYPE am_media_type;     //メディア サンプルの メジャー タイプを指定するグローバル一意識別子 (GUID)

	long nBufferSize = 0;//バッファサイズ
	BYTE* pBuffer = NULL;  //ピクセルデータバッファ
	int linesize = 0;   //1ラインサイズ
	int xs, ys;    //画像サイズ
	int wid, hei; //格納時画像サイズ 
	BYTE* pix = nullptr;
	UINT** m_pix = nullptr;

	UINT** getframe(int width, int height);
	BYTE* getframe1(int width, int height);

public:
	Movie() {}
	Movie(char* fileName);//デコード後のファイルネーム
	BYTE* GetFrame(int width, int height, BYTE Threshold,
		BYTE addR = 0, BYTE addG = 0, BYTE addB = 0, BYTE addA = 0);
	void sound(long volume);
	virtual ~Movie();
};

#endif
