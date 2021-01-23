//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	         DxTextクラス                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DxText_Header
#define Class_DxText_Header

#include "DX_3DCG\Dx12ProcessCore.h"
#define STRTEX_MAX_PCS 125
#define STR_MAX_LENGTH 125
#define VAL_PCS 10

class DxText;

class TextObj :public PolygonData2D {

private:
	friend DxText;
	int Twidth = 0;
	int Theight = 0;
	int Tcount = 0;
	TEXTMETRIC* Tm = nullptr;
	GLYPHMETRICS* Gm = nullptr;
	BYTE* Ptr = nullptr;
	DWORD* Allsize = nullptr;
	bool CreateTextOn = false;

	void SetTextParameter(int width, int height, int textCount,
		TEXTMETRIC** TM, GLYPHMETRICS** GM, BYTE** ptr, DWORD** allsize);

	void SetText(int com_no);
};

class DxText {

private:
	Dx12Process* dx;
	TextObj text[STRTEX_MAX_PCS];       //文字描画用
	TextObj value[VAL_PCS];              //可変数字用
	TCHAR str[STRTEX_MAX_PCS][STR_MAX_LENGTH]; //登録テキスト
	bool Using[STRTEX_MAX_PCS];
	float f_size[STRTEX_MAX_PCS];              //登録テキストのフォントサイズ
	int strcnt[STRTEX_MAX_PCS];       //登録テキスト文字数 
	HFONT hFont, oldFont;           //フォント
	HDC hdc;                       //デバイスコンテキスト
	BYTE* ptr;                    //ビットマップ配列
	TEXTMETRIC* TM;    //文字レイアウト
	GLYPHMETRICS* GM; //文字レイアウト
	DWORD* allsize;   //各要素までの合計サイズ
	bool draw_f = false;

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

	static DxText* textobj;

	DxText(const DxText& obj);     // コピーコンストラクタ禁止
	void operator=(const DxText& obj);// 代入演算子禁止
	DxText();
	~DxText();
	int CreateText(TextObj* p2, TCHAR* c, int texNo, float fontsize);
	TCHAR* CreateTextValue(int val);
	char* CreateTextValueCh(int val);
	char* CreateTextValueCh(double val, int Numdig);

public:
	static void InstanceCreate();
	static DxText* GetInstance();
	static void DeleteInstance();
	TCHAR* getStr(char* str, ...);
	void UpDateText(bool ChangeImmediately, TCHAR* c, float x, float y, float fontsize = 15.0f,
		CoordTf::VECTOR4 cl = { 1.0f, 1.0f, 1.0f, 1.0f });
	void UpDateText(TCHAR* c, float x, float y, float fontsize = 15.0f,
		CoordTf::VECTOR4 cl = { 1.0f, 1.0f, 1.0f, 1.0f });
	void UpDateText(bool ChangeImmediately, char* c, float x, float y, float fontsize = 15.0f,
		CoordTf::VECTOR4 cl = { 1.0f, 1.0f, 1.0f, 1.0f });
	void UpDateText(char* c, float x, float y, float fontsize = 15.0f,
		CoordTf::VECTOR4 cl = { 1.0f, 1.0f, 1.0f, 1.0f });
	void UpDateValue(int val, float x, float y, float fontsize, int pcs,
		CoordTf::VECTOR4 cl = { 1.0f, 1.0f, 1.0f, 1.0f });
	void UpDate();
	void Draw(int com_no);
};

#endif