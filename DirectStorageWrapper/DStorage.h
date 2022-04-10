//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         DStorage                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DStorage_Header
#define Class_DStorage_Header

#include <windows.h>
#include <dstorage.h>
#include <wrl.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class InputParameter {
private:
	char* fName = nullptr;

public:
	DXGI_FORMAT format = {};
	int width = 0;
	int height = 0;

	void setFileName(char* fileName) {
		int ln = (int)strlen(fileName) + 1;
		fName = new char[ln];
		memcpy(fName, fileName, sizeof(char) * ln);
	}

	char* getFileName() {
		return fName;
	}

	~InputParameter() {
		if (fName) {
			delete[] fName;
			fName = nullptr;
		}
	}
};

struct OutputResource {
	UCHAR* Subresource = nullptr;
	ComPtr<ID3D12Resource> Texture = nullptr;
};

namespace DStorage {
	void Delete();
	std::unique_ptr<OutputResource[]> Load(
		ID3D12Device* device,
		InputParameter* in, int numArr,
		char* error = nullptr);
};

#endif
