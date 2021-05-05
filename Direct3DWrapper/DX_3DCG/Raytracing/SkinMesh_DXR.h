//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@      SkinMesh_DXR                                          **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_SkinMesh_DXR_Header
#define Class_SkinMesh_DXR_Header

#include "DXR_Manager.h"
#include "../Rasterize/Dx_SkinMesh.h"

class SkinMesh_DXR :public SkinMesh {

private:
	DXR_Manager::ParameterSet* ps = nullptr;
	int insnum[2] = { 1,1 };

public:
	void GetBuffer(float end_frame, bool singleMesh = false, bool deformer = true);
	void setMaterialType(MaterialType type, int meshIndex = -1, int materialIndex = -1);
	bool CreateFromFBX(bool disp, float divideBufferMagnification = 1.0f);
	bool CreateFromFBX();
	int getEmissiveNo(int meshIndex, int materialIndex);
	~SkinMesh_DXR();
};

#endif