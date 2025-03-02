//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           Dx_Util                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Util_Header
#define Class_Dx_Util_Header

#include "DxStruct.h"
#include <iostream>
#include <fstream>
#include <memory>

namespace Dx_Util {

	void ErrorMessage(char* E_mes);

	bool getErrorState();//事前にErrorMessageが一回でも呼ばれればtrue

	char* GetNameFromPath(char* path);//パスからファイル名を抽出

	void createTangent(int numMaterial, unsigned int* indexCntArr,
		void* vertexArr, unsigned int** indexArr, int structByteStride,
		int posBytePos, int norBytePos, int texBytePos, int tangentBytePos);

	LPCWSTR charToLPCWSTR(char* str);

	char* strcat2(char* s1, char* s2);

	LPCWSTR charToLPCWSTR(char* s1, char* s2);

	void memory_leak_test();

	std::unique_ptr<char[]> ConvertFileToChar(char* file_path);

	CoordTf::MATRIX calculationMatrixWorld(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 theta, CoordTf::VECTOR3 scale);
}

#endif
