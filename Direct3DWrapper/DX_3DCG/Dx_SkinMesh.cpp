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
mutex SkinMesh::mtx;

SkinMesh_sub::SkinMesh_sub() {
	fbxL = new FbxLoader();
	current_frame = 0.0f;
	centering = true;
	offset = false;
	cx = cy = cz = 0.0f;
	connect_step = 3000.0f;
}

SkinMesh_sub::~SkinMesh_sub() {
	S_DELETE(fbxL);
}

bool SkinMesh_sub::Create(CHAR *szFileName) {
	return fbxL->setFbxFile(szFileName);
}

void SkinMesh::CreateManager() {}

void SkinMesh::DeleteManager() {}

SkinMesh::SkinMesh() {
	ZeroMemory(this, sizeof(SkinMesh));
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	fbx = new SkinMesh_sub[FBX_PCS];
	m_ppSubAnimationBone = nullptr;
	m_pClusterName = nullptr;
	AnimLastInd = -1;
	BoneConnect = -1.0f;
	pvVB_delete_f = true;
	pvVB = nullptr;
	texNum = 0;
	addDiffuse = 0.0f;
	addSpecular = 0.0f;
	addAmbient = 0.0f;
}

SkinMesh::~SkinMesh() {
	ARR_DELETE(pvVB);
	ARR_DELETE(m_pClusterName);
	ARR_DELETE(m_ppSubAnimationBone);
	ARR_DELETE(m_pLastBoneMatrix);
	ARR_DELETE(m_pdwNumVert);
	ARR_DELETE(m_BoneArray);
	ARR_DELETE(m_ppCluster);
	ARR_DELETE(m_pMaterial);

	S_DELETE(mObjectCB0);
	S_DELETE(mObjectCB1);
	S_DELETE(mObject_BONES);
	RELEASE(texture);
	RELEASE(textureUp);

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

void SkinMesh::ReadSkinInfo(MY_VERTEX_S* pvVB) {
	FbxLoader* fbl = fbx[0].fbxL;
	FbxMeshNode* mesh = fbl->getFbxMeshNode(0);//�s��֌W�͂ǂ̃��b�V������ł��������𓾂���

	for (int i = 0; i < m_iNumBone; i++) {
		m_ppCluster[i] = mesh->getDeformer(i);
		const char* name = m_ppCluster[i]->getName();
		strcpy(&m_pClusterName[i * 255], name);//�{�[���̖��O�ێ�
	}

	int VertexStart = 0;
	for (int m = 0; m < NodeArraypcs; m++) {
		FbxMeshNode* meshBone = fbl->getFbxMeshNode(m);

		//�eBone�̃E�G�C�g,�C���f�b�N�X�𒲂ג��_�z��ɉ�����
		for (int i = 0; i < m_iNumBone; i++) {
			Deformer* defo = meshBone->getDeformer(i);
			int iNumIndex = defo->getIndicesCount();//���̃{�[���ɉe�����󂯂钸�_�C���f�b�N�X��
			int* piIndex = defo->getIndices();     //���̃{�[���ɉe�����󂯂钸�_�̃C���f�b�N�X�z��
			double* pdWeight = defo->getWeights();//���̃{�[���ɉe�����󂯂钸�_�̃E�G�C�g�z��

			for (int k = 0; k < iNumIndex; k++) {
				int index = piIndex[k];      //�e�����󂯂钸�_
				double weight = pdWeight[k];//�E�G�C�g
				for (int m = 0; m < 4; m++) {
					//�eBone���ɉe�����󂯂钸�_�̃E�G�C�g����ԑ傫�����l�ɍX�V���Ă���
					if (weight > pvVB[index + VertexStart].bBoneWeight[m]) {//���ׂ��E�G�C�g�̕����傫��
						pvVB[index + VertexStart].bBoneIndex[m] = i;//Bone�C���f�b�N�X�o�^
						pvVB[index + VertexStart].bBoneWeight[m] = (float)weight;//�E�G�C�g�o�^
						break;
					}
				}
			}
		}
		VertexStart += m_pdwNumVert[m];
	}

	//�E�G�C�g���K��
	for (DWORD i = 0; i < VerAllpcs; i++) {
		float we = 0;
		for (int j = 0; j < 4; j++) {
			we += pvVB[i].bBoneWeight[j];
		}
		float we1 = 1.0f / we;
		for (int j = 0; j < 4; j++) {
			pvVB[i].bBoneWeight[j] *= we1;
		}
	}

	for (int i = 0; i < m_iNumBone; i++) {
		//�����p���s��ǂݍ���
		//GetCurrentPoseMatrix�Ŏg��
		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				m_BoneArray[i].mBindPose.m[y][x] = (float)m_ppCluster[i]->getTransformLinkMatrix(y, x);
			}
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

	mObjectCB0 = new ConstantBuffer<CONSTANT_BUFFER>(1);
	mObject_BONES = new ConstantBuffer<SHADER_GLOBAL_BONES>(1);

	fbx[0].end_frame = end_frame;
	FbxLoader *fbL = fbx[0].fbxL;
	NodeArraypcs = fbL->getNumFbxMeshNode();

	//���O�Ƀ��b�V�������_���A�|���S�����}�e���A�������𒲂ׂ�
	m_pdwNumVert = new DWORD[NodeArraypcs];
	MateAllpcs = 0;//�}�e���A���S��
	m_pMaterialCount = new DWORD[NodeArraypcs];
	VerAllpcs = 0;//���_�S��
	pdwNumFace = new DWORD[NodeArraypcs];

	//�J�E���g
	for (int i = 0; i < NodeArraypcs; i++) {
		FbxMeshNode *mesh = fbL->getFbxMeshNode(i);
		m_pdwNumVert[i] = mesh->getNumVertices();//���b�V�������_��
		m_pMaterialCount[i] = mesh->getNumMaterial();//���b�V�����}�e���A����
		VerAllpcs += m_pdwNumVert[i];               //���_�S���J�E���g
		MateAllpcs += m_pMaterialCount[i];        //�}�e���A���S���J�E���g
		pdwNumFace[i] = mesh->getNumPolygon();   //���b�V�����|���S����
	}

	//���b�V����4���_�����O�C���f�b�N�X�z�񐶐�
	IndexCount34Me = new int[NodeArraypcs];
	//�}�e���A����4���_������C���f�b�N�X�z�񐶐�
	IndexCount3M = new int[MateAllpcs];

	//�}�e���A���z�񐶐�
	m_pMaterial = new MY_MATERIAL_S[MateAllpcs];

	//�}�e���A���R���X�^���g�o�b�t�@
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(MateAllpcs);

	//VBO
	Vview = std::make_unique<VertexView>();
	//IBO �}�e���A�����ɐ�������
	Iview = std::make_unique<IndexView[]>(MateAllpcs);

	//�C���f�b�N�X���v�Z(�|���S��1�ɕt�����_ 2�`5�����݂��邱�Ƃ��L�邪
	//fbx����Index�z��R�s�[���ɑS�ăR�s�[����̂ł��̂܂܂̌����J�E���g����)
	IndexCount34MeAll = 0;
	for (int k = 0; k < NodeArraypcs; k++) {
		IndexCount34Me[k] = 0;
		FbxMeshNode *mesh = fbL->getFbxMeshNode(k);
		for (DWORD i = 0; i < pdwNumFace[k]; i++) {
			IndexCount34Me[k] += mesh->getPolygonSize(i);//���b�V�����̕����O���_�C���f�b�N�X��
		}
		IndexCount34MeAll += IndexCount34Me[k];
	}

	int mInd = 0;
	for (int k = 0; k < NodeArraypcs; k++) {
		FbxMeshNode *mesh = fbL->getFbxMeshNode(k);
		for (DWORD j = 0; j < m_pMaterialCount[k]; j++) {
			IndexCount3M[mInd] = 0;
			for (DWORD i = 0; i < pdwNumFace[k]; i++) {
				int matId = mesh->getMaterialNoOfPolygon(i);//�|���S��i�̊֘A����}�e���A���ԍ��擾
				if (matId == j) {//���ݏ������̃}�e���A���ƈ�v�����ꍇ�ȉ��������s(1Mesh��2�ȏ�̃}�e���A�����L��ꍇ�̏���)
					int pcs = mesh->getPolygonSize(i);//�e�|���S���̒��_�C���f�b�N�X��
					switch (pcs) {
					case 3:
						IndexCount3M[mInd] += 3;
						break;
					case 4:
						IndexCount3M[mInd] += 6;
						break;
					}
				}
			}
			mInd++;
		}
	}

	pIndex = new int*[MateAllpcs];
	for (int i = 0; i < MateAllpcs; i++) {
		pIndex[i] = new int[IndexCount3M[i]];
	}

	FbxMeshNode *mesh = fbL->getFbxMeshNode(0);
	m_iNumBone = mesh->getNumDeformer();//�ǂ̃��b�V������ł������{�[�����������Ă���悤��
	m_ppCluster = new  Deformer*[m_iNumBone];//�{�[�����z��
	m_pClusterName = new char[m_iNumBone * 255];

	//�{�[���𐶐�
	m_BoneArray = new BONE[m_iNumBone];
	m_pLastBoneMatrix = new MATRIX[m_iNumBone];
}

void SkinMesh::SetVertex() {
	//4���_�����O��Ԃł̃C���f�b�N�X���Œ��_�z�񐶐�
	//4���_�|���S���͕�����@���ω������̈�, ���_��3���_,4���_���ݏ�Ԃ̗v�f���Ő���
	pvVB = new MY_VERTEX_S[IndexCount34MeAll];

	MY_VERTEX_S* tmpVB = new MY_VERTEX_S[VerAllpcs];
	//�X�L�����(�W���C���g, �E�F�C�g)
	ReadSkinInfo(tmpVB);

	//������W���_���X�g
	SameVertexList* svList = new SameVertexList[VerAllpcs];

	FbxLoader* fbL = fbx[0].fbxL;
	//���b�V�����ɔz��i�[����
	int mInd = 0;//�}�e���A�����J�E���g
	DWORD VerArrStart = 0;//�����O�C���f�b�N�X��(�ŏI�I�Ȓ��_��)
	DWORD VerArrStart2 = 0;//�ǂݍ��ݎ����_��
	for (int m = 0; m < NodeArraypcs; m++) {
		FbxMeshNode* mesh = fbL->getFbxMeshNode(m);
		int* piIndex = mesh->getPolygonVertices();//fbx���璸�_�C���f�b�N�X�z��擾

		//���_�z���fbx����R�s�[
		double* pCoord = mesh->getVertices();
		for (int i = 0; i < IndexCount34Me[m]; i++) {
			//fbx����ǂݍ���index���Œ��_�𐮗񂵂Ȃ��璸�_�i�[
			pvVB[i + VerArrStart].vPos.x = (float)pCoord[piIndex[i] * 3 + 0];
			pvVB[i + VerArrStart].vPos.y = (float)pCoord[piIndex[i] * 3 + 1];
			pvVB[i + VerArrStart].vPos.z = (float)pCoord[piIndex[i] * 3 + 2];
			svList[piIndex[i] + VerArrStart2].Push(i + VerArrStart);
			for (int bi = 0; bi < 4; bi++) {
				//ReadSkinInfo(tmpVB)�œǂݍ��񂾊e�p�����[�^�R�s�[
				pvVB[i + VerArrStart].bBoneIndex[bi] = tmpVB[piIndex[i] + VerArrStart2].bBoneIndex[bi];
				pvVB[i + VerArrStart].bBoneWeight[bi] = tmpVB[piIndex[i] + VerArrStart2].bBoneWeight[bi];
			}
		}

		//�@��, �e�N�X�`���[���W�ǂݍ���
		int IndCount = 0;
		int NorCount = 0;
		int UVCount = 0;
		double* Normal = mesh->getNormal();
		double* UV = mesh->getAlignedUV();
		for (DWORD i = 0; i < pdwNumFace[m]; i++) {
			//int iStartIndex = pFbxMesh->GetPolygonVertexIndex(i);//�|���S�����\������ŏ��̃C���f�b�N�X�擾
			int pcs = mesh->getPolygonSize(i);//�e�|���S���̒��_��
			if (pcs != 3 && pcs != 4)continue;//���_��3��,4�ȊO�̓X�L�b�v
			int iVert0Index, iVert1Index, iVert2Index, iVert3Index;

			//�@��,UV�ǂݍ���,index�œǂݍ���
			//���_��index�Ő���ς�
			iVert0Index = IndCount + VerArrStart;
			iVert1Index = IndCount + 1 + VerArrStart;
			iVert2Index = IndCount + 2 + VerArrStart;
			iVert3Index = IndCount + 3 + VerArrStart;

			if (iVert0Index < 0)continue;
			pvVB[iVert0Index].vNorm.x = (float)Normal[NorCount + 0];
			pvVB[iVert0Index].vNorm.y = (float)Normal[NorCount + 1];
			pvVB[iVert0Index].vNorm.z = (float)Normal[NorCount + 2];
			pvVB[iVert0Index].vGeoNorm.x = (float)Normal[NorCount + 0];
			pvVB[iVert0Index].vGeoNorm.y = (float)Normal[NorCount + 1];
			pvVB[iVert0Index].vGeoNorm.z = (float)Normal[NorCount + 2];

			pvVB[iVert0Index].vTex.x = (float)UV[UVCount + 0];
			pvVB[iVert0Index].vTex.y = 1.0f - (float)UV[UVCount + 1];//(1.0f-UV)

			NorCount += 3;
			UVCount += 2;

			pvVB[iVert1Index].vNorm.x = (float)Normal[NorCount + 0];
			pvVB[iVert1Index].vNorm.y = (float)Normal[NorCount + 1];
			pvVB[iVert1Index].vNorm.z = (float)Normal[NorCount + 2];
			pvVB[iVert1Index].vGeoNorm.x = (float)Normal[NorCount + 0];
			pvVB[iVert1Index].vGeoNorm.y = (float)Normal[NorCount + 1];
			pvVB[iVert1Index].vGeoNorm.z = (float)Normal[NorCount + 2];

			pvVB[iVert1Index].vTex.x = (float)UV[UVCount + 0];
			pvVB[iVert1Index].vTex.y = 1.0f - (float)UV[UVCount + 1];

			NorCount += 3;
			UVCount += 2;

			pvVB[iVert2Index].vNorm.x = (float)Normal[NorCount + 0];
			pvVB[iVert2Index].vNorm.y = (float)Normal[NorCount + 1];
			pvVB[iVert2Index].vNorm.z = (float)Normal[NorCount + 2];
			pvVB[iVert2Index].vGeoNorm.x = (float)Normal[NorCount + 0];
			pvVB[iVert2Index].vGeoNorm.y = (float)Normal[NorCount + 1];
			pvVB[iVert2Index].vGeoNorm.z = (float)Normal[NorCount + 2];

			pvVB[iVert2Index].vTex.x = (float)UV[UVCount + 0];
			pvVB[iVert2Index].vTex.y = 1.0f - (float)UV[UVCount + 1];

			NorCount += 3;
			UVCount += 2;

			if (pcs == 4) {
				pvVB[iVert3Index].vNorm.x = (float)Normal[NorCount + 0];
				pvVB[iVert3Index].vNorm.y = (float)Normal[NorCount + 1];
				pvVB[iVert3Index].vNorm.z = (float)Normal[NorCount + 2];
				pvVB[iVert3Index].vGeoNorm.x = (float)Normal[NorCount + 0];
				pvVB[iVert3Index].vGeoNorm.y = (float)Normal[NorCount + 1];
				pvVB[iVert3Index].vGeoNorm.z = (float)Normal[NorCount + 2];

				pvVB[iVert3Index].vTex.x = (float)UV[UVCount + 0];
				pvVB[iVert3Index].vTex.y = 1.0f - (float)UV[UVCount + 1];

				NorCount += 3;
				UVCount += 2;
				IndCount++;
			}
			IndCount += 3;
		}

		for (DWORD i = 0; i < m_pMaterialCount[m]; i++) {
			//�}�e���A�����擾
			//�g�U���ˌ�
			m_pMaterial[mInd].Kd.x = (float)mesh->getDiffuseColor(0, 0);
			m_pMaterial[mInd].Kd.y = (float)mesh->getDiffuseColor(0, 1);
			m_pMaterial[mInd].Kd.z = (float)mesh->getDiffuseColor(0, 2);
			m_pMaterial[mInd].Kd.w = 0.0f;//�g�p���ĂȂ�
			//�X�y�L�����[
			m_pMaterial[mInd].Ks.x = (float)mesh->getSpecularColor(0, 0);
			m_pMaterial[mInd].Ks.y = (float)mesh->getSpecularColor(0, 1);
			m_pMaterial[mInd].Ks.z = (float)mesh->getSpecularColor(0, 2);
			m_pMaterial[mInd].Ks.w = 0.0f;//�g�p���ĂȂ�
			//�A���r�G���g
			m_pMaterial[mInd].Ka.x = (float)mesh->getAmbientColor(0, 0);
			m_pMaterial[mInd].Ka.y = (float)mesh->getAmbientColor(0, 1);
			m_pMaterial[mInd].Ka.z = (float)mesh->getAmbientColor(0, 2);
			m_pMaterial[mInd].Ka.w = 0.0f;//�g�p���ĂȂ�

			//�e�N�X�`���[
			if (mesh->getNormalTextureName(i)) {
				strcpy_s(m_pMaterial[mInd].norTextureName, mesh->getNormalTextureName(i));
				//�t�@�C���������Ɋ��Ƀf�R�[�h�ς݂̃e�N�X�`���ԍ���ǂݍ���
				m_pMaterial[mInd].nortex_no = dx->GetTexNumber(m_pMaterial[mInd].norTextureName);
				texNum++;
			}
			if (mesh->getDiffuseTextureName(i)) {
				strcpy_s(m_pMaterial[mInd].szTextureName, mesh->getDiffuseTextureName(i));
				//�t�@�C���������Ɋ��Ƀf�R�[�h�ς݂̃e�N�X�`���ԍ���ǂݍ���
				m_pMaterial[mInd].tex_no = dx->GetTexNumber(m_pMaterial[mInd].szTextureName);
				texNum++;
			}
			else {
				strcpy_s(m_pMaterial[mInd].szTextureName, mesh->getMaterialName());//�e�N�X�`�����������ꍇ�}�e���A��������
				m_pMaterial[mInd].tex_no = dx->GetTexNumber(m_pMaterial[mInd].szTextureName);
				texNum++;
			}

			int iCount = 0;//�ŏI�I��(������)index�J�E���g
			int vCount = 0;//���_�J�E���g
			int polygon_cnt = 0;
			for (DWORD k = 0; k < pdwNumFace[m]; k++) {
				int matId = mesh->getMaterialNoOfPolygon(k);//�|���S��k�̊֘A����}�e���A���ԍ��擾
				int pcs = mesh->getPolygonSize(k);//�e�|���S���̒��_��
				if (pcs != 3 && pcs != 4)continue;//���_��3��,4�ȊO�̓X�L�b�v
				if (matId == i)//���ݏ������̃}�e���A���ƈ�v�����ꍇ�ȉ��������s(1Mesh��2�ȏ�̃}�e���A�����L��ꍇ�̏���)
				{
					pIndex[mInd][iCount] = vCount + VerArrStart;//vpVB�͕����Oindex���ɐ���ς�
					pIndex[mInd][iCount + 1] = vCount + 1 + VerArrStart;
					pIndex[mInd][iCount + 2] = vCount + 2 + VerArrStart;
					iCount += 3;

					//4���_��{ 0, 1, 2 }, { 0, 2, 3 }�ŕ�������悤����
					if (pcs == 4) {
						pIndex[mInd][iCount] = vCount + VerArrStart;
						pIndex[mInd][iCount + 1] = vCount + 2 + VerArrStart;
						pIndex[mInd][iCount + 2] = vCount + 3 + VerArrStart;
						iCount += 3;//4���_�𕪊���̓C���f�b�N�X��3���_�̔{
						vCount++;
					}
					vCount += 3;
					polygon_cnt++;
				}
			}

			//�C���f�b�N�X�o�b�t�@����1�i���
			if (iCount > 0) CreateIndexBuffer(iCount, mInd);
			m_pMaterial[mInd].dwNumFace = polygon_cnt;//���̃}�e���A�����̃|���S����	
			mInd++;
		}
		VerArrStart += IndexCount34Me[m];
		VerArrStart2 += m_pdwNumVert[m];
	}

	//������W���_�̖@�����ꉻ(�e�Z���[�V�����p)
	for (DWORD i = 0; i < VerAllpcs; i++) {
		int indVB = 0;
		VECTOR3 geo[50];
		int indVb[50];
		int indGeo = 0;
		while (1) {
			indVB = svList[i].Pop();
			if (indVB == -1)break;
			indVb[indGeo] = indVB;
			geo[indGeo++] = pvVB[indVB].vGeoNorm;
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
			pvVB[indVb[i1]].vGeoNorm = ave;
		}
	}

	//�e�ꎞ�i�[�p�z����
	ARR_DELETE(m_pMaterialCount);
	ARR_DELETE(IndexCount34Me);
	ARR_DELETE(IndexCount3M);
	ARR_DELETE(pdwNumFace);
	ARR_DELETE(tmpVB);
	ARR_DELETE(svList);

	for (int i = 0; i < MateAllpcs; i++) {
		CONSTANT_BUFFER2 sg;
		m_pMaterial[i].Kd.x += addDiffuse;
		m_pMaterial[i].Kd.y += addDiffuse;
		m_pMaterial[i].Kd.z += addDiffuse;
		m_pMaterial[i].Ks.x += addSpecular;
		m_pMaterial[i].Ks.y += addSpecular;
		m_pMaterial[i].Ks.z += addSpecular;
		m_pMaterial[i].Ka.x += addAmbient;
		m_pMaterial[i].Ka.y += addAmbient;
		m_pMaterial[i].Ka.z += addAmbient;
		sg.vDiffuse = m_pMaterial[i].Kd;//�f�B�t���[�Y�J���[���V�F�[�_�[�ɓn��
		sg.vSpeculer = m_pMaterial[i].Ks;//�X�y�L�����[���V�F�[�_�[�ɓn��
		sg.vAmbient = m_pMaterial[i].Ka;//�A���r�G���g���V�F�[�_�[�ɓn��
		mObjectCB1->CopyData(i, sg);
	}
}

void SkinMesh::SetDiffuseTextureName(char *textureName, int materialIndex) {
	if (m_pMaterial[materialIndex].tex_no != -1)return;//���ɐݒ�ς݂̏ꍇ����
	strcpy_s(m_pMaterial[materialIndex].szTextureName, textureName);
	//�t�@�C���������Ɋ��Ƀf�R�[�h�ς݂̃e�N�X�`���ԍ���ǂݍ���
	m_pMaterial[materialIndex].tex_no = dx->GetTexNumber(m_pMaterial[materialIndex].szTextureName);
	texNum++;
}

void SkinMesh::SetNormalTextureName(char *textureName, int materialIndex) {
	if (m_pMaterial[materialIndex].nortex_no != -1)return;
	strcpy_s(m_pMaterial[materialIndex].norTextureName, textureName);
	//�t�@�C���������Ɋ��Ƀf�R�[�h�ς݂̃e�N�X�`���ԍ���ǂݍ���
	m_pMaterial[materialIndex].nortex_no = dx->GetTexNumber(m_pMaterial[materialIndex].norTextureName);
	texNum++;
}

bool SkinMesh::CreateFromFBX(bool disp) {
	//�C���f�b�N�X�o�b�t�@����2�i���, �ꎞ�i�[�z����
	for (int i = 0; i < MateAllpcs; i++) {
		CreateIndexBuffer2(pIndex[i], i);
		ARR_DELETE(pIndex[i]);
	}
	ARR_DELETE(pIndex);

	//�o�[�e�b�N�X�o�b�t�@�[�쐬
	const UINT vbByteSize = (UINT)IndexCount34MeAll * sizeof(MY_VERTEX_S);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(mCommandList, pvVB, vbByteSize, Vview->VertexBufferUploader);
	Vview->VertexByteStride = sizeof(MY_VERTEX_S);
	Vview->VertexBufferByteSize = vbByteSize;

	if (pvVB_delete_f)ARR_DELETE(pvVB);//�g��Ȃ��ꍇ���

	vs = dx->pVertexShader_SKIN.Get();
	if (disp) {
		vsB = dx->pVertexShader_SKIN_D.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->pDomainShaderTriangle.Get();
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		primType_drawB = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}
	else {
		vsB = dx->pVertexShader_SKIN.Get();
		hs = nullptr;
		ds = nullptr;
		primType_draw = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		primType_drawB = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	ps = dx->pPixelShader_3D.Get();
	psB = dx->pPixelShader_Bump.Get();

	CD3DX12_DESCRIPTOR_RANGE texTable, nortexTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//����DescriptorRange�̓V�F�[�_�[���\�[�X�r���[,Descriptor 1��, shader��registerIndex
	nortexTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);//DescriptorRange�̐���1��, DescriptorRange�̐擪�A�h���X
	slotRootParameter[1].InitAsDescriptorTable(1, &nortexTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[2].InitAsConstantBufferView(0);
	slotRootParameter[3].InitAsConstantBufferView(1);
	slotRootParameter[4].InitAsConstantBufferView(2);

	mRootSignature = CreateRs(5, slotRootParameter);
	if (mRootSignature == nullptr)return false;

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	mPSO = CreatePsoVsPs(vs, ps, mRootSignature.Get(), dx->pVertexLayout_SKIN, alpha, blend);
	if (mPSO == nullptr)return false;
	mPSO_B = CreatePsoVsHsDsPs(vsB, hs, ds, psB, mRootSignature.Get(), dx->pVertexLayout_SKIN, alpha, blend);
	if (mPSO_B == nullptr)return false;

	return GetTexture();
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
		m_ppSubAnimationBone = new Deformer*[(FBX_PCS - 1) * m_iNumBone];
	}
	return S_OK;
}

void SkinMesh::CreateFromFBX_SubAnimation(int ind) {
	int loopind = 0;
	int searchCount = 0;
	int name2Num = 0;
	while (loopind < m_iNumBone) {
		int sa_ind = (ind - 1) * m_iNumBone + loopind;
		m_ppSubAnimationBone[sa_ind] = fbx[ind].fbxL->getNoneMeshDeformer(searchCount);
		searchCount++;
		const char *name = m_ppSubAnimationBone[sa_ind]->getName();
		if (!strncmp("Skeleton", name, 8))continue;
		char *name2 = &m_pClusterName[loopind * 255];//�eBone���̐擪�A�h���X��n��
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

void SkinMesh::CreateIndexBuffer(int cnt, int IviewInd) {

	const UINT ibByteSize = (UINT)cnt * sizeof(UINT);

	Iview[IviewInd].IndexFormat = DXGI_FORMAT_R32_UINT;
	Iview[IviewInd].IndexBufferByteSize = ibByteSize;
	Iview[IviewInd].IndexCount = cnt;
}

void SkinMesh::CreateIndexBuffer2(int *pIndex, int IviewInd) {

	Iview[IviewInd].IndexBufferGPU = dx->CreateDefaultBuffer(mCommandList, pIndex,
		Iview[IviewInd].IndexBufferByteSize, Iview[IviewInd].IndexBufferUploader);
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
		for (int i = 0; i < m_iNumBone; i++) {
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
	if (!subanm) {
		defo = m_ppCluster[0];
	}
	else {
		defo = m_ppSubAnimationBone[(ind - 1) * m_iNumBone];
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
	for (int i = 0; i < m_iNumBone; i++) {
		Deformer* de = nullptr;
		if (!subanm) {
			de = m_ppCluster[i];
		}
		else {
			de = m_ppSubAnimationBone[(ind - 1) * m_iNumBone + i];
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
			for (int i = 0; i < m_iNumBone; i++) {
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

VECTOR3 SkinMesh::GetVertexPosition(int verNum, float adjustZ, float adjustY, float adjustX, float thetaZ, float thetaY, float thetaX, float scal) {

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
		pos.x = pvVB[verNum].vPos.x;
		pos.y = pvVB[verNum].vPos.y;
		pos.z = pvVB[verNum].vPos.z;
		MATRIX m = GetCurrentPoseMatrix(pvVB[verNum].bBoneIndex[i]);
		VectorMatrixMultiply(&pos, &m);
		VectorMultiply(&pos, pvVB[verNum].bBoneWeight[i]);
		VectorAddition(&ret, &ret, &pos);
	}
	ret.x += adjustX;
	ret.y += adjustY;
	ret.z += adjustZ;
	VectorMatrixMultiply(&ret, &scaro);
	return ret;
}

void SkinMesh::MatrixMap_Bone(SHADER_GLOBAL_BONES *sgb) {

	for (int i = 0; i < m_iNumBone; i++)
	{
		MATRIX mat = GetCurrentPoseMatrix(i);
		MatrixTranspose(&mat);
		sgb->mBone[i] = mat;
	}
}

bool SkinMesh::GetTexture() {

	TextureNo *te = new TextureNo[MateAllpcs];
	for (int i = 0; i < MateAllpcs; i++) {
		te[i].diffuse = m_pMaterial[i].tex_no;
		te[i].normal = m_pMaterial[i].nortex_no;
		te[i].movie = m_on;
	}
	mSrvHeap = CreateSrvHeap(MateAllpcs, texNum, te, texture);

	ARR_DELETE(te);
	if (mSrvHeap == nullptr)return false;

	return true;
}

void SkinMesh::CbSwap() {
	Lock();
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2�v�f����X�V�I��
	}
	sw = 1 - sw;//cb�X���b�v
	dx->ins_no = 0;
	Unlock();
	DrawOn = true;
}

bool SkinMesh::Update(float time, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size) {
	return Update(0, time, x, y, z, r, g, b, a, thetaZ, thetaY, thetaX, size);
}

bool SkinMesh::Update(int ind, float ti, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp) {

	bool frame_end = false;
	dx->InstancedMap(&cb[sw], x, y, z, thetaZ, thetaY, thetaX, size);
	dx->MatrixMap(&cb[sw], r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f);
	if (ti != -1.0f)frame_end = SetNewPoseMatrices(ti, ind);
	MatrixMap_Bone(&sgb[sw]);
	CbSwap();
	return frame_end;
}

void SkinMesh::DrawOff() {
	DrawOn = false;
}

void SkinMesh::Draw() {

	if (!UpOn | !DrawOn)return;

	Lock();
	mObjectCB0->CopyData(0, cb[1 - sw]);
	mObject_BONES->CopyData(0, sgb[1 - sw]);
	Unlock();

	drawPara para;
	para.NumMaterial = MateAllpcs;
	para.srv = mSrvHeap.Get();
	para.rootSignature = mRootSignature.Get();
	para.Vview = Vview.get();
	para.Iview = Iview.get();
	para.material = m_pMaterial;
	para.haveNortexTOPOLOGY = primType_drawB;
	para.notHaveNortexTOPOLOGY = primType_draw;
	para.haveNortexPSO = mPSO_B.Get();
	para.notHaveNortexPSO = mPSO.Get();
	para.cbRes0 = mObjectCB0->Resource();
	para.cbRes1 = mObjectCB1->Resource();
	para.cbRes2 = mObject_BONES->Resource();
	para.sRes0 = nullptr;
	para.sRes1 = nullptr;
	para.insNum = 1;
	drawsub(para);
}


