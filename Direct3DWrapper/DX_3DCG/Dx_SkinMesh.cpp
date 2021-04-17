//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@        SkinMesh�N���X(FbxLoader)                           **//
//**                                                                                     **//
//*****************************************************************************************//

//Smoothie3D�Ń��f����Blender�ŃA�j���[�V�����̏ꍇ,-Z�O��,Y��ŏo�͂���
//makehuman�Ń��f����Blender�ŃA�j���[�V����(�J�[�l�M�[��������w��GET�������[�V�����L���v�`���f�[�^�g�p)
//�̏ꍇ,Y�O��,Z��ŏo�͂���

#define _CRT_SECURE_NO_WARNINGS
#define FBX_PCS 5
#include "Dx_SkinMesh.h"
#include <string.h>

using namespace std;

SkinMesh_sub::SkinMesh_sub() {
	fbxL = new FbxLoader();
	MatrixIdentity(&rotZYX);
	connect_step = 3000.0f;
}

SkinMesh_sub::~SkinMesh_sub() {
	S_DELETE(fbxL);
}

bool SkinMesh_sub::Create(CHAR *szFileName) {
	return fbxL->setFbxFile(szFileName);
}

SkinMesh::SkinMesh() {
	ZeroMemory(this, sizeof(SkinMesh));
	fbx = new SkinMesh_sub[FBX_PCS];
	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;//���_�� 3 �� 3 * 6 = 18
	divArr[1].distance = 500.0f;
	divArr[1].divide = 6;//���_�� 3 �� 3 * 54 = 162
	divArr[2].distance = 300.0f;
	divArr[2].divide = 12;//���_�� 3 �� 3 * 216 = 648
	BoneConnect = -1.0f;
	numDiv = 3;
	AnimLastInd = -1;
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
	ARR_DELETE(boneName);
	ARR_DELETE(m_ppSubAnimationBone);
	ARR_DELETE(m_pLastBoneMatrix);
	ARR_DELETE(m_BoneArray);
	ARR_DELETE(mObj);
	ARR_DELETE(sk);
	S_DELETE(mObject_BONES);

	DestroyFBX();
}

void SkinMesh::SetState(bool al, bool bl, float diffuse, float specu, float ambi) {
	alpha = al;
	blend = bl;
	addDiffuse = diffuse;
	addSpecular = specu;
	addAmbient = ambi;
}

void SkinMesh::ObjCentering(bool f, int ind) {
	fbx[ind].centering = f;
	fbx[ind].offset = false;
	fbx[ind].cx = fbx[ind].cy = fbx[ind].cz = 0.0f;
}

void SkinMesh::CreateRotMatrix(float thetaZ, float thetaY, float thetaX, int ind) {
	using namespace CoordTf;
	MATRIX rotZ, rotY, rotX, rotZY;
	MatrixIdentity(&fbx[ind].rotZYX);
	MatrixRotationZ(&rotZ, thetaZ);
	MatrixRotationY(&rotY, thetaY);
	MatrixRotationX(&rotX, thetaX);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&fbx[ind].rotZYX, &rotZY, &rotX);
}

void SkinMesh::ObjCentering(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind) {
	fbx[ind].centering = true;
	fbx[ind].offset = false;
	fbx[ind].cx = x;
	fbx[ind].cy = y;
	fbx[ind].cz = z;
	CreateRotMatrix(thetaZ, thetaY, thetaX, ind);
}

void SkinMesh::ObjOffset(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind) {
	fbx[ind].centering = true;
	fbx[ind].offset = true;
	fbx[ind].cx = x;
	fbx[ind].cy = y;
	fbx[ind].cz = z;
	CreateRotMatrix(thetaZ, thetaY, thetaX, ind);
}

HRESULT SkinMesh::InitFBX(CHAR *szFileName, int p) {
	bool f = false;
	f = fbx[p].Create(szFileName);
	if (f)return S_OK;
	return E_FAIL;
}

void SkinMesh::DestroyFBX() {
	ARR_DELETE(fbx);
}

void SkinMesh::ReadSkinInfo(FbxMeshNode* mesh, MY_VERTEX_S* pvVB, meshCenterPos* cenPos) {

	cenPos->bBoneWeight = 0.0f;
	int addWeightCnt = 0;
	//�eBone�̃E�G�C�g,�C���f�b�N�X�𒲂ג��_�z��ɉ�����
	for (int i = 0; i < numBone; i++) {
		Deformer* defo = mesh->getDeformer(i);
		if (!defo)return;
		int iNumIndex = defo->getIndicesCount();//���̃{�[���ɉe�����󂯂钸�_�C���f�b�N�X��
		int* piIndex = defo->getIndices();     //���̃{�[���ɉe�����󂯂钸�_�̃C���f�b�N�X�z��
		double* pdWeight = defo->getWeights();//���̃{�[���ɉe�����󂯂钸�_�̃E�G�C�g�z��

		for (int k = 0; k < iNumIndex; k++) {
			int index = piIndex[k];      //�e�����󂯂钸�_
			double weight = pdWeight[k];//�E�G�C�g
			for (int m = 0; m < 4; m++) {
				//�eBone���ɉe�����󂯂钸�_�̃E�G�C�g����ԑ傫�����l�ɍX�V���Ă���
				if (weight > pvVB[index].bBoneWeight[m]) {//���ׂ��E�G�C�g�̕����傫��
					pvVB[index].bBoneIndex[m] = i;//Bone�C���f�b�N�X�o�^
					cenPos->bBoneIndex = i;//�Ƃ肠�����ǂꂩ
					pvVB[index].bBoneWeight[m] = (float)weight;//�E�G�C�g�o�^
					cenPos->bBoneWeight += (float)weight;
					addWeightCnt++;
					break;
				}
			}
		}
	}
	cenPos->bBoneWeight /= (float)addWeightCnt;
	//�E�G�C�g���K��
	for (UINT i = 0; i < mesh->getNumVertices(); i++) {
		float we = 0;
		for (int j = 0; j < 4; j++) {
			we += pvVB[i].bBoneWeight[j];
		}
		float we1 = 1.0f / we;
		for (int j = 0; j < 4; j++) {
			pvVB[i].bBoneWeight[j] *= we1;
		}
	}
}

void SkinMesh::SetConnectStep(int ind, float step) {
	fbx[ind].connect_step = step;
}

void SkinMesh::Vertex_hold() {
	pvVB_delete_f = false;
}

HRESULT SkinMesh::GetFbx(CHAR* szFileName) {
	//FBX���[�_�[��������
	if (FAILED(InitFBX(szFileName, 0)))
	{
		MessageBox(0, L"FBX���[�_�[���������s", nullptr, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}

void SkinMesh::GetBuffer(float end_frame, bool singleMesh, bool deformer) {

	using namespace CoordTf;
	fbx[0].end_frame = end_frame;
	FbxLoader* fbL = fbx[0].fbxL;
	if (singleMesh)fbL->createFbxSingleMeshNode();
	numMesh = fbL->getNumFbxMeshNode();
	newIndex = new UINT * *[numMesh];
	centerPos = std::make_unique<meshCenterPos[]>(numMesh);

	FbxMeshNode* mesh = fbL->getFbxMeshNode(0);
	numBone = mesh->getNumDeformer();
	if (!deformer)numBone = 0;

	if (numBone > 0) {
		boneName = new char[numBone * 255];
		m_BoneArray = new BONE[numBone];
		m_pLastBoneMatrix = new MATRIX[numBone];

		for (int i = 0; i < numBone; i++) {
			Deformer* defo = mesh->getDeformer(i);
			const char* name = defo->getName();
			strcpy(&boneName[i * 255], name);//�{�[���̖��O�ێ�

			//�����p���s��ǂݍ���
			//GetCurrentPoseMatrix�Ŏg��
			for (int x = 0; x < 4; x++) {
				for (int y = 0; y < 4; y++) {
					m_BoneArray[i].mBindPose.m[y][x] = (float)defo->getTransformLinkMatrix(y, x);
				}
			}
		}
		sk = new SkinnedCom[numMesh];
		mObject_BONES = new ConstantBuffer<SHADER_GLOBAL_BONES>(1);
	}

	pvVB = new MY_VERTEX_S * [numMesh];
	pvVBM = new VertexM * [numMesh];
	mObj = new PolygonData[numMesh];

	for (int i = 0; i < numMesh; i++) {
		mObj[i].SetCommandList(com_no);
		mObj[i].getBuffer(fbL->getFbxMeshNode(i)->getNumMaterial(), 1, divArr, numDiv);
		if (numBone > 0)sk[i].getBuffer(&mObj[i]);
		if (mObj[i].dx->DXR_CreateResource) {
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

void SkinMesh::normalRecalculation(bool lclOn, double** nor, FbxMeshNode* mesh) {

	*nor = new double[mesh->getNumPolygonVertices() * 3];
	auto index = mesh->getPolygonVertices();//���_Index�擾(���_xyz�ɑ΂��Ă�Index)
	auto ver = mesh->getVertices();//���_�擾
	GlobalSettings gSet = fbx[0].fbxL->getGlobalSettings();

	CoordTf::VECTOR3 tmpv[3] = {};
	UINT indexCnt = 0;
	for (UINT i = 0; i < mesh->getNumPolygon(); i++) {
		UINT pSize = mesh->getPolygonSize(i);//1�|���S���ł̒��_��
		if (pSize >= 3) {
			for (UINT i1 = 0; i1 < 3; i1++) {
				UINT ind = indexCnt + i1;
				if (lclOn) {
					float v[3] = {
						(float)ver[index[ind] * 3] * gSet.CoordAxisSign,
						(float)ver[index[ind] * 3 + 1] * -gSet.FrontAxisSign,
						(float)ver[index[ind] * 3 + 2] * gSet.UpAxisSign
					};
					tmpv[i1].as(
						(float)ver[gSet.CoordAxis],
						(float)ver[gSet.FrontAxis],
						(float)ver[gSet.UpAxis]
					);
				}
				else {
					tmpv[i1].as(
						(float)ver[index[ind] * 3],
						(float)ver[index[ind] * 3 + 1],
						(float)ver[index[ind] * 3 + 2]
					);
				}
			}
			//��L3���_����@���̕����Z�o
			CoordTf::VECTOR3 N = Dx_Util::normalRecalculation(tmpv);
			for (UINT ps = 0; ps < pSize; ps++) {
				UINT ind = indexCnt + ps;
				(*nor)[ind * 3] = (double)N.x;
				(*nor)[ind * 3 + 1] = (double)N.y;
				(*nor)[ind * 3 + 2] = (double)N.z;
			}
			indexCnt += pSize;
		}
	}
}

void SkinMesh::SetVertex(bool lclOn) {

	FbxLoader* fbL = fbx[0].fbxL;
	GlobalSettings gSet = fbL->getGlobalSettings();

	using namespace CoordTf;
	for (int m = 0; m < numMesh; m++) {

		FbxMeshNode* mesh = fbL->getFbxMeshNode(m);//���b�V�����ɏ�������

		MY_VERTEX_S* tmpVB = new MY_VERTEX_S[mesh->getNumVertices()];
		//�{�[���E�G�C�g
		if (numBone > 0)ReadSkinInfo(mesh, tmpVB, &centerPos[m]);

		auto index = mesh->getPolygonVertices();//���_Index�擾(���_xyz�ɑ΂��Ă�Index)
		auto ver = mesh->getVertices();//���_�擾
		auto nor = mesh->getNormal(0);//�@���擾

		bool norCreate = false;
		if (!nor) {
			normalRecalculation(lclOn, &nor, mesh);
			norCreate = true;
		}

		double* uv0 = mesh->getAlignedUV(0);//�e�N�X�`��UV0
		char* uv0Name = nullptr;            //�e�N�X�`��UVSet��0
		double* uv1 = nullptr;              //�e�N�X�`��UV1
		char* uv1Name = nullptr;            //�e�N�X�`��UVSet��1
		if (mesh->getNumUVObj() > 1) {
			uv1 = mesh->getAlignedUV(1);
			uv0Name = mesh->getUVName(0);
			uv1Name = mesh->getUVName(1);
		}
		else {
			uv1 = mesh->getAlignedUV(0);
		}

		//������W���_���X�g
		SameVertexList* svList = new SameVertexList[mesh->getNumVertices()];
		pvVB[m] = new MY_VERTEX_S[mesh->getNumPolygonVertices()];
		pvVBM[m] = new VertexM[mesh->getNumPolygonVertices()];

		meshCenterPos& cp = centerPos[m];
		VECTOR3& cpp = cp.pos;
		int cppAddCnt = 0;
		MY_VERTEX_S* vb = pvVB[m];
		VertexM* vbm = pvVBM[m];
		for (UINT i = 0; i < mesh->getNumPolygonVertices(); i++) {
			//index���Œ��_�𐮗񂵂Ȃ��璸�_�i�[
			MY_VERTEX_S* v = &vb[i];
			VertexM* vm = &vbm[i];
			if (lclOn) {
				float tmpv[3] = {
					(float)ver[index[i] * 3] * gSet.CoordAxisSign,
					(float)ver[index[i] * 3 + 1] * -gSet.FrontAxisSign,
					(float)ver[index[i] * 3 + 2] * gSet.UpAxisSign
				};
				cpp.x += vm->Pos.x = v->vPos.x = tmpv[gSet.CoordAxis] * (float)mesh->getLcl().Scaling[0];
				cpp.y += vm->Pos.y = v->vPos.y = tmpv[gSet.FrontAxis] * (float)mesh->getLcl().Scaling[1];
				cpp.z += vm->Pos.z = v->vPos.z = tmpv[gSet.UpAxis] * (float)mesh->getLcl().Scaling[2];
			}
			else {
				cpp.x += vm->Pos.x = v->vPos.x = (float)ver[index[i] * 3];
				cpp.y += vm->Pos.y = v->vPos.y = (float)ver[index[i] * 3 + 1];
				cpp.z += vm->Pos.z = v->vPos.z = (float)ver[index[i] * 3 + 2];
			}
			cppAddCnt++;
			vm->normal.x = v->vNorm.x = (float)nor[i * 3];
			vm->normal.y = v->vNorm.y = (float)nor[i * 3 + 1];
			vm->normal.z = v->vNorm.z = (float)nor[i * 3 + 2];
			vm->geoNormal.x = v->vGeoNorm.x = (float)nor[i * 3];
			vm->geoNormal.y = v->vGeoNorm.y = (float)nor[i * 3 + 1];
			vm->geoNormal.z = v->vGeoNorm.z = (float)nor[i * 3 + 2];
			vm->tex.x = v->vTex0.x = (float)uv0[i * 2];
			vm->tex.y = v->vTex0.y = 1.0f - (float)uv0[i * 2 + 1];//(1.0f-UV)
			v->vTex1.x = (float)uv1[i * 2];
			v->vTex1.y = 1.0f - (float)uv1[i * 2 + 1];//(1.0f-UV)
			svList[index[i]].Push(i);
			if (numBone > 0) {
				for (int bi = 0; bi < 4; bi++) {
					//ReadSkinInfo(tmpVB)�œǂݍ��񂾊e�p�����[�^�R�s�[
					v->bBoneIndex[bi] = tmpVB[index[i]].bBoneIndex[bi];
					v->bBoneWeight[bi] = tmpVB[index[i]].bBoneWeight[bi];
				}
			}
		}
		ARR_DELETE(tmpVB);
		if (norCreate)ARR_DELETE(nor);

		cpp.x /= (float)cppAddCnt;
		cpp.y /= (float)cppAddCnt;
		cpp.z /= (float)cppAddCnt;
		//������W���_�̖@�����ꉻ(�e�Z���[�V�����p)
		SameVertexListNormalization svn;
		if (numBone > 0) {
			svn.Normalization(vb, sizeof(MY_VERTEX_S), sizeof(VECTOR3) * 3, mesh->getNumVertices(), svList);
			//���_�o�b�t�@
			mObj[m].getVertexBuffer(sizeof(MY_VERTEX_S), mesh->getNumPolygonVertices());
		}
		else {
			svn.Normalization(vbm, sizeof(VertexM), sizeof(VECTOR3) * 3, mesh->getNumVertices(), svList);
			//���_�o�b�t�@
			mObj[m].getVertexBuffer(sizeof(VertexM), mesh->getNumPolygonVertices());
		}

		ARR_DELETE(svList);

		auto numMaterial = mesh->getNumMaterial();
		int* uvSw = new int[numMaterial];
		createMaterial(m, numMaterial, mesh, uv0Name, uv1Name, uvSw);
		if (numBone > 0)swapTex(vb, mesh, uvSw);
		ARR_DELETE(uvSw);
		splitIndex(numMaterial, mesh, m);
	}
}

void SkinMesh::splitIndex(UINT numMaterial, FbxMeshNode* mesh, int m) {

	//�|���S���������Index���J�E���g
	UINT* numNewIndex = new UINT[numMaterial];
	memset(numNewIndex, 0, sizeof(UINT) * numMaterial);

	for (UINT i = 0; i < mesh->getNumPolygon(); i++) {
		UINT currentMatNo = mesh->getMaterialNoOfPolygon(i);
		UINT pSize = mesh->getPolygonSize(i);
		if (pSize >= 3) {
			numNewIndex[currentMatNo] += (pSize - 2) * 3;
		}
	}

	//�������Index����
	newIndex[m] = new UINT * [numMaterial];
	for (UINT ind1 = 0; ind1 < numMaterial; ind1++) {
		if (numNewIndex[ind1] <= 0) {
			newIndex[m][ind1] = nullptr;
			continue;
		}
		newIndex[m][ind1] = new UINT[numNewIndex[ind1]];
	}
	std::unique_ptr<UINT[]> indexCnt;
	indexCnt = std::make_unique<UINT[]>(numMaterial);

	int Icnt = 0;
	for (UINT i = 0; i < mesh->getNumPolygon(); i++) {
		UINT currentMatNo = mesh->getMaterialNoOfPolygon(i);
		UINT pSize = mesh->getPolygonSize(i);
		if (pSize >= 3) {
			for (UINT ps = 0; ps < pSize - 2; ps++) {
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 1 + ps;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 2 + ps;
			}
			Icnt += pSize;
		}
	}

	//�C���f�b�N�X�o�b�t�@
	for (UINT i = 0; i < numMaterial; i++) {
		const UINT indexSize = numNewIndex[i] * sizeof(UINT);
		mObj[m].getIndexBuffer(i, indexSize, numNewIndex[i]);
	}
	ARR_DELETE(numNewIndex);
}

void SkinMesh::createMaterial(int meshInd, UINT numMaterial, FbxMeshNode* mesh,
	char* uv0Name, char* uv1Name, int* uvSw) {

	Dx12Process* dx = mObj[0].dx;
	int m = meshInd;
	for (UINT i = 0; i < numMaterial; i++) {
		//�f�B�t�F�[�Y�e�N�X�`��Id�擾
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
		//�X�y�L�����e�N�X�`��Id�擾
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
		//�m�[�}���e�N�X�`��Id�擾
		for (int tNo = 0; tNo < mesh->getNumNormalTexture(i); tNo++) {
			//�f�B�t�F�[�Y�e�N�X�`���p�̃m�[�}���}�b�v�����g�p���Ȃ��̂�
			//�擾�ς݂̃f�B�t�F�[�Y�e�N�X�`��UV���Ɠ���UV���̃m�[�}���}�b�v��T��
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
			//uv�t�]
			if (!strcmp(difName, uv1Name) &&
				(!strcmp(speName, "") || !strcmp(speName, uv0Name)))
				uvSw[i] = 1;
			//�ǂ����uv0
			if (!strcmp(difName, uv0Name) &&
				(!strcmp(speName, "") || !strcmp(speName, uv0Name)))
				uvSw[i] = 2;
			//�ǂ����uv1
			if (!strcmp(difName, uv1Name) &&
				(!strcmp(speName, "") || !strcmp(speName, uv1Name)))
				uvSw[i] = 3;
		}

		CONSTANT_BUFFER2 sg = {};
		//�g�U���ˌ�
		CoordTf::VECTOR4* diffuse = &mObj[m].dpara.material[i].diffuse;
		diffuse->x = (float)mesh->getDiffuseColor(0, 0) + addDiffuse;
		diffuse->y = (float)mesh->getDiffuseColor(0, 1) + addDiffuse;
		diffuse->z = (float)mesh->getDiffuseColor(0, 2) + addDiffuse;
		diffuse->w = 0.0f;//�g�p���ĂȂ�
		//�X�y�L�����[
		CoordTf::VECTOR4* specular = &mObj[m].dpara.material[i].specular;
		specular->x = (float)mesh->getSpecularColor(0, 0) + addSpecular;
		specular->y = (float)mesh->getSpecularColor(0, 1) + addSpecular;
		specular->z = (float)mesh->getSpecularColor(0, 2) + addSpecular;
		specular->w = 0.0f;//�g�p���ĂȂ�
		//�A���r�G���g
		CoordTf::VECTOR4* ambient = &mObj[m].dpara.material[i].ambient;
		ambient->x = (float)mesh->getAmbientColor(0, 0) + addAmbient;
		ambient->y = (float)mesh->getAmbientColor(0, 1) + addAmbient;
		ambient->z = (float)mesh->getAmbientColor(0, 2) + addAmbient;
		ambient->w = 0.0f;//�g�p���ĂȂ�

		sg.vDiffuse = *diffuse;//�f�B�t���[�Y�J���[���V�F�[�_�[�ɓn��
		sg.vSpeculer = *specular;//�X�y�L�����[���V�F�[�_�[�ɓn��
		sg.vAmbient = *ambient;//�A���r�G���g���V�F�[�_�[�ɓn��
		mObj[m].mObjectCB1->CopyData(i, sg);
		if (dx->DXR_CreateResource) {
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

static void swaptex(MY_VERTEX_S* v, int uvSw) {
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

void SkinMesh::swapTex(MY_VERTEX_S* vb, FbxMeshNode* mesh, int* uvSw) {
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
	mObj[meshIndex].dpara.material[materialIndex].diftex_no = mObj[0].dx->GetTexNumber(textureName);
}

void SkinMesh::SetNormalTextureName(char* textureName, int materialIndex, int meshIndex) {
	mObj[meshIndex].dpara.material[materialIndex].nortex_no = mObj[0].dx->GetTexNumber(textureName);
}

void SkinMesh::SetSpeculerTextureName(char* textureName, int materialIndex, int meshIndex) {
	mObj[meshIndex].dpara.material[materialIndex].spetex_no = mObj[0].dx->GetTexNumber(textureName);
}

void SkinMesh::GetShaderByteCode(bool disp) {
	Dx12Process* dx = mObj[0].dx;
	Dx_ShaderHolder* sh = dx->shaderH.get();
	for (int i = 0; i < numMesh; i++) {
		PolygonData& o = mObj[i];
		if (disp) {
			if (numBone > 0)
				o.vs = sh->pVertexShader_SKIN_D.Get();
			else
				o.vs = sh->pVertexShader_MESH_D.Get();
			o.hs = sh->pHullShaderTriangle.Get();
			o.ds = sh->pDomainShaderTriangle.Get();
			o.gs = sh->pGeometryShader_Before_ds.Get();
			o.gs_NoMap = sh->pGeometryShader_Before_ds_NoNormalMap.Get();
			o.primType_create = CONTROL_POINT;
			o.dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		}
		else {
			if (numBone > 0)
				o.vs = sh->pVertexShader_SKIN.Get();
			else
				o.vs = sh->pVertexShader_MESH.Get();
			o.gs = sh->pGeometryShader_Before_vs.Get();
			o.gs_NoMap = sh->pGeometryShader_Before_vs_NoNormalMap.Get();
			o.primType_create = SQUARE;
			o.dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
		o.ps = sh->pPixelShader_3D.Get();
		o.ps_NoMap = sh->pPixelShader_3D_NoNormalMap.Get();
	}
}

bool SkinMesh::CreateFromFBX(bool disp, float divideBufferMagnification) {
	GetShaderByteCode(disp);
	const int numSrvTex = 3;
	const int numCbv = 3;

	for (int i = 0; i < numMesh; i++) {
		PolygonData& o = mObj[i];
		o.setDivideArr(divArr, numDiv);
		o.com_no = com_no;

		UINT* indexCntArr = new UINT[mObj[i].dpara.NumMaterial];
		for (int m = 0; m < mObj[i].dpara.NumMaterial; m++) {
			indexCntArr[m] = mObj[i].dpara.Iview[m].IndexCount;
		}
		int vSize = sizeof(VertexM);
		void* vertex = pvVBM[i];
		if (numBone > 0) {
			vSize = sizeof(MY_VERTEX_S);
			vertex = pvVB[i];
		}
		Dx_Util::createTangent(mObj[i].dpara.NumMaterial, indexCntArr,
			vertex, newIndex[i], vSize, 0, 12 * 4, 6 * 4);
		ARR_DELETE(indexCntArr);

		o.createDefaultBuffer(vertex, newIndex[i]);
		if (pvVB_delete_f) {
			ARR_DELETE(pvVBM[i]);
			ARR_DELETE(pvVB[i]);
		}
		for (int m = 0; m < mObj[i].dpara.NumMaterial; m++) {
			ARR_DELETE(newIndex[i][m]);
		}
		ARR_DELETE(newIndex[i]);
		int numUav = 0;
		Dx12Process* dx = o.dx;
		o.createParameterDXR(alpha, blend, divideBufferMagnification);
		if (numBone > 0) {
			if (!sk[i].createParameterDXR())return false;
			if (!o.createPSO(dx->shaderH->pVertexLayout_SKIN, numSrvTex, numCbv, numUav, blend, alpha))return false;
			if (!o.createPSO_DXR(dx->shaderH->pVertexLayout_SKIN, numSrvTex, numCbv, numUav))return false;
			if (!sk[i].createPSO())return false;
			UINT cbSize = mObject_BONES->getSizeInBytes();
			D3D12_GPU_VIRTUAL_ADDRESS ad = mObject_BONES->Resource()->GetGPUVirtualAddress();
			if (!o.setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, ad, cbSize))return false;
			if (!sk[i].createDescHeap(ad, cbSize))return false;
		}
		else {
			if (!o.createPSO(dx->shaderH->pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;
			if (!o.createPSO_DXR(dx->shaderH->pVertexLayout_MESH, numSrvTex, numCbv, numUav))return false;
			if (!o.setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, 0, 0))return false;
		}
	}
	if (pvVB_delete_f) {
		ARR_DELETE(pvVBM);
		ARR_DELETE(pvVB);
	}
	ARR_DELETE(newIndex);
	return true;
}

bool SkinMesh::CreateFromFBX() {
	return CreateFromFBX(false);
}

HRESULT SkinMesh::GetFbxSub(CHAR* szFileName, int ind) {
	if (ind <= 0) {
		MessageBox(0, L"FBX���[�_�[���������s", nullptr, MB_OK);
		return E_FAIL;
	}

	if (FAILED(InitFBX(szFileName, ind))) {
		MessageBox(0, L"FBX���[�_�[���������s", nullptr, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}

HRESULT SkinMesh::GetBuffer_Sub(int ind, float end_frame) {
	fbx[ind].end_frame = end_frame;

	int BoneNum = fbx[ind].fbxL->getNumNoneMeshDeformer();
	if (BoneNum == 0) {
		MessageBox(0, L"FBX���[�_�[���������s", nullptr, MB_OK);
		return E_FAIL;
	}

	if (!m_ppSubAnimationBone) {
		m_ppSubAnimationBone = new Deformer * [(FBX_PCS - 1) * numBone];
	}
	return S_OK;
}

void SkinMesh::CreateFromFBX_SubAnimation(int ind) {
	int loopind = 0;
	int searchCount = 0;
	int name2Num = 0;
	while (loopind < numBone) {
		int sa_ind = (ind - 1) * numBone + loopind;
		m_ppSubAnimationBone[sa_ind] = fbx[ind].fbxL->getNoneMeshDeformer(searchCount);
		searchCount++;
		const char* name = m_ppSubAnimationBone[sa_ind]->getName();
		if (!strncmp("Skeleton", name, 8))continue;
		char* name2 = &boneName[loopind * 255];//�eBone���̐擪�A�h���X��n��
		//Bone���ɋ󔒂��܂܂�Ă���ꍇ�Ō�̋󔒈ȍ~�̕�������I�[�܂ł̕������r�����,
		//�I�[�����܂Ń|�C���^��i��, �I�[���猟�����ċ󔒈ʒu��O�܂Ŕ�r����
		while (*name != '\0')name++;//�I�[�����܂Ń|�C���^��i�߂�
		//�I�[�����܂Ń|�C���^��i�߂�, �󔒂��܂܂�Ȃ������̏ꍇ������̂ŕ������J�E���g��,
		//�������Ŕ�r�����𔻒f����
		while (*name2 != '\0') { name2++; name2Num++; }
		while (*(--name) == *(--name2) && *name2 != ' ' && (--name2Num) > 0);
		if (*name2 != ' ' && name2Num > 0) { name2Num = 0; continue; }
		name2Num = 0;
		loopind++;
	}
}

//�{�[�������̃|�[�Y�ʒu�ɃZ�b�g����
bool SkinMesh::SetNewPoseMatrices(float ti, int ind) {
	if (numBone <= 0)return true;
	if (AnimLastInd == -1)AnimLastInd = ind;//�ŏ��ɕ`�悷��A�j���[�V�����ԍ��ŏ�����
	bool ind_change = false;
	if (AnimLastInd != ind) {//�A�j���[�V�������؂�ւ����
		ind_change = true; AnimLastInd = ind;
		fbx[ind].current_frame = 0.0f;//�A�j���[�V�������؂�ւ���Ă�̂�Matrix�X�V�O�Ƀt���[����0�ɏ�����
	}
	bool frame_end = false;
	fbx[ind].current_frame += ti;
	if (fbx[ind].end_frame <= fbx[ind].current_frame) frame_end = true;

	if (frame_end || ind_change) {
		for (int i = 0; i < numBone; i++) {
			for (int x = 0; x < 4; x++) {
				for (int y = 0; y < 4; y++) {
					m_pLastBoneMatrix[i].m[y][x] = m_BoneArray[i].mNewPose.m[y][x];
				}
			}
		}
		BoneConnect = 0.1f;
	}

	if (BoneConnect != -1.0f)fbx[ind].current_frame = 0.0f;

	int frame = (int)fbx[ind].current_frame;
	Deformer Time;
	int64_t time = Time.getTimeFRAMES60(frame / 10);

	bool subanm = true;
	if (ind <= 0 || ind > FBX_PCS - 1)subanm = false;

	Deformer* defo = nullptr;
	FbxLoader* fbL = fbx[0].fbxL;
	FbxMeshNode* mesh = fbL->getFbxMeshNode(0);
	if (!subanm) {
		defo = mesh->getDeformer(0);
	}
	else {
		defo = m_ppSubAnimationBone[(ind - 1) * numBone];
	}
	defo->EvaluateGlobalTransform(time, InternalAnimationIndex);

	using namespace CoordTf;
	MATRIX mat0_mov;
	MatrixIdentity(&mat0_mov);
	if (fbx[ind].offset) {
		MatrixTranslation(&mat0_mov, fbx[ind].cx, fbx[ind].cy, fbx[ind].cz);
	}
	else {
		MatrixTranslation(&mat0_mov, (float)-defo->getEvaluateGlobalTransform(3, 0) + fbx[ind].cx,
			(float)-defo->getEvaluateGlobalTransform(3, 1) + fbx[ind].cy,
			(float)-defo->getEvaluateGlobalTransform(3, 2) + fbx[ind].cz);
	}

	MATRIX pose;
	for (int i = 0; i < numBone; i++) {
		Deformer* de = nullptr;
		if (!subanm) {
			de = mesh->getDeformer(i);
		}
		else {
			de = m_ppSubAnimationBone[(ind - 1) * numBone + i];
		}
		de->EvaluateGlobalTransform(time, InternalAnimationIndex);

		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				if (fbx[ind].centering) {
					pose.m[y][x] = (float)de->getEvaluateGlobalTransform(y, x);
				}
				else {
					m_BoneArray[i].mNewPose.m[y][x] = (float)de->getEvaluateGlobalTransform(y, x);
				}
			}
		}

		if (fbx[ind].centering) {
			MATRIX tmp;
			MatrixMultiply(&tmp, &fbx[ind].rotZYX, &mat0_mov);
			MatrixMultiply(&m_BoneArray[i].mNewPose, &pose, &tmp);
		}
	}

	if (frame_end)fbx[ind].current_frame = 0.0f;

	if (BoneConnect != -1.0f) {
		if (fbx[ind].connect_step <= 0.0f || BoneConnect > 1.0f)BoneConnect = -1.0f;
		else {
			for (int i = 0; i < numBone; i++) {
				StraightLinear(&m_BoneArray[i].mNewPose, &m_pLastBoneMatrix[i], &m_BoneArray[i].mNewPose, BoneConnect += (ti / fbx[ind].connect_step));
			}
		}
	}
	return frame_end;
}

//�|�[�Y�s���Ԃ�
CoordTf::MATRIX SkinMesh::GetCurrentPoseMatrix(int index) {
	using namespace CoordTf;
	MATRIX inv;
	MatrixIdentity(&inv);
	MatrixInverse(&inv, &m_BoneArray[index].mBindPose);//FBX�̃o�C���h�|�[�Y�͏����p���i��΍��W�j
	MATRIX ret;
	MatrixIdentity(&ret);
	MatrixMultiply(&ret, &inv, &m_BoneArray[index].mNewPose);//�o�C���h�|�[�Y�̋t�s��ƃt���[���p���s���������
	return ret;
}

void SkinMesh::GetMeshCenterPos() {
	using namespace CoordTf;
	for (int i = 0; i < numMesh; i++) {
		if (mObj[i].dx->DXR_CreateResource) {
			VECTOR3 cp = centerPos[i].pos;
			UpdateDXR& ud = mObj[i].dxrPara.updateDXR[mObj[i].dx->dxrBuffSwap[0]];
			VECTOR3* vv = &ud.v[0];
			vv->as(cp.x, cp.y, cp.z);
			if (numBone > 0) {
				float w = centerPos[i].bBoneWeight;
				UINT ind = centerPos[i].bBoneIndex;
				MATRIX m = GetCurrentPoseMatrix(ind);
				VectorMatrixMultiply(vv, &m);
				VectorMultiply(vv, w);
			}
		}
	}
}

CoordTf::VECTOR3 SkinMesh::GetVertexPosition(int meshIndex, int verNum, float adjustZ, float adjustY, float adjustX,
	float thetaZ, float thetaY, float thetaX, float scal) {

	using namespace CoordTf;
	//���_�Ƀ{�[���s����|���o��
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

void SkinMesh::MatrixMap_Bone(SHADER_GLOBAL_BONES* sgb) {

	using namespace CoordTf;
	for (int i = 0; i < numBone; i++)
	{
		MATRIX mat = GetCurrentPoseMatrix(i);
		MatrixTranspose(&mat);
		sgb->mBone[i] = mat;
	}
	GetMeshCenterPos();
}

bool SkinMesh::Update(int ind, float ti, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
	CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
	float disp, float shininess) {

	bool frame_end = false;
	int insnum = 0;
	if (ti != -1.0f)frame_end = SetNewPoseMatrices(ti, ind);
	MatrixMap_Bone(&sgb[mObj[0].dx->cBuffSwap[0]]);

	for (int i = 0; i < numMesh; i++)
		mObj[i].Update(pos, Color, angle, size, disp, shininess);

	return frame_end;
}

void SkinMesh::DrawOff() {
	for (int i = 0; i < numMesh; i++)
		mObj[i].DrawOff();
}

void SkinMesh::Draw(int com) {

	if (mObject_BONES)mObject_BONES->CopyData(0, sgb[mObj[0].dx->cBuffSwap[1]]);

	for (int i = 0; i < numMesh; i++)
		mObj[i].Draw(com);
}

void SkinMesh::StreamOutput(int com) {

	if (mObject_BONES)mObject_BONES->CopyData(0, sgb[mObj[0].dx->cBuffSwap[1]]);

	for (int i = 0; i < numMesh; i++) {
		mObj[i].StreamOutput(com);
		sk[i].Skinning(com);
	}
}

void SkinMesh::Draw() {
	Draw(com_no);
}

void SkinMesh::StreamOutput() {
	StreamOutput(com_no);
}

void SkinMesh::SetCommandList(int no) {
	com_no = no;
	if (mObj) {
		for (int i = 0; i < numMesh; i++) {
			mObj[i].SetCommandList(com_no);
		}
	}
}

void SkinMesh::CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int texIndex, int meshIndex) {
	mObj[meshIndex].CopyResource(texture, res, texIndex);
}

void SkinMesh::TextureInit(int width, int height, int texIndex, int meshIndex) {
	mObj[meshIndex].TextureInit(width, height, texIndex);
}

HRESULT SkinMesh::SetTextureMPixel(BYTE* frame, int texIndex, int meshIndex) {
	return mObj[meshIndex].SetTextureMPixel(frame, texIndex);
}