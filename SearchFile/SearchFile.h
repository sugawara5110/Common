//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@	       SearchFile                                        **//
//**                                                                                     **//
//*****************************************************************************************//
#ifndef Class_SearchFile_Header
#define Class_SearchFile_Header

#include <Windows.h>

class SearchFile {

private:
	HANDLE hFile;
	WIN32_FIND_DATA fd;
	char ***fileName = nullptr;//directoryNum, fileNum, strNum
	UINT directoryNum = 0;
	UINT *fileNum = nullptr;

	SearchFile() {}
	bool StepStr(char *str, char **searchExtension, UINT searchExtensionNum, UINT *exNum);

public:
	SearchFile(UINT directoryNum);
	~SearchFile();
	void Search(wchar_t *pass, UINT directoryInd, char **searchExtension, UINT searchExtensionNum);
	UINT GetFileNum(UINT directoryInd);
	char *GetFileName(UINT directoryInd, UINT fileNum);
};

#endif
