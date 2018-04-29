//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	      DxTextクラス (unicode対応,マルチバイトだと×)      **//
//**                                  DrawText関数                                       **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxText.h"
#include <string.h>
#include <tchar.h>

DxText *DxText::textobj = NULL;

void DxText::InstanceCreate() {
	if (textobj == NULL)textobj = new DxText();
}

DxText *DxText::GetInstance(){

	if (textobj != NULL)return textobj;
	return NULL;
}

void DxText::DeleteInstance(){

	if (textobj != NULL){
		delete textobj;
		textobj = NULL;
	}
}

DxText::DxText(const DxText &obj) {}      // コピーコンストラクタ禁止
void DxText::operator=(const DxText& obj) {} // 代入演算子禁止

DxText::DxText() {

	dx = Dx12Process::GetInstance();

	dx->Bigin(0);
	//文字列用バッファ初期化
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		text[i].SetCommandList(0);
		text[i].GetVBarray2D(1);
		text[i].TexOn();
		text[i].CreateBox(0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, TRUE, TRUE);
		_tcscpy_s(str[i], STR_MAX_LENGTH * sizeof(TCHAR), _T("***************************************"));//直接代入
		f_size[i] = 0;
	}

	//可変用
	for (int i = 0; i < VAL_PCS; i++) {
		value[i].SetCommandList(0);
		value[i].GetVBarray2D(1);
		value[i].TexOn();
		value[i].CreateBox(0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, TRUE, TRUE);
		TCHAR *va = CreateTextValue(i);
		CreateText(value, va, i, 15.0f);
		value[i].SetText();
	}
	dx->End(0);
	dx->WaitFenceCurrent();
	CreateTextNo = 0;
}

DxText::~DxText(){

}

int DxText::CreateText(PolygonData2D *p2, TCHAR *c, int texNo, float fontsize) {

	MAT2 Mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
	int fsize = 100;
	LOGFONT lf = { fsize, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS,
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN, (L"ＭＳ ゴシック") };
	hFont = CreateFontIndirect(&lf);

	//デバイスコンテキスト取得。GetGlyphOutline関数がエラーになる為
	hdc = GetDC(NULL);
	oldFont = (HFONT)SelectObject(hdc, hFont);
	int count = 0;//文字数カウント
	UINT code = 0;//文字コード
	do {
#if _UNICODE
		// unicodeの場合、文字コードは単純にワイド文字のUINT変換です
		code = (UINT)c[count++];
#else
		// マルチバイト文字の場合、
		// 1バイト文字のコードは1バイト目のUINT変換、
		// 2バイト文字のコードは[先導コード]*256 + [文字コード]
		if (IsDBCSLeadByte(*c))//指定された文字が先行バイトかどうかを調べる
			code = (BYTE)c[0] << 8 | (BYTE)c[1];
		else
			code = c[0];
#endif
	} while (code != 0);
	count--;
	if (count == 0)return 0;

	ptr = NULL;
	TM = new TEXTMETRIC[count]();
	GM = new GLYPHMETRICS[count]();
	allsize = new DWORD[count]();//各要素までの合計サイズ

	for (int cnt = 0; cnt < count; cnt++) {
		code = 0;
#if _UNICODE
		// unicodeの場合、文字コードは単純にワイド文字のUINT変換です
		code = (UINT)c[cnt];
#else
		// マルチバイト文字の場合、
		// 1バイト文字のコードは1バイト目のUINT変換、
		// 2バイト文字のコードは[先導コード]*256 + [文字コード]
		if (IsDBCSLeadByte(*c))//指定された文字が先行バイトかどうかを調べる
			code = (BYTE)c[0] << 8 | (BYTE)c[1];
		else
			code = c[0];
#endif
		// フォントビットマップ取得
		GetTextMetrics(hdc, &TM[cnt]);
		DWORD size = GetGlyphOutline(hdc, code, GGO_GRAY4_BITMAP, &GM[cnt], 0, NULL, &Mat);

		//各要素までの合計サイズ格納
		if (cnt == 0)allsize[0] = size; else {
			allsize[cnt] = allsize[cnt - 1] + size;
		}
	}
	ptr = new BYTE[allsize[count - 1]]();

	for (int cnt = 0; cnt < count; cnt++) {
		code = 0;
#if _UNICODE
		// unicodeの場合、文字コードは単純にワイド文字のUINT変換です
		code = (UINT)c[cnt];
#else
		// マルチバイト文字の場合、
		// 1バイト文字のコードは1バイト目のUINT変換、
		// 2バイト文字のコードは[先導コード]*256 + [文字コード]
		if (IsDBCSLeadByte(*c))//指定された文字が先行バイトかどうかを調べる
			code = (BYTE)c[0] << 8 | (BYTE)c[1];
		else
			code = c[0];
#endif
		DWORD size;
		BYTE *p;
		if (cnt == 0) {
			size = allsize[0];
			p = &ptr[0];
		}
		else {
			size = allsize[cnt] - allsize[cnt - 1];
			p = &ptr[allsize[cnt - 1]];
		}
		GetGlyphOutline(hdc, code, GGO_GRAY4_BITMAP, &GM[cnt], size, p, &Mat);
	}

	UINT w = 0;
	for (int cnt = 0; cnt < count; cnt++) {
		w += GM[cnt].gmCellIncX;
	}

	p2[texNo].SetTextParameter((int)(w * 1.3f), TM[0].tmHeight, count, &TM, &GM, &ptr, &allsize);//1.3は表示範囲幅補正

	delete TM;
	TM = NULL;
	delete GM;
	GM = NULL;
	delete allsize;
	allsize = NULL;
	delete[] ptr;
	ptr = NULL;

	// デバイスコンテキストとフォントハンドルの開放
	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
	ReleaseDC(NULL, hdc);

	return count;
}

TCHAR *DxText::CreateTextValue(int val){

	int cnt = 0;
	int va = val;
	static TCHAR c[STR_MAX_LENGTH];
	_tcscpy_s(c, L"");//初期化

	//桁数カウント
	for (int i = 1;; i *= 10){
		if (val / i == 0)break;
		cnt++;
	}
	if (cnt == 0)_tcscpy_s(c, L"０");

	for (int i = cnt - 1; i >= 0; i--){

		int s = va / (int)pow(10.0, i);
		switch (s){
		case 0:
			_tcscat(c, L"０");
			break;
		case 1:
			_tcscat(c, L"１");
			break;
		case 2:
			_tcscat(c, L"２");
			break;
		case 3:
			_tcscat(c, L"３");
			break;
		case 4:
			_tcscat(c, L"４");
			break;
		case 5:
			_tcscat(c, L"５");
			break;
		case 6:
			_tcscat(c, L"６");
			break;
		case 7:
			_tcscat(c, L"７");
			break;
		case 8:
			_tcscat(c, L"８");
			break;
		case 9:
			_tcscat(c, L"９");
			break;
		}
		va = va - (int)pow(10.0, i) * s;
	}
	return c;
}

void DxText::UpDateText(TCHAR *c, float x, float y, float fontsize, VECTOR4 cl) {
	bool match = FALSE;
	int texNo = -1;

	//登録済みテキスト検索
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		if (_tcscmp(c, str[i]) == 0 && f_size[i] == fontsize) {
			match = TRUE;
			texNo = i;
		}
	}

	if (texNo == -1) {
		if (CreateTextNo == STRTEX_MAX_PCS)CreateTextNo = 0;
		texNo = CreateTextNo++;
	}

	if (match == FALSE) {
		strcnt[texNo] = CreateText(text, c, texNo, fontsize);
		_tcscpy_s(str[texNo], c);//テキスト登録
		f_size[texNo] = fontsize;//フォントサイズ登録
	}

	//後で描画する為に必要なデータをOBJ毎に保持しておく
	textInsData[texNo].s[textInsData[texNo].pcs].x = x;
	textInsData[texNo].s[textInsData[texNo].pcs].y = y;
	textInsData[texNo].s[textInsData[texNo].pcs].r = cl.x;
	textInsData[texNo].s[textInsData[texNo].pcs].g = cl.y;
	textInsData[texNo].s[textInsData[texNo].pcs].b = cl.z;
	textInsData[texNo].s[textInsData[texNo].pcs].a = cl.w;
	textInsData[texNo].s[textInsData[texNo].pcs].sizeX = f_size[texNo] * strcnt[texNo];
	textInsData[texNo].s[textInsData[texNo].pcs].sizeY = f_size[texNo];
	textInsData[texNo].pcs++;
	draw_f = TRUE;
}

void DxText::UpDateValue(int val, float x, float y, float fontsize, int pcs, VECTOR4 cl) {

	//桁数カウント
	int cnt = 0;
	for (int i = 1;; i *= 10) {
		if (val / i == 0)break;
		cnt++;
	}
	if (val == 0)cnt = 1;

	float xx = x + (pcs - cnt) * fontsize;//描画開始位置(x方向)
	for (int i = cnt - 1; i >= 0; i--) {
		int s = val / (int)pow(10.0, i);

		//後で描画する為に必要なデータをOBJ毎に保持しておく
		valueInsData[s].s[valueInsData[s].pcs].x = xx;
		valueInsData[s].s[valueInsData[s].pcs].y = y;
		valueInsData[s].s[valueInsData[s].pcs].r = cl.x;
		valueInsData[s].s[valueInsData[s].pcs].g = cl.y;
		valueInsData[s].s[valueInsData[s].pcs].b = cl.z;
		valueInsData[s].s[valueInsData[s].pcs].a = cl.w;
		valueInsData[s].s[valueInsData[s].pcs].sizeX = fontsize;
		valueInsData[s].s[valueInsData[s].pcs].sizeY = fontsize;
		valueInsData[s].pcs++;

		val = val - (int)pow(10.0, i) * s;
		xx += fontsize;
	}
	draw_f = TRUE;
}

void DxText::UpDate() {

	if (!draw_f)return;

	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		if (textInsData[i].pcs == 0) { text[i].DrawOff(); continue; }
		int i1;
		for (i1 = 0; i1 < textInsData[i].pcs; i1++) {
			text[i].InstancedSetConstBf(
				textInsData[i].s[i1].x,
				textInsData[i].s[i1].y,
				textInsData[i].s[i1].r,
				textInsData[i].s[i1].g,
				textInsData[i].s[i1].b,
				textInsData[i].s[i1].a,
				textInsData[i].s[i1].sizeX,
				textInsData[i].s[i1].sizeY
			);
		}
		text[i].InstanceUpdate();
	}

	for (int i = 0; i < VAL_PCS; i++) {
		if (valueInsData[i].pcs == 0) { value[i].DrawOff(); continue; }
		int i1;
		for (i1 = 0; i1 < valueInsData[i].pcs; i1++) {
			value[i].InstancedSetConstBf(
				valueInsData[i].s[i1].x,
				valueInsData[i].s[i1].y,
				valueInsData[i].s[i1].r,
				valueInsData[i].s[i1].g,
				valueInsData[i].s[i1].b,
				valueInsData[i].s[i1].a,
				valueInsData[i].s[i1].sizeX,
				valueInsData[i].s[i1].sizeY
			);
		}
		value[i].InstanceUpdate();
	}
	//描画終了したら描画個数リセット
	for (int i = 0; i < STRTEX_MAX_PCS; i++)textInsData[i].pcs = 0;
	for (int i = 0; i < VAL_PCS; i++)valueInsData[i].pcs = 0;
}

void DxText::Draw(int com_no) {
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		text[i].SetCommandList(com_no);
		text[i].SetText();
		text[i].Draw();
	}
	for (int i = 0; i < VAL_PCS; i++) {
		value[i].SetCommandList(com_no);
		value[i].Draw();
	}
}

