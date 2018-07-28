//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	       SearchFile                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "SearchFile.h"
#include <locale.h>
#include <string.h>

SearchFile::SearchFile(UINT directorynum) {
	directoryNum = directorynum;
	fileName = new char**[directoryNum];
	fileNum = new UINT[directoryNum];
}

bool SearchFile::StepStr(char *str, char **searchExtension, UINT searchExtensionNum, UINT *exNum) {
	UINT cnt = 0;
	if (wcstombs(str, fd.cFileName, 255) > 100 ||
		!strcmp(str, ".") || !strcmp(str, ".."))return false;
	while (cnt < 255 && str[++cnt] != '\0');//終端文字まで進める
	if (cnt >= 255)return false;//255文字以上の場合スキップ
	while (str[--cnt] != '.');//終端側から探索して'.'まで進める
	cnt++;//'.'から1文字進めて拡張子1文字目に合わせる

	for (UINT i = 0; i < searchExtensionNum; i++) {
		if (!strcmp(&str[cnt], searchExtension[i]))(*exNum)++;//指定した拡張子と比較
	}

	return true;
}

void SearchFile::Search(wchar_t *pass, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	char str[255] = { '\0' };
	hFile = FindFirstFile(pass, &fd);//探索場所カレントデレクトリ
	char fullpass[255] = { '\0' };
	size_t fullsize = wcstombs(fullpass, pass, 255);
	size_t passLen = 0;
	for (size_t t = fullsize - 1; t > 0 && fullpass[t] != '/'; t--) {
		passLen = t;
	}
	fullpass[passLen] = '\0';
	setlocale(LC_CTYPE, "jpn");

	//ファイル数カウント
	fileNum[directoryInd] = 0;
	do {
		UINT exNum = 0;
		if (!StepStr(str, searchExtension, searchExtensionNum, &exNum))continue;
		if (exNum > 0)fileNum[directoryInd]++;
	} while (FindNextFile(hFile, &fd));
	FindClose(hFile);

	fileName[directoryInd] = new char*[fileNum[directoryInd]];
	for (UINT i = 0; i < fileNum[directoryInd]; i++)fileName[directoryInd][i] = new char[101];

	hFile = FindFirstFile(pass, &fd);

	//ファイル読み込み
	fileNum[directoryInd] = 0;
	do {
		UINT exNum = 0;
		if (!StepStr(str, searchExtension, searchExtensionNum, &exNum))continue;
		if (exNum > 0) {
			strcpy(fileName[directoryInd][fileNum[directoryInd]], fullpass);
			strcat(fileName[directoryInd][fileNum[directoryInd]++], str);
		}
	} while (FindNextFile(hFile, &fd));
	FindClose(hFile);
}

SearchFile::~SearchFile() {
	for (UINT k = 0; k < directoryNum; k++) {
		for (UINT i = 0; i < fileNum[k]; i++) {
			if (fileName[k][i] == nullptr)continue;
			delete[] fileName[k][i];
			fileName[k][i] = nullptr;
		}
		if (fileName[k] == nullptr)continue;
		delete[] fileName[k];
		fileName[k] = nullptr;
	}
	if (fileName == nullptr)return;
	delete[] fileName;
	fileName = nullptr;
	delete[] fileNum;
	fileNum = nullptr;
}

UINT SearchFile::GetFileNum(UINT directoryInd) {
	return fileNum[directoryInd];
}

char *SearchFile::GetFileName(UINT directoryInd, UINT fileNum) {
	return fileName[directoryInd][fileNum];
}