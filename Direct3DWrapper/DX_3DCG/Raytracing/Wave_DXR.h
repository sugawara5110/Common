//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         Wave_DXR                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Wave_DXR_Header
#define Class_Wave_DXR_Header

#include "DXR_Manager.h"
#include "../Rasterize/Dx_Wave.h"

class Wave_DXR : public Wave {

private:
	DXR_Manager::ParameterSet* ps = nullptr;

public:
	void GetVBarray(int numMaxInstance);
	void setMaterialType(MaterialType type);
	bool Create(int texNo, bool blend, bool alpha, float waveHeight, float divide);
	bool Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide);
	int getEmissiveNo();
	~Wave_DXR();
};

#endif
