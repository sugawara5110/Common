//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@      MeshData_DXR                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_MeshData_DXR_Header
#define Class_MeshData_DXR_Header

#include "DXR_Manager.h"
#include "../Rasterize/Dx_MeshData.h"

class MeshData_DXR :public MeshData {

private:
	DXR_Manager::ParameterSet* ps = nullptr;

public:
	bool GetBuffer(char* FileName, int numMaxInstance);
	void setMaterialType(MaterialType type, int materialIndex = -1);
	bool CreateMesh(float divideBufferMagnification = 1.0f);
	int getEmissiveNo(int materialIndex);
	~MeshData_DXR();
};

#endif