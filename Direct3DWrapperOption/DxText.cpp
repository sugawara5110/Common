//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	      DxTextクラス (unicode対応,マルチバイトだと×)      **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxText.h"
#include <string.h>
#include <tchar.h>
#include <stdarg.h>
#include <locale.h>

void TextObj::SetTextParameter(int width, int height, int textCount,
	TEXTMETRIC** TM, GLYPHMETRICS** GM, BYTE** ptr, DWORD** allsize) {

	Twidth = width;
	Theight = height;
	Tcount = textCount;
	Tm = new TEXTMETRIC[Tcount]();
	memcpy(Tm, *TM, sizeof(TEXTMETRIC) * Tcount);
	Gm = new GLYPHMETRICS[Tcount]();
	memcpy(Gm, *GM, sizeof(GLYPHMETRICS) * Tcount);
	Allsize = new DWORD[Tcount]();
	memcpy(Allsize, *allsize, sizeof(DWORD) * Tcount);
	Ptr = new BYTE[Allsize[Tcount - 1]]();
	memcpy(Ptr, *ptr, sizeof(BYTE) * Allsize[Tcount - 1]);
	CreateTextOn = true;
}

void TextObj::SetText(int com) {

	if (!CreateTextOn)return;

	texture[0].Reset();
	textureUp[0].Reset();

	UCHAR* pBits = new UCHAR[Twidth * 4 * Theight];
	memset(pBits, 0, Twidth * 4 * Theight);//0埋め
	UINT temp = (UINT)(Twidth * 4 / Tcount / 4);
	UINT s_rowPitch = temp * 4;
	for (int cnt = 0; cnt < Tcount; cnt++) {

		UINT offset1 = s_rowPitch * cnt;//4の倍数になっている事
		// フォント情報の書き込み
		// iOfs_x, iOfs_y : 書き出し位置(左上)
		// iBmp_w, iBmp_h : フォントビットマップの幅高
		// Level : α値の段階 (GGO_GRAY4_BITMAPなので17段階)
		int iOfs_x = Gm[cnt].gmptGlyphOrigin.x;
		int iOfs_y = Tm[cnt].tmAscent - Gm[cnt].gmptGlyphOrigin.y;
		int iBmp_w = Gm[cnt].gmBlackBoxX + (4 - (Gm[cnt].gmBlackBoxX % 4)) % 4;
		int iBmp_h = Gm[cnt].gmBlackBoxY;
		int Level = 17;
		int x, y;
		DWORD Alpha, Color;

		for (y = iOfs_y; y < iOfs_y + iBmp_h; y++) {
			for (x = iOfs_x; x < iOfs_x + iBmp_w; x++) {
				int offset2;
				if (cnt == 0)offset2 = 0; else {
					offset2 = Allsize[cnt - 1];
				}
				Alpha = (255 * Ptr[(x - iOfs_x + iBmp_w * (y - iOfs_y)) + offset2]) / (Level - 1);
				Color = 0x00ffffff | (Alpha << 24);
				memcpy(&pBits[Twidth * 4 * y + 4 * x + offset1], &Color, sizeof(DWORD));
			}
		}
	}

	dx->createTexture(com, pBits, DXGI_FORMAT_B8G8R8A8_UNORM,
		textureUp[0].ReleaseAndGetAddressOf(), texture[0].ReleaseAndGetAddressOf(),
		Twidth, Twidth * 4, Theight);

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor = mDescHeap->GetCPUDescriptorHandleForHeapStart();
	Dx_Device::GetInstance()->CreateSrvTexture(hDescriptor, texture[0].GetAddressOf(), 1);

	ARR_DELETE(pBits);
	ARR_DELETE(Tm);
	ARR_DELETE(Gm);
	ARR_DELETE(Ptr);
	ARR_DELETE(Allsize);

	CreateTextOn = false;
}

DxText *DxText::textobj = nullptr;

void DxText::InstanceCreate() {
	if (textobj == nullptr)textobj = new DxText();
}

DxText *DxText::GetInstance(){

	if (textobj != nullptr)return textobj;
	return nullptr;
}

void DxText::DeleteInstance(){

	if (textobj != nullptr){
		delete textobj;
		textobj = nullptr;
	}
}

DxText::DxText(const DxText &obj) {}      // コピーコンストラクタ禁止
void DxText::operator=(const DxText& obj) {} // 代入演算子禁止

DxText::DxText() {

	dx = Dx12Process::GetInstance();

	dx->Bigin(0);
	//文字列用バッファ初期化
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		Using[i] = false;
		text[i].SetName("DxText_text");
		text[i].SetCommandList(0);
		text[i].GetVBarray2D(1);
		text[i].TexOn();
		text[i].CreateBox(0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, true, true);
		_tcscpy_s(str[i], sizeof(TCHAR), _T(""));
		f_size[i] = 0;
	}

	//可変用
	for (int i = 0; i < VAL_PCS; i++) {
		value[i].SetName("DxText_value");
		value[i].SetCommandList(0);
		value[i].GetVBarray2D(1);
		value[i].TexOn();
		value[i].CreateBox(0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, true, true);
		TCHAR* va = CreateTextValue(i);
		CreateText(value, va, i, 15.0f);
		value[i].SetText(0);
	}
	dx->End(0);
	dx->RunGpu();
	dx->WaitFence();
}

DxText::~DxText(){

}

int DxText::CreateText(TextObj* p2, TCHAR* c, int texNo, float fontsize) {

	MAT2 Mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
	int fsize = 100;
	LOGFONT lf = { fsize, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, OUT_TT_ONLY_PRECIS,
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN, (L"ＭＳ ゴシック") };
	hFont = CreateFontIndirect(&lf);

	//デバイスコンテキスト取得。GetGlyphOutline関数がエラーになる為
	hdc = GetDC(nullptr);
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

	ptr = nullptr;
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
		DWORD size = GetGlyphOutline(hdc, code, GGO_GRAY4_BITMAP, &GM[cnt], 0, nullptr, &Mat);

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
		BYTE* p;
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

	int w = 0;
	for (int cnt = 0; cnt < count; cnt++) {
		w += GM[cnt].gmCellIncX;
	}

	p2[texNo].SetTextParameter(w, TM[0].tmHeight, count, &TM, &GM, &ptr, &allsize);

	ARR_DELETE(TM);
	ARR_DELETE(GM);
	ARR_DELETE(allsize);
	ARR_DELETE(ptr);

	// デバイスコンテキストとフォントハンドルの開放
	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
	ReleaseDC(nullptr, hdc);

	return count;
}

TCHAR* DxText::CreateTextValue(int val) {

	int cnt = 0;
	int va = val;
	static TCHAR c[STR_MAX_LENGTH];
	_tcscpy_s(c, L"");//初期化

	//桁数カウント
	for (int i = 1;; i *= 10) {
		if (val / i == 0)break;
		cnt++;
	}
	if (cnt == 0)_tcscpy_s(c, L"０");

	for (int i = cnt - 1; i >= 0; i--) {

		int s = va / (int)pow(10.0, i);
		switch (s) {
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

char* DxText::CreateTextValueCh(int val) {

	int cnt = 0;
	int va = val;
	bool sig = false;
	if (va < 0)sig = true;
	static char c[STR_MAX_LENGTH];

	//桁数カウント
	for (int i = 1;; i *= 10) {
		if (val / i == 0)break;
		cnt++;
	}
	if (cnt == 0)strcpy(c, "０");

	int cInd = 0;
	if (sig)strcpy(&c[cInd++], "－");
	for (int i = cnt - 1; i >= 0; i--) {

		int s = va / (int)pow(10.0, i);
		switch (s) {
		case 0:
			strcpy(&c[cInd], "０");
			break;
		case 1:
			strcpy(&c[cInd], "１");
			break;
		case 2:
			strcpy(&c[cInd], "２");
			break;
		case 3:
			strcpy(&c[cInd], "３");
			break;
		case 4:
			strcpy(&c[cInd], "４");
			break;
		case 5:
			strcpy(&c[cInd], "５");
			break;
		case 6:
			strcpy(&c[cInd], "６");
			break;
		case 7:
			strcpy(&c[cInd], "７");
			break;
		case 8:
			strcpy(&c[cInd], "８");
			break;
		case 9:
			strcpy(&c[cInd], "９");
			break;
		}
		cInd += 2;
		va = va - (int)pow(10.0, i) * s;
	}
	return c;
}

char* DxText::CreateTextValueCh(double val, int Numdig) {

	static char c[STR_MAX_LENGTH];
	int cInd = 0;

	int valInt = (int)val;
	char* va = CreateTextValueCh(valInt);
	int vasize = (int)strlen(va) * sizeof(char);
	memcpy(&c[cInd], va, vasize);
	cInd += vasize;

	double Decimal = val - (double)valInt;
	if (Decimal <= 0.0) {
		c[cInd] = '\0';
		return c;
	}
	c[cInd++] = '.';
	int DecimalInt = int(Decimal * pow(10, Numdig));
	char* vad = CreateTextValueCh(DecimalInt);
	int vadsize = (int)strlen(vad) * sizeof(char);
	memcpy(&c[cInd], vad, vadsize);
	cInd += vadsize;
	c[cInd] = '\0';

	return c;
}

TCHAR* DxText::getStr(char* str, ...) {

	va_list ap;
	va_start(ap, str);

	char c[STR_MAX_LENGTH];
	static TCHAR tc[STR_MAX_LENGTH];
	int cInd = 0;

	while (*str != '\0') {

		if (*str == '%') {
			str++;
			char* out2 = nullptr;

			if (*str == 'd') {
				int out = va_arg(ap, int);
				out2 = CreateTextValueCh(out);
			}
			else if (*str == 'f') {
				double out = va_arg(ap, double);
				out2 = CreateTextValueCh(out, 4);
			}
			if (!out2)continue;
			int size = (int)strlen(out2) * sizeof(char);
			memcpy(&c[cInd], out2, size);
			cInd += size;
		}
		else {
			c[cInd++] = *str;
		}
		str++;
	}
	c[cInd] = '\0';
	va_end(ap);

	//ロケール指定
	setlocale(LC_ALL, "japanese");
	//char→TCHAR(wchar)変換
	mbstowcs(tc, c, sizeof(c));

	return tc;
}

void DxText::UpDateText(bool ChangeImmediately, TCHAR* c, float x, float y, float fontsize,
	CoordTf::VECTOR4 cl) {

	int texNo = -1;

	//登録済みテキスト検索
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		if (_tcscmp(c, str[i]) == 0 && f_size[i] == fontsize) {
			texNo = i;
			break;
		}
	}

	//登録されてないテキストの場合新規登録
	if (texNo == -1) {
		//空きかつ使用中でないかつupdate前配列で一番若い配列に登録する
		for (int i = 0; i < STRTEX_MAX_PCS; i++) {
			if (!Using[i] && textInsData[i].pcs <= 0) {
				texNo = i;
				break;
			}
		}
		//空き無しの場合,配列0に登録
		if (texNo == -1)texNo = 0;
		//テキスト登録
		strcnt[texNo] = CreateText(text, c, texNo, fontsize);
		_tcscpy_s(str[texNo], c);//テキスト登録
		f_size[texNo] = fontsize;//フォントサイズ登録
		Using[texNo] = ChangeImmediately;//継続使用するかどうか登録
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
	draw_f = true;
}

void DxText::UpDateText(TCHAR* c, float x, float y, float fontsize,
	CoordTf::VECTOR4 cl) {

	UpDateText(true, c, x, y, fontsize, cl);
}

void DxText::UpDateText(bool ChangeImmediately, char* c, float x, float y, float fontsize, CoordTf::VECTOR4 cl) {
	TCHAR str[STR_MAX_LENGTH] = {};
	int len = (int)strlen(c);
	if (len > STR_MAX_LENGTH - 1)len = STR_MAX_LENGTH - 1;
	mbstowcs(str, c, sizeof(TCHAR) * len);
	str[len] = '\0';
	UpDateText(ChangeImmediately, str, x, y, fontsize, cl);
}

void DxText::UpDateText(char* c, float x, float y, float fontsize, CoordTf::VECTOR4 cl) {
	UpDateText(true, c, x, y, fontsize, cl);
}

void DxText::UpDateValue(int val, float x, float y, float fontsize, int pcs, CoordTf::VECTOR4 cl) {

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
	draw_f = true;
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

void DxText::Draw(int com) {
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		text[i].SetText(com);
		text[i].Draw(com);
	}
	for (int i = 0; i < VAL_PCS; i++) {
		value[i].Draw(com);
	}
}

