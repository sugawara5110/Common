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

void Dx_Util::ErrorMessage(char* E_mes) {
	MessageBoxA(0, E_mes, 0, MB_OK);
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

CoordTf::VECTOR3 Dx_Util::CalcTangent(CoordTf::VECTOR3 v0, CoordTf::VECTOR3 v1, CoordTf::VECTOR3 v2,
	CoordTf::VECTOR2 uv0, CoordTf::VECTOR2 uv1, CoordTf::VECTOR2 uv2) {

	CoordTf::VECTOR3 cp0[3] = {
		{v0.x, uv0.x, uv0.y},
		{v0.y, uv0.x, uv0.y},
		{v0.z, uv0.x, uv0.y}
	};
	CoordTf::VECTOR3 cp1[3] = {
		{v1.x, uv1.x, uv1.y},
		{v1.y, uv1.x, uv1.y},
		{v1.z, uv1.x, uv1.y}
	};
	CoordTf::VECTOR3 cp2[3] = {
		{v2.x, uv2.x, uv2.y},
		{v2.y, uv2.x, uv2.y},
		{v2.z, uv2.x, uv2.y}
	};

	CoordTf::VECTOR3 tangent = {};
	float tan[3] = {};

	for (int i = 0; i < 3; i++) {
		CoordTf::VECTOR3 v1 = {};
		v1.as(cp1[i].x - cp0[i].x,
			cp1[i].y - cp0[i].y,
			cp1[i].z - cp0[i].z);
		CoordTf::VECTOR3 v2 = {};
		v2.as(cp2[i].x - cp1[i].x,
			cp2[i].y - cp1[i].y,
			cp2[i].z - cp1[i].z);
		CoordTf::VECTOR3 t = {};
		CoordTf::VectorCross(&t, &v1, &v2);

		if (t.x == 0.0f) {
			tan[0] = 0.0f;
			tan[1] = 0.0f;
			tan[2] = 0.0f;
			break;
		}
		else {
			tan[i] = -t.y / t.x;
		}
	}

	tangent.x = tan[0];
	tangent.y = tan[1];
	tangent.z = tan[2];
	CoordTf::VECTOR3 ret = {};
	CoordTf::VectorNormalize(&ret, &tangent);
	return ret;
}

void Dx_Util::createTangent(int numMaterial, UINT* indexCntArr,
	void* vertexArr, UINT** indexArr, int structByteStride,
	int posBytePos, int texBytePos, int tangentBytePos) {

	BYTE* b_posSt = (BYTE*)vertexArr + posBytePos;
	BYTE* b_texSt = (BYTE*)vertexArr + texBytePos;
	BYTE* b_tanSt = (BYTE*)vertexArr + tangentBytePos;
	for (int i = 0; i < numMaterial; i++) {
		UINT cnt = 0;
		while (indexCntArr[i] > cnt) {
			CoordTf::VECTOR3* posVec[3] = {};
			CoordTf::VECTOR2* texVec[3] = {};
			CoordTf::VECTOR3* tanVec[3] = {};

			for (int ind = 0; ind < 3; ind++) {
				UINT index = indexArr[i][cnt++] * structByteStride;
				BYTE* b_pos = b_posSt + index;
				BYTE* b_tex = b_texSt + index;
				BYTE* b_tan = b_tanSt + index;
				posVec[ind] = (CoordTf::VECTOR3*)b_pos;
				texVec[ind] = (CoordTf::VECTOR2*)b_tex;
				tanVec[ind] = (CoordTf::VECTOR3*)b_tan;
			}
			CoordTf::VECTOR3 tangent = CalcTangent(*(posVec[0]), *(posVec[1]), *(posVec[2]),
				*(texVec[0]), *(texVec[1]), *(texVec[2]));
			*(tanVec[0]) = *(tanVec[1]) = *(tanVec[2]) = tangent;
		}
	}
}

CoordTf::VECTOR3 Dx_Util::normalRecalculation(CoordTf::VECTOR3 N[3]) {
	using namespace CoordTf;
	VECTOR3 vecX = {};
	VECTOR3 vecY = {};
	vecX.as(N[0].x - N[1].x, N[0].y - N[1].y, N[0].z - N[1].z);
	vecY.as(N[0].x - N[2].x, N[0].y - N[2].y, N[0].z - N[2].z);
	VECTOR3 vec = {};
	VectorCross(&vec, &vecX, &vecY);
	return vec;
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