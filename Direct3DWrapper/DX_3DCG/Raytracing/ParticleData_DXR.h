//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　    ParticleData_DXR                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_ParticleData_DXR_Header
#define Class_ParticleData_DXR_Header

#include "DXR_Manager.h"
#include "../Rasterize/Dx_ParticleData.h"

class ParticleData_DXR :public ParticleData {

private:
	DXR_Manager::ParameterSet* ps = nullptr;
	int insnum[2] = { 1,1 };

public:
	void GetBufferParticle(int texture_no, float size, float density);//テクスチャを元にパーティクルデータ生成, 全体サイズ倍率, 密度
	void GetBufferBill(int v);
	void setMaterialType(MaterialType type);
	bool CreateParticle(int texNo, bool alpha, bool blend);//パーティクル1個のテクスチャナンバー
	bool CreateBillboard(bool alpha, bool blend);//ver個の四角形を生成
	int getEmissiveNo();
	~ParticleData_DXR();
};

#endif
