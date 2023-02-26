//*****************************************************************************************//
//**                                                                                     **//
//**                                ParameterDXR                                         **//
//**                                                                                     **//
//*****************************************************************************************//

#include "ParameterDXR.h"

void UpdateDXR::InstanceMaskChange(bool DrawOn) {
	if (DrawOn)InstanceMask = 0xFF;
	else InstanceMask = 0x00;
}

void UpdateDXR::create(int numMaterial, int numMaxInstance) {
	plightOn = std::make_unique<float[]>(numMaxInstance * numMaterial * numVertex);
	for (UINT i = 0; i < numMaxInstance * numMaterial * numVertex; i++)plightOn[i] = 0.0f;
	Lightst = std::make_unique<CoordTf::VECTOR4[]>(numMaxInstance * numMaterial * numVertex);
	InstanceID = std::make_unique<UINT[]>(numMaxInstance * numMaterial);
	Transform = std::make_unique<CoordTf::MATRIX[]>(numMaxInstance);
	WVP = std::make_unique<CoordTf::MATRIX[]>(numMaxInstance);
	AddObjColor = std::make_unique<CoordTf::VECTOR4[]>(numMaxInstance);
	VviewDXR = std::make_unique<std::unique_ptr<VertexView[]>[]>(numMaterial);
	currentIndexCount = std::make_unique<std::unique_ptr<UINT[]>[]>(numMaterial);
	for (int i = 0; i < numMaterial; i++) {
		VviewDXR[i] = std::make_unique<VertexView[]>(numMaxInstance);
		currentIndexCount[i] = std::make_unique<UINT[]>(numMaxInstance);
	}
}

void ParameterDXR::create(int numMaterial, int numMaxInstance) {
	NumMaterial = numMaterial;
	NumMaxInstance = numMaxInstance;
	mType = std::make_unique<MaterialType[]>(numMaterial);
	IviewDXR = std::make_unique<IndexView[]>(numMaterial);
	difTex = std::make_unique<ID3D12Resource* []>(numMaterial);
	norTex = std::make_unique<ID3D12Resource* []>(numMaterial);
	speTex = std::make_unique<ID3D12Resource* []>(numMaterial);
	diffuse = std::make_unique<CoordTf::VECTOR4[]>(numMaterial);
	specular = std::make_unique<CoordTf::VECTOR4[]>(numMaterial);
	ambient = std::make_unique<CoordTf::VECTOR4[]>(numMaterial);
	SviewDXR = std::make_unique<std::unique_ptr<StreamView[]>[]>(numMaterial);
	for (int i = 0; i < numMaterial; i++) {
		SviewDXR[i] = std::make_unique<StreamView[]>(numMaxInstance);
		mType[i] = NONREFLECTION;
	}
	updateDXR[0].create(numMaterial, numMaxInstance);
	updateDXR[1].create(numMaterial, numMaxInstance);
}

void ParameterDXR::setAllMaterialType(MaterialType type) {
	for (int i = 0; i < NumMaterial; i++)
		mType[i] = type;
}

void ParameterDXR::resetCreateAS() {
	updateDXR[0].createAS = false;
	updateDXR[1].createAS = false;
}

void ParameterDXR::setPointLight(
	int SwapNo,
	UINT VertexIndex, int MaterialIndex, int InstanceIndex,
	bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	int index = VertexIndex * NumMaterial * NumMaxInstance +
		MaterialIndex * NumMaxInstance +
		InstanceIndex;

	UpdateDXR& ud = updateDXR[SwapNo];
	float& plightOn = ud.plightOn[index];
	if (on_off)plightOn = 1.0f; else plightOn = 0.0f;
	ud.Lightst[index].as(range, atten.x, atten.y, atten.z);
}

void ParameterDXR::setPointLightAll(
	int SwapNo,
	bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	UpdateDXR& ud = updateDXR[SwapNo];

	int num = ud.numVertex * NumMaterial * NumMaxInstance;

	for (int i = 0; i < num; i++) {
		float& plightOn = ud.plightOn[i];
		if (on_off)plightOn = 1.0f; else plightOn = 0.0f;
		ud.Lightst[i].as(range, atten.x, atten.y, atten.z);
	}
}

float ParameterDXR::getplightOn(int SwapNo,
	UINT VertexIndex, int MaterialIndex, int InstanceIndex) {

	UpdateDXR& ud = updateDXR[SwapNo];
	int index = VertexIndex * NumMaterial * NumMaxInstance +
		MaterialIndex * NumMaxInstance +
		InstanceIndex;

	return ud.plightOn[index];
}

CoordTf::VECTOR4 ParameterDXR::getLightst(int SwapNo,
	UINT VertexIndex, int MaterialIndex, int InstanceIndex) {

	UpdateDXR& ud = updateDXR[SwapNo];
	int index = VertexIndex * NumMaterial * NumMaxInstance +
		MaterialIndex * NumMaxInstance +
		InstanceIndex;

	return ud.Lightst[index];
}