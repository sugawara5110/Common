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

	CoordTf::VECTOR3 CalcTangent(CoordTf::VECTOR3 v0, CoordTf::VECTOR3 v1, CoordTf::VECTOR3 v2,
		CoordTf::VECTOR2 uv0, CoordTf::VECTOR2 uv1, CoordTf::VECTOR2 uv2);

	void createTangent(int numMaterial, UINT* indexCntArr,
		void* vertexArr, UINT** indexArr, int structByteStride,
		int posBytePos, int texBytePos, int tangentBytePos);

	CoordTf::VECTOR3 normalRecalculation(CoordTf::VECTOR3 Nor[3]);

	LPCWSTR charToLPCWSTR(char* str);

	char* strcat2(char* s1, char* s2);

	LPCWSTR charToLPCWSTR(char* s1, char* s2);
}

#endif
