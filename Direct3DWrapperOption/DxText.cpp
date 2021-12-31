//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	      DxText�N���X (unicode�Ή�,�}���`�o�C�g���Ɓ~)      **//
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
	memset(pBits, 0, Twidth * 4 * Theight);//0����
	UINT temp = (UINT)(Twidth * 4 / Tcount / 4);
	UINT s_rowPitch = temp * 4;
	for (int cnt = 0; cnt < Tcount; cnt++) {

		UINT offset1 = s_rowPitch * cnt;//4�̔{���ɂȂ��Ă��鎖
		// �t�H���g���̏�������
		// iOfs_x, iOfs_y : �����o���ʒu(����)
		// iBmp_w, iBmp_h : �t�H���g�r�b�g�}�b�v�̕���
		// Level : ���l�̒i�K (GGO_GRAY4_BITMAP�Ȃ̂�17�i�K)
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

DxText::DxText(const DxText &obj) {}      // �R�s�[�R���X�g���N�^�֎~
void DxText::operator=(const DxText& obj) {} // ������Z�q�֎~

DxText::DxText() {

	dx = Dx12Process::GetInstance();

	dx->Bigin(0);
	//������p�o�b�t�@������
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

	//�ϗp
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
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN, (L"�l�r �S�V�b�N") };
	hFont = CreateFontIndirect(&lf);

	//�f�o�C�X�R���e�L�X�g�擾�BGetGlyphOutline�֐����G���[�ɂȂ��
	hdc = GetDC(nullptr);
	oldFont = (HFONT)SelectObject(hdc, hFont);
	int count = 0;//�������J�E���g
	UINT code = 0;//�����R�[�h
	do {
#if _UNICODE
		// unicode�̏ꍇ�A�����R�[�h�͒P���Ƀ��C�h������UINT�ϊ��ł�
		code = (UINT)c[count++];
#else
		// �}���`�o�C�g�����̏ꍇ�A
		// 1�o�C�g�����̃R�[�h��1�o�C�g�ڂ�UINT�ϊ��A
		// 2�o�C�g�����̃R�[�h��[�擱�R�[�h]*256 + [�����R�[�h]
		if (IsDBCSLeadByte(*c))//�w�肳�ꂽ��������s�o�C�g���ǂ����𒲂ׂ�
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
	allsize = new DWORD[count]();//�e�v�f�܂ł̍��v�T�C�Y

	for (int cnt = 0; cnt < count; cnt++) {
		code = 0;
#if _UNICODE
		// unicode�̏ꍇ�A�����R�[�h�͒P���Ƀ��C�h������UINT�ϊ��ł�
		code = (UINT)c[cnt];
#else
		// �}���`�o�C�g�����̏ꍇ�A
		// 1�o�C�g�����̃R�[�h��1�o�C�g�ڂ�UINT�ϊ��A
		// 2�o�C�g�����̃R�[�h��[�擱�R�[�h]*256 + [�����R�[�h]
		if (IsDBCSLeadByte(*c))//�w�肳�ꂽ��������s�o�C�g���ǂ����𒲂ׂ�
			code = (BYTE)c[0] << 8 | (BYTE)c[1];
		else
			code = c[0];
#endif
		// �t�H���g�r�b�g�}�b�v�擾
		GetTextMetrics(hdc, &TM[cnt]);
		DWORD size = GetGlyphOutline(hdc, code, GGO_GRAY4_BITMAP, &GM[cnt], 0, nullptr, &Mat);

		//�e�v�f�܂ł̍��v�T�C�Y�i�[
		if (cnt == 0)allsize[0] = size; else {
			allsize[cnt] = allsize[cnt - 1] + size;
		}
	}
	ptr = new BYTE[allsize[count - 1]]();

	for (int cnt = 0; cnt < count; cnt++) {
		code = 0;
#if _UNICODE
		// unicode�̏ꍇ�A�����R�[�h�͒P���Ƀ��C�h������UINT�ϊ��ł�
		code = (UINT)c[cnt];
#else
		// �}���`�o�C�g�����̏ꍇ�A
		// 1�o�C�g�����̃R�[�h��1�o�C�g�ڂ�UINT�ϊ��A
		// 2�o�C�g�����̃R�[�h��[�擱�R�[�h]*256 + [�����R�[�h]
		if (IsDBCSLeadByte(*c))//�w�肳�ꂽ��������s�o�C�g���ǂ����𒲂ׂ�
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

	// �f�o�C�X�R���e�L�X�g�ƃt�H���g�n���h���̊J��
	SelectObject(hdc, oldFont);
	DeleteObject(hFont);
	ReleaseDC(nullptr, hdc);

	return count;
}

TCHAR* DxText::CreateTextValue(int val) {

	int cnt = 0;
	int va = val;
	static TCHAR c[STR_MAX_LENGTH];
	_tcscpy_s(c, L"");//������

	//�����J�E���g
	for (int i = 1;; i *= 10) {
		if (val / i == 0)break;
		cnt++;
	}
	if (cnt == 0)_tcscpy_s(c, L"�O");

	for (int i = cnt - 1; i >= 0; i--) {

		int s = va / (int)pow(10.0, i);
		switch (s) {
		case 0:
			_tcscat(c, L"�O");
			break;
		case 1:
			_tcscat(c, L"�P");
			break;
		case 2:
			_tcscat(c, L"�Q");
			break;
		case 3:
			_tcscat(c, L"�R");
			break;
		case 4:
			_tcscat(c, L"�S");
			break;
		case 5:
			_tcscat(c, L"�T");
			break;
		case 6:
			_tcscat(c, L"�U");
			break;
		case 7:
			_tcscat(c, L"�V");
			break;
		case 8:
			_tcscat(c, L"�W");
			break;
		case 9:
			_tcscat(c, L"�X");
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

	//�����J�E���g
	for (int i = 1;; i *= 10) {
		if (val / i == 0)break;
		cnt++;
	}
	if (cnt == 0)strcpy(c, "�O");

	int cInd = 0;
	if (sig)strcpy(&c[cInd++], "�|");
	for (int i = cnt - 1; i >= 0; i--) {

		int s = va / (int)pow(10.0, i);
		switch (s) {
		case 0:
			strcpy(&c[cInd], "�O");
			break;
		case 1:
			strcpy(&c[cInd], "�P");
			break;
		case 2:
			strcpy(&c[cInd], "�Q");
			break;
		case 3:
			strcpy(&c[cInd], "�R");
			break;
		case 4:
			strcpy(&c[cInd], "�S");
			break;
		case 5:
			strcpy(&c[cInd], "�T");
			break;
		case 6:
			strcpy(&c[cInd], "�U");
			break;
		case 7:
			strcpy(&c[cInd], "�V");
			break;
		case 8:
			strcpy(&c[cInd], "�W");
			break;
		case 9:
			strcpy(&c[cInd], "�X");
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

	//���P�[���w��
	setlocale(LC_ALL, "japanese");
	//char��TCHAR(wchar)�ϊ�
	mbstowcs(tc, c, sizeof(c));

	return tc;
}

void DxText::UpDateText(bool ChangeImmediately, TCHAR* c, float x, float y, float fontsize,
	CoordTf::VECTOR4 cl) {

	int texNo = -1;

	//�o�^�ς݃e�L�X�g����
	for (int i = 0; i < STRTEX_MAX_PCS; i++) {
		if (_tcscmp(c, str[i]) == 0 && f_size[i] == fontsize) {
			texNo = i;
			break;
		}
	}

	//�o�^����ĂȂ��e�L�X�g�̏ꍇ�V�K�o�^
	if (texNo == -1) {
		//�󂫂��g�p���łȂ�����update�O�z��ň�ԎႢ�z��ɓo�^����
		for (int i = 0; i < STRTEX_MAX_PCS; i++) {
			if (!Using[i] && textInsData[i].pcs <= 0) {
				texNo = i;
				break;
			}
		}
		//�󂫖����̏ꍇ,�z��0�ɓo�^
		if (texNo == -1)texNo = 0;
		//�e�L�X�g�o�^
		strcnt[texNo] = CreateText(text, c, texNo, fontsize);
		_tcscpy_s(str[texNo], c);//�e�L�X�g�o�^
		f_size[texNo] = fontsize;//�t�H���g�T�C�Y�o�^
		Using[texNo] = ChangeImmediately;//�p���g�p���邩�ǂ����o�^
	}

	//��ŕ`�悷��ׂɕK�v�ȃf�[�^��OBJ���ɕێ����Ă���
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

	//�����J�E���g
	int cnt = 0;
	for (int i = 1;; i *= 10) {
		if (val / i == 0)break;
		cnt++;
	}
	if (val == 0)cnt = 1;

	float xx = x + (pcs - cnt) * fontsize;//�`��J�n�ʒu(x����)
	for (int i = cnt - 1; i >= 0; i--) {
		int s = val / (int)pow(10.0, i);

		//��ŕ`�悷��ׂɕK�v�ȃf�[�^��OBJ���ɕێ����Ă���
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
	//�`��I��������`������Z�b�g
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

