//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           Dx_Util                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Util_Header
#define Class_Dx_Util_Header

#include "DxStruct.h"

namespace Dx_Util {

	void ErrorMessage(char* E_mes);

	bool getErrorState();

	char* GetNameFromPass(char* pass);//パスからファイル名を抽出

	void createTangent(int numMaterial, unsigned int* indexCntArr,
		void* vertexArr, unsigned int** indexArr, int structByteStride,
		int posBytePos, int norBytePos, int texBytePos, int tangentBytePos);

	CoordTf::VECTOR3 normalRecalculation(CoordTf::VECTOR3 Nor[3]);

	LPCWSTR charToLPCWSTR(char* str);

	char* strcat2(char* s1, char* s2);

	LPCWSTR charToLPCWSTR(char* s1, char* s2);
}

#endif
