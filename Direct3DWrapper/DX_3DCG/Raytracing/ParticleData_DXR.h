//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@    ParticleData_DXR                                        **//
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
	void GetBufferParticle(int texture_no, float size, float density);//�e�N�X�`�������Ƀp�[�e�B�N���f�[�^����, �S�̃T�C�Y�{��, ���x
	void GetBufferBill(int v);
	void setMaterialType(MaterialType type);
	bool CreateParticle(int texNo, bool alpha, bool blend);//�p�[�e�B�N��1�̃e�N�X�`���i���o�[
	bool CreateBillboard(bool alpha, bool blend);//ver�̎l�p�`�𐶐�
	int getEmissiveNo();
	~ParticleData_DXR();
};

#endif
