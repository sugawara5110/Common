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
	filePass = new char**[directoryNum];
	fileNum = new UINT[directoryNum];
}

bool SearchFile::FileCopy(char *file, char **searchExtension, UINT searchExtensionNum, UINT *exNum) {
	UINT cnt = 0;
	while (cnt < MAX_PATH && file[++cnt] != '\0');//終端文字まで進める
	if (cnt >= MAX_PATH)return false;//MAX_PATH文字以上(終端文字無し)の場合スキップ
	while (file[--cnt] != '.');//終端側から探索して'.'まで進める
	cnt++;//'.'から1文字進めて拡張子1文字目に合わせる

	for (UINT i = 0; i < searchExtensionNum; i++) {
		if (!strcmp(&file[cnt], searchExtension[i]))(*exNum)++;
		//指定した拡張子と比較(exNumが1の場合一致)
	}

	return true;
}

void SearchFile::FileNumCount(wchar_t *pass, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	char file[MAX_PATH] = { '\0' };
	WIN32_FIND_DATA fd;
	HANDLE hFile = FindFirstFile(pass, &fd);//探索場所
	char fullpass[MAX_PATH] = { '\0' };
	setlocale(LC_CTYPE, "jpn");
	size_t fullsize = wcstombs(fullpass, pass, MAX_PATH);
	size_t passLen = 0;
	for (size_t t = fullsize - 1; t > 0 && fullpass[t] != '/'; t--) {
		passLen = t;
	}
	fullpass[passLen] = '\0';

	//ファイル数カウント
	do {
		//wcstombs:ワイド文字→マルチバイト配列, ファイル名コピー
		if (wcstombs(file, fd.cFileName, MAX_PATH) > 100 ||
			!strcmp(file, ".") || !strcmp(file, ".."))continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//ディレクトリの場合
			wchar_t ws[MAX_PATH];
			//マルチバイト配列→ワイド文字
			mbstowcs(ws, fullpass, MAX_PATH);
			//ワイド文字連結
			wcsncat(ws, fd.cFileName, MAX_PATH);
			wcsncat(ws, L"/*", 2);
			FileNumCount(ws, directoryInd, searchExtension, searchExtensionNum);
		}
		else {
			//ファイルの場合
			UINT exNum = 0;
			if (!FileCopy(file, searchExtension, searchExtensionNum, &exNum))continue;
			if (exNum > 0)fileNum[directoryInd]++;
		}
	} while (FindNextFile(hFile, &fd));

	FindClose(hFile);
}

void SearchFile::FileLoad(wchar_t *pass, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	char file[MAX_PATH] = { '\0' };
	WIN32_FIND_DATA fd;
	HANDLE hFile = FindFirstFile(pass, &fd);//探索場所
	char fullpass[MAX_PATH] = { '\0' };
	setlocale(LC_CTYPE, "jpn");
	size_t fullsize = wcstombs(fullpass, pass, MAX_PATH);
	size_t passLen = 0;
	for (size_t t = fullsize - 1; t > 0 && fullpass[t] != '/'; t--) {
		passLen = t;
	}
	fullpass[passLen] = '\0';

	//ファイル読み込み
	do {
		//wcstombs:ワイド文字→マルチバイト配列, ファイル名コピー
		if (wcstombs(file, fd.cFileName, MAX_PATH) > 100 ||
			!strcmp(file, ".") || !strcmp(file, ".."))continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//ディレクトリの場合
			wchar_t ws[MAX_PATH];
			//マルチバイト配列→ワイド文字
			mbstowcs(ws, fullpass, MAX_PATH);
			//ワイド文字連結
			wcsncat(ws, fd.cFileName, MAX_PATH);
			wcsncat(ws, L"/*", 2);
			FileLoad(ws, directoryInd, searchExtension, searchExtensionNum);
		}
		else {
			UINT exNum = 0;
			if (!FileCopy(file, searchExtension, searchExtensionNum, &exNum))continue;
			if (exNum > 0) {
				strcpy(filePass[directoryInd][fileNum[directoryInd]], fullpass);
				strcat(filePass[directoryInd][fileNum[directoryInd]++], file);
			}
		}
	} while (FindNextFile(hFile, &fd));

	FindClose(hFile);
}

void SearchFile::Search(wchar_t *pass, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	fileNum[directoryInd] = 0;
	FileNumCount(pass, directoryInd, searchExtension, searchExtensionNum);
	filePass[directoryInd] = new char*[fileNum[directoryInd]];
	for (UINT i = 0; i < fileNum[directoryInd]; i++)filePass[directoryInd][i] = new char[101];
	fileNum[directoryInd] = 0;
	FileLoad(pass, directoryInd, searchExtension, searchExtensionNum);
}

SearchFile::~SearchFile() {
	for (UINT k = 0; k < directoryNum; k++) {
		for (UINT i = 0; i < fileNum[k]; i++) {
			if (filePass[k][i] == nullptr)continue;
			delete[] filePass[k][i];
			filePass[k][i] = nullptr;
		}
		if (filePass[k] == nullptr)continue;
		delete[] filePass[k];
		filePass[k] = nullptr;
	}
	if (filePass == nullptr)return;
	delete[] filePass;
	filePass = nullptr;
	delete[] fileNum;
	fileNum = nullptr;
}

UINT SearchFile::GetFileNum(UINT directoryInd) {
	return fileNum[directoryInd];
}

char *SearchFile::GetFileName(UINT directoryInd, UINT fileNum) {
	return filePass[directoryInd][fileNum];
}