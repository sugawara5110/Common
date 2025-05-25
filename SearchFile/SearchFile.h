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
	char ***filePath = nullptr;//directoryNum, fileNum, fileï∂éöêî
	UINT directoryNum = 0;
	UINT *fileNum = nullptr;

	SearchFile() {}
	bool FileCopy(char *file, char **searchExtension, UINT searchExtensionNum, UINT *exNum);
	void FileNumCount(wchar_t *Path, UINT directoryInd, char **searchExtension, UINT searchExtensionNum);
	void FileLoad(wchar_t *Path, UINT directoryInd, char **searchExtension, UINT searchExtensionNum);

public:
	SearchFile(UINT directoryNum);
	~SearchFile();
	void Search(wchar_t *Path, UINT directoryInd, char **searchExtension, UINT searchExtensionNum);
	UINT GetFileNum(UINT directoryInd);
	char *GetFileName(UINT directoryInd, UINT fileNum);
};

#endif
