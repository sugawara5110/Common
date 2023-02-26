//*****************************************************************************************//
//**                                                                                     **//
//**                                ParameterDXR                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_ParameterDXR_Header
#define Class_ParameterDXR_Header

#include "Dx_SwapChain.h"

struct UpdateDXR {
	UINT NumInstance = 1;
	std::unique_ptr<std::unique_ptr<VertexView[]>[]> VviewDXR = nullptr;
	std::unique_ptr<std::unique_ptr<UINT[]>[]> currentIndexCount = nullptr;
	std::unique_ptr<CoordTf::MATRIX[]> Transform = nullptr;
	std::unique_ptr<CoordTf::MATRIX[]> WVP = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> AddObjColor = nullptr;//�I�u�W�F�N�g�̐F�ω��p
	float shininess = 0.0f;
	std::unique_ptr<UINT[]> InstanceID = nullptr;
	bool firstSet = false;//VviewDXR�̍ŏ��̃f�[�^�X�V�����t���O
	bool createAS = false;//AS�̍ŏ��̍\�z�����t���O
	UINT InstanceMask = 0xFF;
	float RefractiveIndex = 0.0f;//���ܗ�

	//ParticleData, SkinMesh�p
	bool useVertex = false;
	UINT numVertex = 1;
	std::unique_ptr<CoordTf::VECTOR3[]> v = nullptr;//�����p

	//�|�C���g���C�g
	std::unique_ptr<float[]> plightOn = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> Lightst = nullptr;

	void InstanceMaskChange(bool DrawOn);

	void create(int numMaterial, int numMaxInstance);
};

enum MaterialType {
	NONREFLECTION = 0b0000,
	METALLIC = 0b1000,
	EMISSIVE = 0b0100,
	DIRECTIONLIGHT = 0b0010,
	TRANSLUCENCE = 0b0001
};

struct ParameterDXR {
	int NumMaterial = 0;
	UINT NumMaxInstance = 1;
	bool hs = false;
	std::unique_ptr<MaterialType[]> mType = nullptr;
	std::unique_ptr<ID3D12Resource* []> difTex = nullptr;
	std::unique_ptr<ID3D12Resource* []> norTex = nullptr;
	std::unique_ptr<ID3D12Resource* []> speTex = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> diffuse = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> specular = nullptr;
	std::unique_ptr<CoordTf::VECTOR4[]> ambient = nullptr;
	std::unique_ptr<IndexView[]> IviewDXR = nullptr;
	std::unique_ptr<std::unique_ptr<StreamView[]>[]> SviewDXR = nullptr;
	UpdateDXR updateDXR[2] = {};
	bool updateF = false;//AS�\�z���update�̗L��
	bool tessellationF = false;//�e�Z���[�V�����L��

	void create(int numMaterial, int numMaxInstance);

	void setAllMaterialType(MaterialType type);

	void resetCreateAS();

	bool alphaBlend = false;
	bool alphaTest = false;

	void setPointLight(
		int SwapNo,
		UINT VertexIndex, int MaterialIndex, int InstanceIndex,
		bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	void setPointLightAll(
		int SwapNo,
		bool on_off,
		float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });

	float getplightOn(int SwapNo,
		UINT VertexIndex, int MaterialIndex, int InstanceIndex);

	CoordTf::VECTOR4 getLightst(int SwapNo,
		UINT VertexIndex, int MaterialIndex, int InstanceIndex);
};

#endif