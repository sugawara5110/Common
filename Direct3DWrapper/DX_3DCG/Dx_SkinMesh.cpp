//*****************************************************************************************//
//**                                                                                     **//
//**                                 SkinMeshクラス(FbxLoader)                           **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_SkinMesh.h"
#include <string.h>

using namespace std;

SkinMesh::SkinMesh() {
	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;//頂点数 3 → 3 * 6 = 18
	divArr[1].distance = 500.0f;
	divArr[1].divide = 6;//頂点数 3 → 3 * 54 = 162
	divArr[2].distance = 300.0f;
	divArr[2].divide = 12;//頂点数 3 → 3 * 216 = 648
	numDiv = 3;
}

SkinMesh::~SkinMesh() {
	if (pvVB) {
		for (int i = 0; i < numMesh; i++) {
			ARR_DELETE(pvVB[i]);
		}
	}
	if (pvVBM) {
		for (int i = 0; i < numMesh; i++) {
			ARR_DELETE(pvVBM[i]);
		}
	}
	ARR_DELETE(pvVB);
	ARR_DELETE(pvVBM);
	ARR_DELETE(mObj);
	ARR_DELETE(sk);
	S_DELETE(mObject_BONES);
}

void SkinMesh::SetState(bool al, bool bl, float diffuse, float specu, float ambi) {
	alpha = al;
	blend = bl;
	addDiffuse = diffuse;
	addSpecular = specu;
	addAmbient = ambi;
}

void SkinMesh::GetBuffer(int numMaxInstance, float end_frame, bool singleMesh, bool deformer) {
	float ef[1] = { end_frame };
	GetBuffer(numMaxInstance, 1, ef, singleMesh, deformer);
}

void SkinMesh::GetBuffer(int numMaxInstance, int num_end_frame, float* end_frame, bool singleMesh, bool deformer) {

	using namespace CoordTf;

	getBuffer(num_end_frame, end_frame, singleMesh, deformer);

	if (maxNumBone > 0) {
		sk = new SkinnedCom[numMesh];
		mObject_BONES = new ConstantBuffer<SHADER_GLOBAL_BONES>(1);
	}

	pvVBM = new VertexM * [numMesh];
	mObj = new BasicPolygon[numMesh];

	Dx_Device* dev = Dx_Device::GetInstance();
	FbxLoader* fbL = fbx[0].fbxL;

	for (int i = 0; i < numMesh; i++) {
		mObj[i].getBuffer(fbL->getFbxMeshNode(i)->getNumMaterial(), numMaxInstance, divArr, numDiv);
		if (numBone[i] > 0) {
			sk[i].getBuffer(&mObj[i]);
		}
		if (dev->getDxrCreateResourceState()) {
			UpdateDXR& u0 = mObj[i].dxrPara.updateDXR[0];
			UpdateDXR& u1 = mObj[i].dxrPara.updateDXR[1];
			u0.useVertex = true;
			u0.numVertex = 1;
			u0.v = std::make_unique<VECTOR3[]>(1);
			u1.useVertex = true;
			u1.numVertex = 1;
			u1.v = std::make_unique<VECTOR3[]>(1);
		}
	}
}

void SkinMesh::SetVertex(bool lclOn, bool axisOn, bool VerCentering) {

	Skin_VERTEX_Set vset = setVertex(lclOn, axisOn, VerCentering);
	pvVB = vset.pvVB;
	NumNewIndex = vset.NumNewIndex;
	newIndex = vset.newIndex;

	using namespace CoordTf;
	for (int m = 0; m < numMesh; m++) {

		FbxLoader* fbL = fbx[0].fbxL;
		FbxMeshNode* mesh = fbL->getFbxMeshNode(m);//メッシュ毎に処理する

		Skin_VERTEX* vb = pvVB[m];
		Vertex_M* vb_m = vset.pvVB_M[m];

		VertexM* vbm = pvVBM[m] = new VertexM[mesh->getNumPolygonVertices()];

		for (uint32_t i = 0; i < mesh->getNumPolygonVertices(); i++) {
			VertexM* v = &vbm[i];
			Vertex_M* vv = &vb_m[i];
			v->Pos = vv->Pos;
			v->normal = vv->normal;
			v->tangent = vv->tangent;
			v->geoNormal = vv->geoNormal;
			v->tex = vv->tex0;
		}

		ARR_DELETE(vb_m);

		auto index = mesh->getPolygonVertices();//頂点Index取得(頂点xyzに対してのIndex)

		//同一座標頂点リスト
		SameVertexList* svList = new SameVertexList[mesh->getNumVertices()];
		for (uint32_t i = 0; i < mesh->getNumPolygonVertices(); i++) {
			svList[index[i]].Push(i);
		}

		//同一座標頂点の法線統一化(テセレーション用)
		SameVertexListNormalization svn;
		if (numBone[m] > 0) {
			svn.Normalization(vb, sizeof(Skin_VERTEX), sizeof(VECTOR3) * 3, mesh->getNumVertices(), svList);
			//頂点バッファ
			mObj[m].getVertexBuffer(sizeof(Skin_VERTEX), mesh->getNumPolygonVertices());
		}
		else {
			svn.Normalization(vbm, sizeof(VertexM), sizeof(VECTOR3) * 3, mesh->getNumVertices(), svList);
			//頂点バッファ
			mObj[m].getVertexBuffer(sizeof(VertexM), mesh->getNumPolygonVertices());
		}

		ARR_DELETE(svList);

		char* uv0Name = nullptr;            //テクスチャUVSet名0
		char* uv1Name = nullptr;            //テクスチャUVSet名1
		if (mesh->getNumUVObj() > 1) {
			uv0Name = mesh->getUVName(0);
			uv1Name = mesh->getUVName(1);
		}

		auto numMaterial = mesh->getNumMaterial();
		int* uvSw = new int[numMaterial];
		createMaterial(m, numMaterial, mesh, uv0Name, uv1Name, uvSw);
		if (numBone[m] > 0)swapTex(vb, mesh, uvSw);
		ARR_DELETE(uvSw);
	}

	for (int m = 0; m < numMesh; m++) {
		FbxLoader* fbL = fbx[0].fbxL;
		FbxMeshNode* mesh = fbL->getFbxMeshNode(m);
		//インデックスバッファ
		uint32_t* numNewIndex = NumNewIndex[m];
		for (UINT i = 0; i < mesh->getNumMaterial(); i++) {
			const UINT indexSize = numNewIndex[i] * sizeof(uint32_t);
			mObj[m].getIndexBuffer(i, indexSize, numNewIndex[i]);
		}
		ARR_DELETE(numNewIndex);
	}

	ARR_DELETE(NumNewIndex);
	ARR_DELETE(vset.pvVB_M);
}

void SkinMesh::Vertex_hold() {
	pvVB_delete_f = false;
}

void SkinMesh::createMaterial(int meshInd, UINT numMaterial, FbxMeshNode* mesh,
	char* uv0Name, char* uv1Name, int* uvSw) {

	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	Dx_Device* dev = Dx_Device::GetInstance();
	int m = meshInd;
	for (UINT i = 0; i < numMaterial; i++) {
		//ディフェーズテクスチャId取得
		for (int tNo = 0; tNo < mesh->getNumDiffuseTexture(i); tNo++) {
			textureType type = mesh->getDiffuseTextureType(i, tNo);
			if (type.DiffuseColor && !type.SpecularColor ||
				mesh->getNumDiffuseTexture(i) == 1) {
				auto diffName = Dx_Util::GetNameFromPass(mesh->getDiffuseTextureName(i, tNo));
				mObj[m].dpara.material[i].diftex_no = dx->GetTexNumber(diffName);
				auto str = mesh->getDiffuseTextureUVName(i, tNo);
				if (str)strcpy(mObj[m].dpara.material[i].difUvName, str);
				break;
			}
		}
		//スペキュラテクスチャId取得
		for (int tNo = 0; tNo < mesh->getNumDiffuseTexture(i); tNo++) {
			textureType type = mesh->getDiffuseTextureType(i, tNo);
			if (type.SpecularColor) {
				auto speName = Dx_Util::GetNameFromPass(mesh->getDiffuseTextureName(i, tNo));
				mObj[m].dpara.material[i].spetex_no = dx->GetTexNumber(speName);
				auto str = mesh->getDiffuseTextureUVName(i, tNo);
				if (str)strcpy(mObj[m].dpara.material[i].speUvName, str);
				break;
			}
		}
		//ノーマルテクスチャId取得
		for (int tNo = 0; tNo < mesh->getNumNormalTexture(i); tNo++) {
			//ディフェーズテクスチャ用のノーマルマップしか使用しないので
			//取得済みのディフェーズテクスチャUV名と同じUV名のノーマルマップを探す
			auto uvNorName = mesh->getNormalTextureUVName(i, tNo);
			if (mesh->getNumNormalTexture(i) == 1 ||
				uvNorName && !strcmp(mObj[m].dpara.material[i].difUvName, uvNorName)) {
				auto norName = Dx_Util::GetNameFromPass(mesh->getNormalTextureName(i, tNo));
				mObj[m].dpara.material[i].nortex_no = dx->GetTexNumber(norName);
				auto str = mesh->getNormalTextureUVName(i, tNo);
				if (str)strcpy(mObj[m].dpara.material[i].norUvName, str);
				break;
			}
		}
		uvSw[i] = 0;
		if (uv0Name != nullptr) {
			char* difName = mObj[m].dpara.material[i].difUvName;
			char* speName = mObj[m].dpara.material[i].speUvName;
			//uv逆転
			if (!strcmp(difName, uv1Name) &&
				(!strcmp(speName, "") || !strcmp(speName, uv0Name)))
				uvSw[i] = 1;
			//どちらもuv0
			if (!strcmp(difName, uv0Name) &&
				(!strcmp(speName, "") || !strcmp(speName, uv0Name)))
				uvSw[i] = 2;
			//どちらもuv1
			if (!strcmp(difName, uv1Name) &&
				(!strcmp(speName, "") || !strcmp(speName, uv1Name)))
				uvSw[i] = 3;
		}

		CONSTANT_BUFFER2 sg = {};
		//拡散反射光
		CoordTf::VECTOR4* diffuse = &mObj[m].dpara.material[i].diffuse;
		float DiffuseFactor = (float)mesh->getDiffuseFactor(i);
		diffuse->x = (float)mesh->getDiffuseColor(i, 0) * DiffuseFactor + addDiffuse;
		diffuse->y = (float)mesh->getDiffuseColor(i, 1) * DiffuseFactor + addDiffuse;
		diffuse->z = (float)mesh->getDiffuseColor(i, 2) * DiffuseFactor + addDiffuse;
		diffuse->w = 0.0f;//使用してない
		//スペキュラー
		CoordTf::VECTOR4* specular = &mObj[m].dpara.material[i].specular;
		float SpecularFactor = (float)mesh->getSpecularFactor(i);
		specular->x = (float)mesh->getSpecularColor(i, 0) * SpecularFactor + addSpecular;
		specular->y = (float)mesh->getSpecularColor(i, 1) * SpecularFactor + addSpecular;
		specular->z = (float)mesh->getSpecularColor(i, 2) * SpecularFactor + addSpecular;
		specular->w = 0.0f;//使用してない
		//アンビエント
		CoordTf::VECTOR4* ambient = &mObj[m].dpara.material[i].ambient;
		float AmbientFactor = (float)mesh->getAmbientFactor(i);
		ambient->x = (float)mesh->getAmbientColor(i, 0) * AmbientFactor + addAmbient;
		ambient->y = (float)mesh->getAmbientColor(i, 1) * AmbientFactor + addAmbient;
		ambient->z = (float)mesh->getAmbientColor(i, 2) * AmbientFactor + addAmbient;
		ambient->w = 0.0f;//使用してない

		sg.vDiffuse = *diffuse;//ディフューズカラーをシェーダーに渡す
		sg.vSpeculer = *specular;//スペキュラーをシェーダーに渡す
		sg.vAmbient = *ambient;//アンビエントをシェーダーに渡す
		mObj[m].mObjectCB1->CopyData(i, sg);
		if (dev->getDxrCreateResourceState()) {
			mObj[m].setColorDXR(i, sg);
		}
	}
}

static void swap(CoordTf::VECTOR2* a, CoordTf::VECTOR2* b) {
	float tmp;
	tmp = a->x;
	a->x = b->x;
	b->x = tmp;
	tmp = a->y;
	a->y = b->y;
	b->y = tmp;
}

static void swaptex(Skin_VERTEX* v, int uvSw) {
	switch (uvSw) {
	case 0:
		break;
	case 1:
		swap(v->vTex0, v->vTex1);
		break;
	case 2:
		v->vTex1 = v->vTex0;
		break;
	case 3:
		v->vTex0 = v->vTex1;
		break;
	}
}

void SkinMesh::swapTex(Skin_VERTEX* vb, FbxMeshNode* mesh, int* uvSw) {
	int Icnt = 0;
	for (UINT i = 0; i < mesh->getNumPolygon(); i++) {
		UINT currentMatNo = mesh->getMaterialNoOfPolygon(i);
		UINT pSize = mesh->getPolygonSize(i);
		if (pSize >= 3) {
			for (UINT ps = 0; ps < pSize; ps++) {
				swaptex(&vb[Icnt + ps], uvSw[currentMatNo]);
			}
			Icnt += pSize;
		}
	}
}

void SkinMesh::SetDiffuseTextureName(char* textureName, int materialIndex, int meshIndex) {
	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	mObj[meshIndex].dpara.material[materialIndex].diftex_no = dx->GetTexNumber(textureName);
}

void SkinMesh::SetNormalTextureName(char* textureName, int materialIndex, int meshIndex) {
	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	mObj[meshIndex].dpara.material[materialIndex].nortex_no = dx->GetTexNumber(textureName);
}

void SkinMesh::SetSpeculerTextureName(char* textureName, int materialIndex, int meshIndex) {
	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	mObj[meshIndex].dpara.material[materialIndex].spetex_no = dx->GetTexNumber(textureName);
}

void SkinMesh::GetShaderByteCode(bool disp, bool smooth) {
	for (int i = 0; i < numMesh; i++) {
		BasicPolygon& o = mObj[i];
		if (disp) {
			if (numBone[i] > 0)
				o.GetShaderByteCode(CONTROL_POINT, true, smooth, false, Dx_ShaderHolder::pVertexShader_SKIN_D.Get(), nullptr);
			else
				o.GetShaderByteCode(CONTROL_POINT, true, smooth, false, o.vs = Dx_ShaderHolder::pVertexShader_MESH_D.Get(), nullptr);
		}
		else {
			if (numBone[i] > 0)
				o.GetShaderByteCode(SQUARE, true, smooth, false, Dx_ShaderHolder::pVertexShader_SKIN.Get(), nullptr);
			else
				o.GetShaderByteCode(SQUARE, true, smooth, false, o.vs = Dx_ShaderHolder::pVertexShader_MESH.Get(), nullptr);
		}
	}
}

void SkinMesh::setMaterialType(MaterialType type, int materialIndex, int meshIndex) {
	if (materialIndex == -1) {
		for (int i = 0; i < numMesh; i++)
			mObj[i].dxrPara.setAllMaterialType(type);
		return;
	}
	mObj[meshIndex].dxrPara.mType[materialIndex] = type;
}

void SkinMesh::setPointLight(int meshIndex, int materialIndex, int InstanceIndex, bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	Dx_Device* dev = Dx_Device::GetInstance();
	mObj[meshIndex].dxrPara.setPointLight(dev->dxrBuffSwapIndex(), 0, materialIndex, InstanceIndex, on_off, range, atten);
}

void SkinMesh::setPointLightAll(bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	Dx_Device* dev = Dx_Device::GetInstance();
	for (int i = 0; i < numMesh; i++) {
		mObj[i].dxrPara.setPointLightAll(dev->dxrBuffSwapIndex(), on_off, range, atten);
	}
}

bool SkinMesh::CreateFromFBX(int comIndex, bool disp, bool smooth, float divideBufferMagnification) {
	GetShaderByteCode(disp, smooth);
	const int numSrvTex = 3;
	const int numCbv = 3;

	for (int i = 0; i < numMesh; i++) {
		BasicPolygon& o = mObj[i];
		o.setDivideArr(divArr, numDiv);

		UINT* indexCntArr = new UINT[o.dpara.NumMaterial];
		for (int m = 0; m < o.dpara.NumMaterial; m++) {
			indexCntArr[m] = o.dpara.Iview[m].IndexCount;
		}
		int vSize = sizeof(VertexM);
		void* vertex = pvVBM[i];
		if (numBone[i] > 0) {
			vSize = sizeof(Skin_VERTEX);
			vertex = pvVB[i];
		}
		Dx_Util::createTangent(o.dpara.NumMaterial, indexCntArr,
			vertex, newIndex[i], vSize, 0, 3 * 4, 12 * 4, 6 * 4);
		ARR_DELETE(indexCntArr);

		o.createDefaultBuffer(comIndex, vertex, newIndex[i]);
		if (pvVB_delete_f) {
			ARR_DELETE(pvVBM[i]);
			ARR_DELETE(pvVB[i]);
		}
		for (int m = 0; m < o.dpara.NumMaterial; m++) {
			ARR_DELETE(newIndex[i][m]);
		}
		ARR_DELETE(newIndex[i]);
		int numUav = 0;
		o.createParameterDXR(comIndex, alpha, blend, divideBufferMagnification);
		if (numBone[i] > 0) {
			if (!sk[i].createParameterDXR(comIndex))return false;
			if (!o.createPSO(Dx_ShaderHolder::pVertexLayout_SKIN, numSrvTex, numCbv, numUav, blend, alpha))return false;
			if (!o.createPSO_DXR(Dx_ShaderHolder::pVertexLayout_SKIN, numSrvTex, numCbv, numUav, smooth))return false;
			if (!sk[i].createPSO())return false;
			UINT cbSize = mObject_BONES->getSizeInBytes();
			D3D12_GPU_VIRTUAL_ADDRESS ad = mObject_BONES->Resource()->GetGPUVirtualAddress();
			if (!o.setDescHeap(comIndex, numSrvTex, 0, nullptr, nullptr, numCbv, ad, cbSize))return false;
			if (!sk[i].createDescHeap(ad, cbSize))return false;
		}
		else {
			if (!o.createPSO(Dx_ShaderHolder::pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;
			if (!o.createPSO_DXR(Dx_ShaderHolder::pVertexLayout_MESH, numSrvTex, numCbv, numUav, smooth))return false;
			if (!o.setDescHeap(comIndex, numSrvTex, 0, nullptr, nullptr, numCbv, 0, 0))return false;
		}
	}
	if (pvVB_delete_f) {
		ARR_DELETE(pvVBM);
		ARR_DELETE(pvVB);
	}
	ARR_DELETE(newIndex);
	return true;
}

void SkinMesh::GetMeshCenterPos() {
	using namespace CoordTf;
	for (int i = 0; i < numMesh; i++) {
		Dx_Device* dev = Dx_Device::GetInstance();
		if (dev->getDxrCreateResourceState()) {
			VECTOR3 cp = centerPos[i].pos;
			UpdateDXR& ud = mObj[i].dxrPara.updateDXR[dev->dxrBuffSwapIndex()];//StreamOutput内もdxrBuffSwapIndex()だが書き込み箇所が異なるのでOK
			VECTOR3* vv = &ud.v[0];
			vv->as(cp.x, cp.y, cp.z);
			if (numBone[i] > 0) {
				float w = centerPos[i].bBoneWeight;
				UINT ind = centerPos[i].bBoneIndex;
				MATRIX m = GetCurrentPoseMatrix(ind);
				VectorMatrixMultiply(vv, &m);
				VectorMultiply(vv, w);
			}
		}
	}
}

void SkinMesh::Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
	CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size) {

	for (int i = 0; i < numMesh; i++) {
		if (!noUseMesh[i])
			mObj[i].Instancing(pos, angle, size, Color);
	}
}

bool SkinMesh::InstancingUpdate(int ind, float ti, int InternalAnimationIndex,
	float disp,
	float SmoothRange, float SmoothRatio, float shininess) {

	bool frame_end = false;
	int insnum = 0;
	if (ti != -1.0f)frame_end = SetNewPoseMatrices(ti, ind, InternalAnimationIndex);
	Dx_Device* dev = Dx_Device::GetInstance();
	MatrixMap_Bone(&sgb[dev->cBuffSwapUpdateIndex()]);
	GetMeshCenterPos();

	for (int i = 0; i < numMesh; i++) {
		if (!noUseMesh[i])
			mObj[i].InstancingUpdate(disp, SmoothRange, SmoothRatio, shininess);
	}

	return frame_end;
}

bool SkinMesh::Update(int ind, float ti,
	CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
	CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
	int InternalAnimationIndex,
	float disp,
	float SmoothRange, float SmoothRatio, float shininess) {

	Instancing(pos, Color, angle, size);
	return InstancingUpdate(ind, ti, InternalAnimationIndex, disp, SmoothRange, SmoothRatio, shininess);
}

void SkinMesh::DrawOff() {
	for (int i = 0; i < numMesh; i++)
		mObj[i].DrawOff();
}

void SkinMesh::Draw(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();
	if (mObject_BONES)mObject_BONES->CopyData(0, sgb[dev->cBuffSwapDrawOrStreamoutputIndex()]);

	for (int i = 0; i < numMesh; i++)
		mObj[i].Draw(comIndex);
}

void SkinMesh::StreamOutput(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();
	if (mObject_BONES)mObject_BONES->CopyData(0, sgb[dev->cBuffSwapDrawOrStreamoutputIndex()]);

	for (int i = 0; i < numMesh; i++) {
		mObj[i].StreamOutput(comIndex);
		if (numBone[i] > 0)sk[i].Skinning(comIndex);
	}
}

void SkinMesh::CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int texIndex, int meshIndex) {
	mObj[meshIndex].CopyResource(comIndex, texture, res, texIndex);
}

void SkinMesh::TextureInit(int width, int height, int texIndex, int meshIndex) {
	mObj[meshIndex].TextureInit(width, height, texIndex);
}

HRESULT SkinMesh::SetTextureMPixel(int com_no, BYTE* frame, int texIndex, int meshIndex) {
	return mObj[meshIndex].SetTextureMPixel(com_no, frame, texIndex);
}

CoordTf::VECTOR3 SkinMesh::GetVertexPosition(int meshIndex, int verNum, float adjustZ, float adjustY, float adjustX,
	float thetaZ, float thetaY, float thetaX, float scal) {

	using namespace CoordTf;
	//頂点にボーン行列を掛け出力
	VECTOR3 ret, pos;
	MATRIX rotZ, rotY, rotX, rotZY, rotZYX;
	MATRIX scale, scaro;
	MatrixScaling(&scale, scal, scal, scal);
	MatrixRotationZ(&rotZ, thetaZ);
	MatrixRotationY(&rotY, thetaY);
	MatrixRotationX(&rotX, thetaX);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&rotZYX, &rotZY, &rotX);
	MatrixMultiply(&scaro, &scale, &rotZYX);
	ret.x = ret.y = ret.z = 0.0f;

	for (int i = 0; i < 4; i++) {
		pos.x = pvVB[meshIndex][verNum].vPos.x;
		pos.y = pvVB[meshIndex][verNum].vPos.y;
		pos.z = pvVB[meshIndex][verNum].vPos.z;
		MATRIX m = GetCurrentPoseMatrix(pvVB[meshIndex][verNum].bBoneIndex[i]);
		VectorMatrixMultiply(&pos, &m);
		VectorMultiply(&pos, pvVB[meshIndex][verNum].bBoneWeight[i]);
		VectorAddition(&ret, &ret, &pos);
	}
	ret.x += adjustX;
	ret.y += adjustY;
	ret.z += adjustZ;
	VectorMatrixMultiply(&ret, &scaro);
	return ret;
}
