//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@           Dx_Util                                          **//
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

char* Dx_Util::GetNameFromPath(char* path) {

	if (strlen(path) > 255) {
		ErrorMessage("GetNameFromPass  ��������255�𒴂��Ă܂�");
		return nullptr;
	}
	CHAR temp[256];
	strcpy(temp, path);

	bool f = false;

	for (int i = 0; temp[i] != '\0' && i < 256; i++) {
		if (temp[i] == '\\' || temp[i] == '/') { f = true; break; }
	}

	if (f) {
		//�t�@�C�����݂̂ł͖����ꍇ�̏���
		while (*path != '\0') path++;//�I�[�����܂Ń|�C���^��i�߂�
		while (*path != '\\' && *path != '/')path--;//�t�@�C�����擪��'\'��'/'�܂Ń|�C���^��i�߂�
		path++;//'\'�܂���'/'�̎�(�t�@�C�����擪����)�܂Ń|�C���^��i�߂�
	}

	return path;//�|�C���^���삵�Ă�̂ŕԂ�l���g�p������
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
	//���P�[���w��
	setlocale(LC_ALL, "japanese");
	static TCHAR buf[256] = {};
	buf[255] = '\0';
	//char��TCHAR(wchar)�ϊ�
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

std::unique_ptr<char[]> Dx_Util::ConvertFileToChar(char* file_pass) {

	std::ifstream fin(file_pass);
	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	std::unique_ptr<char[]> ret = std::make_unique<char[]>(size);

	fin.read((char*)ret.get(), size);
	fin.close();

	return ret;
}

CoordTf::MATRIX Dx_Util::calculationMatrixWorld(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 theta, CoordTf::VECTOR3 scale) {
	using namespace CoordTf;
	MATRIX mov;
	MATRIX rotZ, rotY, rotX;
	MATRIX sca;
	MatrixScaling(&sca, scale.x, scale.y, scale.z);
	MatrixRotationZ(&rotZ, theta.z);
	MatrixRotationY(&rotY, theta.y);
	MatrixRotationX(&rotX, theta.x);
	MATRIX rotZYX = rotZ * rotY * rotX;
	MatrixTranslation(&mov, pos.x, pos.y, pos.z);
	return sca * rotZYX * mov;
}