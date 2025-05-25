//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@	       SearchFile                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "SearchFile.h"
#include <locale.h>
#include <string.h>

SearchFile::SearchFile(UINT directorynum) {
	directoryNum = directorynum;
	filePath = new char**[directoryNum];
	fileNum = new UINT[directoryNum];
}

bool SearchFile::FileCopy(char *file, char **searchExtension, UINT searchExtensionNum, UINT *exNum) {
	UINT cnt = 0;
	while (cnt < MAX_PATH && file[++cnt] != '\0');//�I�[�����܂Ői�߂�
	if (cnt >= MAX_PATH)return false;//MAX_PATH�����ȏ�(�I�[��������)�̏ꍇ�X�L�b�v
	while (file[--cnt] != '.');//�I�[������T������'.'�܂Ői�߂�
	cnt++;//'.'����1�����i�߂Ċg���q1�����ڂɍ��킹��

	for (UINT i = 0; i < searchExtensionNum; i++) {
		if (!strcmp(&file[cnt], searchExtension[i]))(*exNum)++;
		//�w�肵���g���q�Ɣ�r(exNum��1�̏ꍇ��v)
	}

	return true;
}

void SearchFile::FileNumCount(wchar_t *Path, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	char file[MAX_PATH] = { '\0' };
	WIN32_FIND_DATA fd;
	HANDLE hFile = FindFirstFile(Path, &fd);//�T���ꏊ
	char fullPath[MAX_PATH] = { '\0' };
	setlocale(LC_CTYPE, "jpn");
	size_t fullsize = wcstombs(fullPath, Path, MAX_PATH);
	size_t PathLen = 0;
	for (size_t t = fullsize - 1; t > 0 && fullPath[t] != '/'; t--) {
		PathLen = t;
	}
	fullPath[PathLen] = '\0';

	//�t�@�C�����J�E���g
	do {
		//wcstombs:���C�h�������}���`�o�C�g�z��, �t�@�C�����R�s�[
		if (wcstombs(file, fd.cFileName, MAX_PATH) > 100 ||
			!strcmp(file, ".") || !strcmp(file, ".."))continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//�f�B���N�g���̏ꍇ
			wchar_t ws[MAX_PATH];
			//�}���`�o�C�g�z�񁨃��C�h����
			mbstowcs(ws, fullPath, MAX_PATH);
			//���C�h�����A��
			wcsncat(ws, fd.cFileName, MAX_PATH);
			wcsncat(ws, L"/*", 2);
			FileNumCount(ws, directoryInd, searchExtension, searchExtensionNum);
		}
		else {
			//�t�@�C���̏ꍇ
			UINT exNum = 0;
			if (!FileCopy(file, searchExtension, searchExtensionNum, &exNum))continue;
			if (exNum > 0)fileNum[directoryInd]++;
		}
	} while (FindNextFile(hFile, &fd));

	FindClose(hFile);
}

void SearchFile::FileLoad(wchar_t *Path, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	char file[MAX_PATH] = { '\0' };
	WIN32_FIND_DATA fd;
	HANDLE hFile = FindFirstFile(Path, &fd);//�T���ꏊ
	char fullPath[MAX_PATH] = { '\0' };
	setlocale(LC_CTYPE, "jpn");
	size_t fullsize = wcstombs(fullPath, Path, MAX_PATH);
	size_t PathLen = 0;
	for (size_t t = fullsize - 1; t > 0 && fullPath[t] != '/'; t--) {
		PathLen = t;
	}
	fullPath[PathLen] = '\0';

	//�t�@�C���ǂݍ���
	do {
		//wcstombs:���C�h�������}���`�o�C�g�z��, �t�@�C�����R�s�[
		if (wcstombs(file, fd.cFileName, MAX_PATH) > 100 ||
			!strcmp(file, ".") || !strcmp(file, ".."))continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			//�f�B���N�g���̏ꍇ
			wchar_t ws[MAX_PATH];
			//�}���`�o�C�g�z�񁨃��C�h����
			mbstowcs(ws, fullPath, MAX_PATH);
			//���C�h�����A��
			wcsncat(ws, fd.cFileName, MAX_PATH);
			wcsncat(ws, L"/*", 2);
			FileLoad(ws, directoryInd, searchExtension, searchExtensionNum);
		}
		else {
			UINT exNum = 0;
			if (!FileCopy(file, searchExtension, searchExtensionNum, &exNum))continue;
			if (exNum > 0) {
				strcpy(filePath[directoryInd][fileNum[directoryInd]], fullPath);
				strcat(filePath[directoryInd][fileNum[directoryInd]++], file);
			}
		}
	} while (FindNextFile(hFile, &fd));

	FindClose(hFile);
}

void SearchFile::Search(wchar_t *Path, UINT directoryInd, char **searchExtension, UINT searchExtensionNum) {
	fileNum[directoryInd] = 0;
	FileNumCount(Path, directoryInd, searchExtension, searchExtensionNum);
	filePath[directoryInd] = new char*[fileNum[directoryInd]];
	for (UINT i = 0; i < fileNum[directoryInd]; i++)filePath[directoryInd][i] = new char[101];
	fileNum[directoryInd] = 0;
	FileLoad(Path, directoryInd, searchExtension, searchExtensionNum);
}

SearchFile::~SearchFile() {
	for (UINT k = 0; k < directoryNum; k++) {
		for (UINT i = 0; i < fileNum[k]; i++) {
			if (filePath[k][i] == nullptr)continue;
			delete[] filePath[k][i];
			filePath[k][i] = nullptr;
		}
		if (filePath[k] == nullptr)continue;
		delete[] filePath[k];
		filePath[k] = nullptr;
	}
	if (filePath == nullptr)return;
	delete[] filePath;
	filePath = nullptr;
	delete[] fileNum;
	fileNum = nullptr;
}

UINT SearchFile::GetFileNum(UINT directoryInd) {
	return fileNum[directoryInd];
}

char *SearchFile::GetFileName(UINT directoryInd, UINT fileNum) {
	return filePath[directoryInd][fileNum];
}