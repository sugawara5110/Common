//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           Dx_Util                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_Util.h"
#include <tchar.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

static bool Dx_UtilErrorF = false;

void Dx_Util::ErrorMessage(char* E_mes) {
	MessageBoxA(0, E_mes, 0, MB_OK);
	Dx_UtilErrorF = true;
}

bool Dx_Util::getErrorState() {
	return Dx_UtilErrorF;
}

char* Dx_Util::GetNameFromPass(char* pass) {

	if (strlen(pass) > 255) {
		ErrorMessage("GetNameFromPass  文字数が255を超えてます");
		return nullptr;
	}
	CHAR temp[256];
	strcpy(temp, pass);

	bool f = false;

	for (int i = 0; temp[i] != '\0' && i < 256; i++) {
		if (temp[i] == '\\' || temp[i] == '/') { f = true; break; }
	}

	if (f) {
		//ファイル名のみでは無い場合の処理
		while (*pass != '\0') pass++;//終端文字までポインタを進める
		while (*pass != '\\' && *pass != '/')pass--;//ファイル名先頭の'\'か'/'までポインタを進める
		pass++;//'\'または'/'の次(ファイル名先頭文字)までポインタを進める
	}

	return pass;//ポインタ操作してるので返り値を使用させる
}

void Dx_Util::createTangent(int numMaterial, unsigned int* indexCntArr,
	void* vertexArr, unsigned int** indexArr, int structByteStride,
	int posBytePos, int norBytePos, int texBytePos, int tangentBytePos) {

	using namespace CoordTf;

	unsigned char* b_posSt = (unsigned char*)vertexArr + posBytePos;
	unsigned char* b_norSt = (unsigned char*)vertexArr + norBytePos;
	unsigned char* b_texSt = (unsigned char*)vertexArr + texBytePos;
	unsigned char* b_tanSt = (unsigned char*)vertexArr + tangentBytePos;
	for (int i = 0; i < numMaterial; i++) {
		unsigned int cnt = 0;
		while (indexCntArr[i] > cnt) {
			VECTOR3* posVec[3] = {};
			VECTOR3* norVec[3] = {};
			VECTOR2* texVec[3] = {};
			VECTOR3* tanVec[3] = {};

			for (int ind = 0; ind < 3; ind++) {
				unsigned int index = indexArr[i][cnt++] * structByteStride;
				unsigned char* b_pos = b_posSt + index;
				unsigned char* b_nor = b_norSt + index;
				unsigned char* b_tex = b_texSt + index;
				unsigned char* b_tan = b_tanSt + index;
				posVec[ind] = (VECTOR3*)b_pos;
				norVec[ind] = (VECTOR3*)b_nor;
				texVec[ind] = (VECTOR2*)b_tex;
				tanVec[ind] = (VECTOR3*)b_tan;
			}
			VECTOR3 tangent = CalcTangent(*(posVec[0]), *(posVec[1]), *(posVec[2]),
				*(texVec[0]), *(texVec[1]), *(texVec[2]), *(norVec[0]));

			VECTOR3 outN = {};
			VectorNormalize(&outN, &tangent);

			*(tanVec[0]) = *(tanVec[1]) = *(tanVec[2]) = outN;
		}
	}
}

LPCWSTR Dx_Util::charToLPCWSTR(char* str) {
	//ロケール指定
	setlocale(LC_ALL, "japanese");
	static TCHAR buf[256] = {};
	buf[255] = '\0';
	//char→TCHAR(wchar)変換
	int size = (int)strlen(str) + 1;
	if (size > 255)size = 255;
	mbstowcs(buf, str, size);
	static LPCWSTR buf2;
	buf2 = buf;
	return buf2;
}

char* Dx_Util::strcat2(char* s1, char* s2) {
	static char str[256] = {};
	str[255] = '\0';
	if (strlen(s1) + strlen(s2) > 254)return "The number of characters exceeds 254.";
	memcpy(str, s1, sizeof(char) * strlen(s1));
	str[strlen(s1)] = '_';
	str[strlen(s1) + 1] = '\0';
	strcat(str, s2);
	return str;
}

LPCWSTR Dx_Util::charToLPCWSTR(char* s1, char* s2) {
	return charToLPCWSTR(strcat2(s1, s2));
}

void Dx_Util::memory_leak_test() {
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}