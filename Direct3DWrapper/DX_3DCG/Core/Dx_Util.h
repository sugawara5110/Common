//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　           Dx_Util                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_Util_Header
#define Class_Dx_Util_Header

#include "Dx_ShaderHolder.h"

namespace Dx_Util {

	char* GetNameFromPass(char* pass);//パスからファイル名を抽出

	CoordTf::VECTOR3 CalcTangent(CoordTf::VECTOR3 v0, CoordTf::VECTOR3 v1, CoordTf::VECTOR3 v2,
		CoordTf::VECTOR2 uv0, CoordTf::VECTOR2 uv1, CoordTf::VECTOR2 uv2);

	void createTangent(int numMaterial, UINT* indexCntArr,
		void* vertexArr, UINT** indexArr, int structByteStride,
		int posBytePos, int texBytePos, int tangentBytePos);

	CoordTf::VECTOR3 normalRecalculation(CoordTf::VECTOR3 Nor[3]);
}

#endif
