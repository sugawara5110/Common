//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@      PolygonData_DXR                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_PolygonData_DXR_Header
#define Class_PolygonData_DXR_Header

#include "DXR_Manager.h"

class PolygonData_DXR :public PolygonData {

private:
	DXR_Manager::ParameterSet* ps = nullptr;

public:
	void GetVBarray(PrimitiveType type, int numMaxInstance);
	void setMaterialType(MaterialType type);
	bool Create(bool light, int tNo, bool blend, bool alpha);
	bool Create(bool light, int tNo, int nortNo, int spetNo, bool blend, bool alpha, float divideBufferMagnification = 1.0f);
	int getEmissiveNo();
	~PolygonData_DXR();
};

#endif
