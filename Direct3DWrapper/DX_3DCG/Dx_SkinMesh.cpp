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
	divArr[0].divide = 2;
	divArr[1].distance = 500.0f;
	divArr[1].divide = 5;
	divArr[2].distance = 300.0f;
	divArr[2].divide = 10;
}

SkinMesh::~SkinMesh() {
	if (pvVB) {
		for (int i = 0; i < numMesh; i++) {
			ARR_DELETE(pvVB[i]);
		}
	}
	ARR_DELETE(pvVB);
	ARR_DELETE(boneName);
	ARR_DELETE(m_ppSubAnimationBone);
	ARR_DELETE(m_pLastBoneMatrix);
	ARR_DELETE(m_BoneArray);
	ARR_DELETE(mObj);
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

void SkinMesh::ReadSkinInfo(FbxMeshNode* mesh, MY_VERTEX_S* pvVB) {

	//�eBone�̃E�G�C�g,�C���f�b�N�X�𒲂ג��_�z��ɉ�����
	for (int i = 0; i < numBone; i++) {
		Deformer* defo = mesh->getDeformer(i);
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
					pvVB[index].bBoneWeight[m] = (float)weight;//�E�G�C�g�o�^
					break;
				}
			}
		}
	}

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

void SkinMesh::GetBuffer(float end_frame) {

	fbx[0].end_frame = end_frame;
	FbxLoader* fbL = fbx[0].fbxL;
	numMesh = fbL->getNumFbxMeshNode();
	newIndex = new UINT * *[numMesh];

	mObject_BONES = new ConstantBuffer<SHADER_GLOBAL_BONES>(1);

	FbxMeshNode* mesh = fbL->getFbxMeshNode(0);//�{�[���͂ǂ̃��b�V���ł��擾�ł���悤��
	numBone = mesh->getNumDeformer();
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

	pvVB = new MY_VERTEX_S * [numMesh];
	mObj = new PolygonData[numMesh];
	for (int i = 0; i < numMesh; i++) {
		mObj[i].SetCommandList(com_no);
		mObj[i].getBuffer(fbL->getFbxMeshNode(i)->getNumMaterial());
	}
}

void SkinMesh::SetVertex() {

	FbxLoader* fbL = fbx[0].fbxL;

	for (int m = 0; m < numMesh; m++) {

		FbxMeshNode* mesh = fbL->getFbxMeshNode(m);//���b�V�����ɏ�������

		MY_VERTEX_S* tmpVB = new MY_VERTEX_S[mesh->getNumVertices()];
		//�{�[���E�G�C�g
		ReadSkinInfo(mesh, tmpVB);

		auto index = mesh->getPolygonVertices();//���_Index�擾(���_xyz�ɑ΂��Ă�Index)
		auto ver = mesh->getVertices();//���_�擾
		auto nor = mesh->getNormal(0);//�@���擾

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

		MY_VERTEX_S* vb = pvVB[m];
		for (UINT i = 0; i < mesh->getNumPolygonVertices(); i++) {
			//index���Œ��_�𐮗񂵂Ȃ��璸�_�i�[
			MY_VERTEX_S* v = &vb[i];
			v->vPos.x = (float)ver[index[i] * 3];
			v->vPos.y = (float)ver[index[i] * 3 + 1];
			v->vPos.z = (float)ver[index[i] * 3 + 2];
			v->vNorm.x = (float)nor[i * 3];
			v->vNorm.y = (float)nor[i * 3 + 1];
			v->vNorm.z = (float)nor[i * 3 + 2];
			v->vGeoNorm.x = (float)nor[i * 3];
			v->vGeoNorm.y = (float)nor[i * 3 + 1];
			v->vGeoNorm.z = (float)nor[i * 3 + 2];
			v->vTex0.x = (float)uv0[i * 2];
			v->vTex0.y = 1.0f - (float)uv0[i * 2 + 1];//(1.0f-UV)
			v->vTex1.x = (float)uv1[i * 2];
			v->vTex1.y = 1.0f - (float)uv1[i * 2 + 1];//(1.0f-UV)
			svList[index[i]].Push(i);
			for (int bi = 0; bi < 4; bi++) {
				//ReadSkinInfo(tmpVB)�œǂݍ��񂾊e�p�����[�^�R�s�[
				v->bBoneIndex[bi] = tmpVB[index[i]].bBoneIndex[bi];
				v->bBoneWeight[bi] = tmpVB[index[i]].bBoneWeight[bi];
			}
		}
		ARR_DELETE(tmpVB);

		//������W���_�̖@�����ꉻ(�e�Z���[�V�����p)
		for (UINT i = 0; i < mesh->getNumVertices(); i++) {
			int indVB = 0;
			VECTOR3 geo[50];
			int indVb[50];
			int indGeo = 0;
			while (1) {
				indVB = svList[i].Pop();
				if (indVB == -1)break;
				indVb[indGeo] = indVB;
				geo[indGeo++] = vb[indVB].vGeoNorm;
			}
			VECTOR3 sum;
			sum.as(0.0f, 0.0f, 0.0f);
			for (int i1 = 0; i1 < indGeo; i1++) {
				sum.x += geo[i1].x;
				sum.y += geo[i1].y;
				sum.z += geo[i1].z;
			}
			VECTOR3 ave;
			ave.x = sum.x / (float)indGeo;
			ave.y = sum.y / (float)indGeo;
			ave.z = sum.z / (float)indGeo;
			for (int i1 = 0; i1 < indGeo; i1++) {
				vb[indVb[i1]].vGeoNorm = ave;
			}
		}

		//���_�o�b�t�@
		auto numMaterial = mesh->getNumMaterial();
		mObj[m].getVertexBuffer(sizeof(MY_VERTEX_S), mesh->getNumPolygonVertices());

		ARR_DELETE(svList);

		//4���_�|���S���������Index���J�E���g
		UINT* numNewIndex = new UINT[numMaterial];
		memset(numNewIndex, 0, sizeof(UINT) * numMaterial);
		UINT currentMatNo = -1;
		for (UINT i = 0; i < mesh->getNumPolygon(); i++) {
			if (mesh->getMaterialNoOfPolygon(i) != currentMatNo) {
				currentMatNo = mesh->getMaterialNoOfPolygon(i);
			}
			if (mesh->getPolygonSize(i) == 3) {
				numNewIndex[currentMatNo] += 3;
			}
			if (mesh->getPolygonSize(i) == 4) {
				numNewIndex[currentMatNo] += 6;
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
		currentMatNo = -1;
		int Icnt = 0;
		for (UINT i = 0; i < mesh->getNumPolygon(); i++) {
			if (mesh->getMaterialNoOfPolygon(i) != currentMatNo) {
				currentMatNo = mesh->getMaterialNoOfPolygon(i);
			}

			if (mesh->getPolygonSize(i) == 3) {

				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 1;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 2;

				Icnt += 3;
			}
			if (mesh->getPolygonSize(i) == 4) {

				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 1;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 2;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 2;
				newIndex[m][currentMatNo][indexCnt[currentMatNo]++] = Icnt + 3;

				Icnt += 4;
			}
		}

		//�C���f�b�N�X�o�b�t�@
		for (UINT i = 0; i < numMaterial; i++) {
			const UINT indexSize = numNewIndex[i] * sizeof(UINT);
			mObj[m].getIndexBuffer(i, indexSize, numNewIndex[i]);
		}
		ARR_DELETE(numNewIndex);
		Dx12Process* dx = mObj[0].dx;
		for (UINT i = 0; i < numMaterial; i++) {
			//�f�B�t�F�[�Y�e�N�X�`��Id�擾
			for (int tNo = 0; tNo < mesh->getNumDiffuseTexture(i); tNo++) {
				textureType type = mesh->getDiffuseTextureType(i, tNo);
				if (type.DiffuseColor && !type.SpecularColor ||
					mesh->getNumDiffuseTexture(i) == 1) {
					auto diffName = dx->GetNameFromPass(mesh->getDiffuseTextureName(i, tNo));
					mObj[m].dpara.material[i].diftex_no = dx->GetTexNumber(diffName);
					auto str = mesh->getDiffuseTextureUVName(i, tNo);
					strcpy(mObj[m].dpara.material[i].difUvName, str);
					break;
				}
			}
			//�X�y�L�����e�N�X�`��Id�擾
			for (int tNo = 0; tNo < mesh->getNumDiffuseTexture(i); tNo++) {
				textureType type = mesh->getDiffuseTextureType(i, tNo);
				if (type.SpecularColor) {
					auto speName = dx->GetNameFromPass(mesh->getDiffuseTextureName(i, tNo));
					mObj[m].dpara.material[i].spetex_no = dx->GetTexNumber(speName);
					auto str = mesh->getDiffuseTextureUVName(i, tNo);
					strcpy(mObj[m].dpara.material[i].speUvName, str);
					break;
				}
			}
			//�m�[�}���e�N�X�`��Id�擾
			for (int tNo = 0; tNo < mesh->getNumNormalTexture(i); tNo++) {
				//�f�B�t�F�[�Y�e�N�X�`���p�̃m�[�}���}�b�v�����g�p���Ȃ��̂�
				//�擾�ς݂̃f�B�t�F�[�Y�e�N�X�`��UV���Ɠ���UV���̃m�[�}���}�b�v��T��
				if (!strcmp(mObj[m].dpara.material[i].difUvName, mesh->getNormalTextureUVName(i, tNo)) ||
					mesh->getNumNormalTexture(i) == 1) {
					auto norName = dx->GetNameFromPass(mesh->getNormalTextureName(i, tNo));
					mObj[m].dpara.material[i].nortex_no = dx->GetTexNumber(norName);
					auto str = mesh->getNormalTextureUVName(i, tNo);
					strcpy(mObj[m].dpara.material[i].norUvName, str);
					break;
				}
			}
			float uvSw = 0.0f;
			if (uv0Name != nullptr) {
				char* difName = mObj[m].dpara.material[i].difUvName;
				char* speName = mObj[m].dpara.material[i].speUvName;
				//uv�t�]
				if (!strcmp(difName, uv1Name) &&
					(!strcmp(speName, "") || !strcmp(speName, uv0Name)))
					uvSw = 1.0f;
				//�ǂ����uv0
				if (!strcmp(difName, uv0Name) &&
					(!strcmp(speName, "") || !strcmp(speName, uv0Name)))
					uvSw = 2.0f;
				//�ǂ����uv1
				if (!strcmp(difName, uv1Name) &&
					(!strcmp(speName, "") || !strcmp(speName, uv1Name)))
					uvSw = 3.0f;
			}

			CONSTANT_BUFFER2 sg;
			//�g�U���ˌ�
			VECTOR4* diffuse = &mObj[m].dpara.material[i].diffuse;
			diffuse->x = (float)mesh->getDiffuseColor(0, 0) + addDiffuse;
			diffuse->y = (float)mesh->getDiffuseColor(0, 1) + addDiffuse;
			diffuse->z = (float)mesh->getDiffuseColor(0, 2) + addDiffuse;
			diffuse->w = 0.0f;//�g�p���ĂȂ�
			//�X�y�L�����[
			VECTOR4* specular = &mObj[m].dpara.material[i].specular;
			specular->x = (float)mesh->getSpecularColor(0, 0) + addSpecular;
			specular->y = (float)mesh->getSpecularColor(0, 1) + addSpecular;
			specular->z = (float)mesh->getSpecularColor(0, 2) + addSpecular;
			specular->w = 0.0f;//�g�p���ĂȂ�
			//�A���r�G���g
			VECTOR4* ambient = &mObj[m].dpara.material[i].ambient;
			ambient->x = (float)mesh->getAmbientColor(0, 0) + addAmbient;
			ambient->y = (float)mesh->getAmbientColor(0, 1) + addAmbient;
			ambient->z = (float)mesh->getAmbientColor(0, 2) + addAmbient;
			ambient->w = 0.0f;//�g�p���ĂȂ�

			sg.vDiffuse = *diffuse;//�f�B�t���[�Y�J���[���V�F�[�_�[�ɓn��
			sg.vSpeculer = *specular;//�X�y�L�����[���V�F�[�_�[�ɓn��
			sg.vAmbient = *ambient;//�A���r�G���g���V�F�[�_�[�ɓn��
			sg.uvSwitch.x = uvSw;
			mObj[m].mObjectCB1->CopyData(i, sg);
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
	if (disp) {
		vs = dx->pVertexShader_SKIN_D.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		gs_NoMap = dx->pGeometryShader_Before_ds_NoNormalMap.Get();
		primType_create = CONTROL_POINT;
		for (int i = 0; i < numMesh; i++)
			mObj[i].dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}
	else {
		vs = dx->pVertexShader_SKIN.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		gs_NoMap = dx->pGeometryShader_Before_vs_NoNormalMap.Get();
		primType_create = SQUARE;
		for (int i = 0; i < numMesh; i++)
			mObj[i].dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	ps = dx->pPixelShader_3D.Get();
	ps_NoMap = dx->pPixelShader_3D_NoNormalMap.Get();
}

bool SkinMesh::CreateFromFBX(bool disp) {
	GetShaderByteCode(disp);
	const int numSrvTex = 3;
	const int numCbv = 3;
	for (int i = 0; i < numMesh; i++) {
		mObj[i].com_no = com_no;
		mObj[i].primType_create = primType_create;
		mObj[i].vs = vs;
		mObj[i].hs = hs;
		mObj[i].ds = ds;
		mObj[i].ps = ps;
		mObj[i].ps_NoMap = ps_NoMap;
		mObj[i].gs = gs;
		mObj[i].gs_NoMap = gs_NoMap;
		mObj[i].createDefaultBuffer(pvVB[i], newIndex[i], pvVB_delete_f);
		if (!mObj[i].createPSO(mObj[0].dx->pVertexLayout_SKIN, numSrvTex, numCbv, blend, alpha))return false;
		UINT cbSize = mObject_BONES->getSizeInBytes();
		D3D12_GPU_VIRTUAL_ADDRESS ad = mObject_BONES->Resource()->GetGPUVirtualAddress();
		if (!mObj[i].setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, ad, cbSize))return false;
	}
	if (pvVB_delete_f)ARR_DELETE(pvVB);
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
	defo->EvaluateGlobalTransform(time);

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
		de->EvaluateGlobalTransform(time);

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
MATRIX SkinMesh::GetCurrentPoseMatrix(int index) {
	MATRIX inv;
	MatrixIdentity(&inv);
	MatrixInverse(&inv, &m_BoneArray[index].mBindPose);//FBX�̃o�C���h�|�[�Y�͏����p���i��΍��W�j
	MATRIX ret;
	MatrixIdentity(&ret);
	MatrixMultiply(&ret, &inv, &m_BoneArray[index].mNewPose);//�o�C���h�|�[�Y�̋t�s��ƃt���[���p���s���������
	return ret;
}

VECTOR3 SkinMesh::GetVertexPosition(int meshIndex, int verNum, float adjustZ, float adjustY, float adjustX,
	float thetaZ, float thetaY, float thetaX, float scal) {

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

	for (int i = 0; i < numBone; i++)
	{
		MATRIX mat = GetCurrentPoseMatrix(i);
		MatrixTranspose(&mat);
		sgb->mBone[i] = mat;
	}
}

bool SkinMesh::Update(float time, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size) {
	return Update(0, time, x, y, z, r, g, b, a, thetaZ, thetaY, thetaX, size);
}

bool SkinMesh::Update(int ind, float ti, float x, float y, float z, float r, float g, float b, float a,
	float thetaZ, float thetaY, float thetaX, float size, float disp, float shininess) {

	bool frame_end = false;
	int insnum = 0;
	if (ti != -1.0f)frame_end = SetNewPoseMatrices(ti, ind);
	MatrixMap_Bone(&sgb[mObj[0].dx->cBuffSwap[0]]);

	for (int i = 0; i < numMesh; i++)
		mObj[i].update(x, y, z, r, g, b, a, thetaZ, thetaY, thetaX, size, divArr, numDiv, disp, shininess);

	return frame_end;
}

void SkinMesh::DrawOff() {
	for (int i = 0; i < numMesh; i++)
		mObj[i].DrawOff();
}

void SkinMesh::Draw() {

	mObject_BONES->CopyData(0, sgb[mObj[0].dx->cBuffSwap[1]]);

	for (int i = 0; i < numMesh; i++) {
		mObj[i].com_no = com_no;
		mObj[i].Draw();
	}
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