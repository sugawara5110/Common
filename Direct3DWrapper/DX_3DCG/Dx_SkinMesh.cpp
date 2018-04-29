//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          SkinMesh�N���X(FBX_SDK)                           **//
//**                                                                                     **//
//*****************************************************************************************//

//Smoothie3D�Ń��f����Blender�ŃA�j���[�V�����̏ꍇ,-Z�O��,Y��ŏo�͂���
//makehuman�Ń��f����Blender�ŃA�j���[�V����(�J�[�l�M�[��������w��GET�������[�V�����L���v�`���f�[�^�g�p)
//�̏ꍇ,Y�O��,Z��ŏo�͂���

#define _CRT_SECURE_NO_WARNINGS
#define FBX_PCS 5
#define WAIT(str) do{stInitFBX_ON = TRUE;while (stSetNewPose_ON);str;stInitFBX_ON = FALSE;}while(0)
#include "Dx_SkinMesh.h"
#include <string.h>

using namespace std;

mutex SkinMesh::mtx;

volatile bool SkinMesh::stInitFBX_ON = FALSE;
volatile bool SkinMesh::stSetNewPose_ON = FALSE;

FbxManager *SkinMesh::m_pSdkManager = NULL;
FbxImporter *SkinMesh::m_pImporter = NULL;
bool SkinMesh::ManagerCreated = false;

SkinMesh_sub::SkinMesh_sub() {
	current_frame = 0.0f;
	centering = TRUE;
	offset = FALSE;
	cx = cy = cz = 0.0f;
	connect_step = 3000.0f;
}

SkinMesh_sub::~SkinMesh_sub() {
	if (m_pmyScene) m_pmyScene->Destroy();
}

bool SkinMesh_sub::Create(CHAR *szFileName) {
	int iFormat = -1;
	SkinMesh::m_pImporter->Initialize((const char*)szFileName, iFormat);
	m_pmyScene = FbxScene::Create(SkinMesh::m_pSdkManager, "my scene");
	return SkinMesh::m_pImporter->Import(m_pmyScene);
}

void SkinMesh::CreateManager() {
	m_pSdkManager = FbxManager::Create();
	m_pImporter = FbxImporter::Create(m_pSdkManager, "my importer");
	ManagerCreated = true;
}

void SkinMesh::DeleteManager() {
	if (m_pSdkManager) m_pSdkManager->Destroy();
	ManagerCreated = false;
}

SkinMesh::SkinMesh() {
	if (!ManagerCreated) {
		MessageBoxA(0, "CreateManager()�����s����Ă��܂���", 0, MB_OK);
	}

	ZeroMemory(this, sizeof(SkinMesh));
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	fbx = new SkinMesh_sub[FBX_PCS];
	m_ppSubAnimationBone = NULL;
	m_pClusterName = NULL;
	AnimLastInd = -1;
	BoneConnect = -1.0f;
	pvVB_delete_f = TRUE;
	pvVB = NULL;
	texNum = 0;
}

SkinMesh::~SkinMesh() {
	ARR_DELETE(pvVB);
	ARR_DELETE(m_pClusterName);
	ARR_DELETE(m_ppSubAnimationBone);
	ARR_DELETE(m_pLastBoneMatrix);
	ARR_DELETE(m_pdwNumVert);
	ARR_DELETE(m_ppNodeArray);
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

void SkinMesh::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

void SkinMesh::SetState(bool al, bool bl) {
	alpha = al;
	blend = bl;
}

void SkinMesh::ObjCentering(bool f, int ind) {
	fbx[ind].centering = f;
	fbx[ind].offset = FALSE;
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
	fbx[ind].centering = TRUE;
	fbx[ind].offset = FALSE;
	fbx[ind].cx = x;
	fbx[ind].cy = y;
	fbx[ind].cz = z;
	CreateRotMatrix(thetaZ, thetaY, thetaX, ind);
}

void SkinMesh::ObjOffset(float x, float y, float z, float thetaZ, float thetaY, float thetaX, int ind) {
	fbx[ind].centering = TRUE;
	fbx[ind].offset = TRUE;
	fbx[ind].cx = x;
	fbx[ind].cy = y;
	fbx[ind].cz = z;
	CreateRotMatrix(thetaZ, thetaY, thetaX, ind);
}

int SkinMesh::SearchNodeCount(FbxNode *pnode, FbxNodeAttribute::EType SearchType) {
	static int matchNodeCount = 0;//�X���b�h�Z�[�t�ɂ���ꍇ�̓N���X�ϐ��ɕύX����

	if (!pnode) { matchNodeCount = 0; return 0; }//pNode��NULL��^����ƃ��Z�b�g

	FbxNodeAttribute* pAttr = pnode->GetNodeAttribute();//�m�[�h�^�C�v�擾(rootNode�̏ꍇNULL�ɂȂ�)
	if (pAttr && pAttr->GetAttributeType() == SearchType && strncmp("GZM_", pnode->GetName(), 4) != 0) matchNodeCount++;//�^�C�v��v�̏ꍇ�J�E���g

	int childNodeCount = pnode->GetChildCount();
	if (childNodeCount == 0)return matchNodeCount;//�����ŒT���I���̉\�����L��̂ŃJ�E���g����Ԃ�

	for (int i = 0; i < childNodeCount; i++) {
		FbxNode *pChild = pnode->GetChild(i);  //�qNode���擾
		SearchNodeCount(pChild, SearchType);
	}

	return matchNodeCount;//�����ŒT���I���̉\�����L��̂ŃJ�E���g����Ԃ�
}

FbxNode *SkinMesh::SearchNode(FbxNode *pnode, FbxNodeAttribute::EType SearchType, int Ind) {
	static int matchNodeCount = 0;

	if (!pnode) { matchNodeCount = 0; return NULL; }//���Z�b�g

	FbxNodeAttribute* pAttr = pnode->GetNodeAttribute();//�m�[�h�^�C�v�擾(rootNode�̏ꍇNULL�ɂȂ�)
	if (pAttr && pAttr->GetAttributeType() == SearchType && strncmp("GZM_", pnode->GetName(), 4) != 0) { //�T��Node�^�C�v����v������
		if (matchNodeCount == Ind) {//�T��Node����v������
			return pnode;//�T������
		}
		matchNodeCount++;//��v���Ȃ��ꍇ�J�E���g
	}

	//rootNode�܂��͒T�������̏ꍇ�q�m�[�h�m�F, �����ꍇNULL��Ԃ�
	int childNodeCount = pnode->GetChildCount();
	if (childNodeCount == 0)return NULL;

	//�qNode�T��
	for (int i = 0; i < childNodeCount; i++) {
		FbxNode *pChild = pnode->GetChild(i);  //�qNode���擾
		FbxNode *returnNode = SearchNode(pChild, SearchType, Ind);
		if (returnNode)return returnNode;//NULL�ȊO���Ԃ����ꍇ�T�������ɂȂ�̂�Node��Ԃ�
		//NULL�̏ꍇ�qNode�T�����p��
	}

	//�q�m�[�h�S��NULL�̏ꍇ
	return NULL;
}

HRESULT SkinMesh::InitFBX(CHAR *szFileName, int p) {

	bool f = FALSE;
	WAIT(f = fbx[p].Create(szFileName));//WAIT()FBX_SDK�S�֐��ɂ�邩�͗l�q��
	if (f)return S_OK;
	return E_FAIL;
}

FbxScene *SkinMesh::GetScene(int p) {
	return fbx[p].m_pmyScene;
}

void SkinMesh::DestroyFBX() {
	WAIT(ARR_DELETE(fbx));
}

HRESULT SkinMesh::ReadSkinInfo(MY_VERTEX_S *pvVB) {

	FbxMesh *pmesh = m_ppNodeArray[0]->GetMesh();
	FbxDeformer *pDeformer = pmesh->GetDeformer(0);
	FbxSkin *pSkinInfo = static_cast<FbxSkin*>(pDeformer);

	for (int i = 0; i < m_iNumBone; i++) {
		m_ppCluster[i] = pSkinInfo->GetCluster(i);//�s��֌W�͂ǂ̃��b�V������ł��������𓾂���悤��
		const char *name = m_ppCluster[i]->GetName();
		strcpy(&m_pClusterName[i * 255], name);//�{�[���̖��O�ێ�
	}

	int VertexStart = 0;
	for (int m = 0; m < NodeArraypcs; m++) {

		FbxMesh *pmesh = m_ppNodeArray[m]->GetMesh();
		FbxDeformer *pDeformer = pmesh->GetDeformer(0);
		FbxSkin *pSkinInfo = static_cast<FbxSkin*>(pDeformer);

		//�eBone�̃E�G�C�g,�C���f�b�N�X�𒲂ג��_�z��ɉ�����
		for (int i = 0; i < m_iNumBone; i++) {
			FbxCluster *cl = pSkinInfo->GetCluster(i);
			int iNumIndex = cl->GetControlPointIndicesCount();//���̃{�[���ɉe�����󂯂钸�_�C���f�b�N�X��
			int *piIndex = cl->GetControlPointIndices();     //���̃{�[���ɉe�����󂯂钸�_�̃C���f�b�N�X�z��
			double *pdWeight = cl->GetControlPointWeights();//���̃{�[���ɉe�����󂯂钸�_�̃E�G�C�g�z��

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

	FbxAMatrix mat;
	for (int i = 0; i < m_iNumBone; i++) {
		//�����p���s��ǂݍ���
		//GetCurrentPoseMatrix�Ŏg��
		m_ppCluster[i]->GetTransformLinkMatrix(mat);
		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				m_BoneArray[i].mBindPose.m[y][x] = (float)mat.Get(y, x);
			}
		}
	}

	return S_OK;
}

void SkinMesh::SetConnectStep(int ind, float step) {
	fbx[ind].connect_step = step;
}

void SkinMesh::Vertex_hold() {
	pvVB_delete_f = FALSE;
}

HRESULT SkinMesh::GetFbx(CHAR* szFileName) {
	//FBX���[�_�[��������
	if (FAILED(InitFBX(szFileName, 0)))
	{
		MessageBox(0, L"FBX���[�_�[���������s", NULL, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}

void SkinMesh::GetBuffer(float end_frame) {

	mObjectCB0 = new UploadBuffer<CONSTANT_BUFFER>(dx->md3dDevice.Get(), 1, true);
	mObject_BONES = new UploadBuffer<SHADER_GLOBAL_BONES>(dx->md3dDevice.Get(), 1, true);

	fbx[0].end_frame = end_frame;
	FbxScene *pScene = GetScene(0);//�V�[���擾
	FbxNode *pNodeRoot = pScene->GetRootNode();//���[�g�m�[�h�擾

	WAIT(NodeArraypcs = SearchNodeCount(pNodeRoot, FbxNodeAttribute::eMesh));//eMesh���J�E���g
	SearchNodeCount(NULL, FbxNodeAttribute::eMesh);//���Z�b�g

	//�e���b�V���ւ̃|�C���^�z��擾
	m_ppNodeArray = new FbxNode*[NodeArraypcs];
	WAIT(
		for (int i = 0; i < NodeArraypcs; i++) {
			m_ppNodeArray[i] = SearchNode(pNodeRoot, FbxNodeAttribute::eMesh, i);
			SearchNode(NULL, FbxNodeAttribute::eMesh, 0);//���Z�b�g
		}
	);

	//���O�Ƀ��b�V�������_���A�|���S�����}�e���A�������𒲂ׂ�
	m_pdwNumVert = new DWORD[NodeArraypcs];
	MateAllpcs = 0;//�}�e���A���S��
	m_pMaterialCount = new DWORD[NodeArraypcs];
	VerAllpcs = 0;//���_�S��
	pdwNumFace = new DWORD[NodeArraypcs];

	//�J�E���g
	for (int i = 0; i < NodeArraypcs; i++) {
		FbxMesh *pFbxMesh = m_ppNodeArray[i]->GetMesh();
		m_pdwNumVert[i] = pFbxMesh->GetControlPointsCount();//���b�V�������_��
		m_pMaterialCount[i] = m_ppNodeArray[i]->GetMaterialCount();//���b�V�����}�e���A����
		VerAllpcs += m_pdwNumVert[i];                 //���_�S���J�E���g
		MateAllpcs += m_pMaterialCount[i];           //�}�e���A���S���J�E���g
		pdwNumFace[i] = pFbxMesh->GetPolygonCount();//���b�V�����|���S����
	}

	//���b�V����4���_�����O�C���f�b�N�X�z�񐶐�
	IndexCount34Me = new int[NodeArraypcs];
	//�}�e���A����4���_������C���f�b�N�X�z�񐶐�
	IndexCount3M = new int[MateAllpcs];

	//�}�e���A���z�񐶐�
	m_pMaterial = new MY_MATERIAL_S[MateAllpcs];

	//�}�e���A���R���X�^���g�o�b�t�@
	mObjectCB1 = new UploadBuffer<CONSTANT_BUFFER2>(dx->md3dDevice.Get(), MateAllpcs, true);

	//VBO
	Vview = std::make_unique<VertexView>();
	//IBO �}�e���A�����ɐ�������
	Iview = std::make_unique<IndexView[]>(MateAllpcs);

	//�C���f�b�N�X���v�Z(�|���S��1�ɕt�����_ 2�`5�����݂��邱�Ƃ��L�邪
	//fbx����Index�z��R�s�[���ɑS�ăR�s�[����̂ł��̂܂܂̌����J�E���g����)
	IndexCount34MeAll = 0;
	for (int k = 0; k < NodeArraypcs; k++) {
		IndexCount34Me[k] = 0;
		FbxMesh *pFbxMesh = m_ppNodeArray[k]->GetMesh();
		for (DWORD i = 0; i < pdwNumFace[k]; i++) {
			IndexCount34Me[k] += pFbxMesh->GetPolygonSize(i);//���b�V�����̕����O���_�C���f�b�N�X��
		}
		IndexCount34MeAll += IndexCount34Me[k];
	}

	int mInd = 0;
	for (int k = 0; k < NodeArraypcs; k++) {
		FbxMesh *pFbxMesh = m_ppNodeArray[k]->GetMesh();
		FbxLayerElementMaterial *mat = pFbxMesh->GetLayer(0)->GetMaterials();//���C���[0�̃}�e���A���擾
		for (DWORD j = 0; j < m_pMaterialCount[k]; j++) {
			IndexCount3M[mInd] = 0;
			for (DWORD i = 0; i < pdwNumFace[k]; i++) {
				int matId = mat->GetIndexArray().GetAt(i);//�|���S��i�̊֘A����}�e���A���ԍ��擾
				if (matId == j) {//���ݏ������̃}�e���A���ƈ�v�����ꍇ�ȉ��������s(1Mesh��2�ȏ�̃}�e���A�����L��ꍇ�̏���)
					int pcs = pFbxMesh->GetPolygonSize(i);//�e�|���S���̒��_�C���f�b�N�X��
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

	FbxMesh *pmesh = m_ppNodeArray[0]->GetMesh();
	FbxDeformer *pDeformer = pmesh->GetDeformer(0);
	FbxSkin *pSkinInfo = static_cast<FbxSkin*>(pDeformer);
	m_iNumBone = pSkinInfo->GetClusterCount();//�ǂ̃��b�V������ł������{�[�����������Ă���悤��
	m_ppCluster = new  FbxCluster*[m_iNumBone];//�{�[�����z��
	m_pClusterName = new char[m_iNumBone * 255];

	//�{�[���𐶐�
	m_BoneArray = new BONE[m_iNumBone];
	m_pLastBoneMatrix = new MATRIX[m_iNumBone];
}

void SkinMesh::SetVertex() {

	//4���_�����O��Ԃł̃C���f�b�N�X���Œ��_�z�񐶐�
	//4���_�|���S���͕�����@���ω������̈�, ���_��3���_,4���_���ݏ�Ԃ̗v�f���Ő���
	pvVB = new MY_VERTEX_S[IndexCount34MeAll];

	MY_VERTEX_S *tmpVB = new MY_VERTEX_S[VerAllpcs];
	//�X�L�����(�W���C���g, �E�F�C�g)
	ReadSkinInfo(tmpVB);

	//������W���_���X�g
	SameVertexList *svList = new SameVertexList[VerAllpcs];

	//���b�V�����ɔz��i�[����
	int mInd = 0;//�}�e���A�����J�E���g
	DWORD VerArrStart = 0;//�����O�C���f�b�N�X��(�ŏI�I�Ȓ��_��)
	DWORD VerArrStart2 = 0;//�ǂݍ��ݎ����_��
	for (int m = 0; m < NodeArraypcs; m++) {
		FbxMesh *pFbxMesh = m_ppNodeArray[m]->GetMesh();
		FbxNode *pNode = pFbxMesh->GetNode();

		int *piIndex = pFbxMesh->GetPolygonVertices();//fbx���璸�_�C���f�b�N�X�z��擾

		//���_�z���fbx����R�s�[
		FbxVector4 *pCoord = pFbxMesh->GetControlPoints();
		for (int i = 0; i < IndexCount34Me[m]; i++) {
			//fbx����ǂݍ���index���Œ��_�𐮗񂵂Ȃ��璸�_�i�[
			pvVB[i + VerArrStart].vPos.x = (float)pCoord[piIndex[i]][0];
			pvVB[i + VerArrStart].vPos.y = (float)pCoord[piIndex[i]][1];
			pvVB[i + VerArrStart].vPos.z = (float)pCoord[piIndex[i]][2];
			svList[piIndex[i] + VerArrStart2].Push(i + VerArrStart);
			for (int bi = 0; bi < 4; bi++) {
				//ReadSkinInfo(tmpVB)�œǂݍ��񂾊e�p�����[�^�R�s�[
				pvVB[i + VerArrStart].bBoneIndex[bi] = tmpVB[piIndex[i] + VerArrStart2].bBoneIndex[bi];
				pvVB[i + VerArrStart].bBoneWeight[bi] = tmpVB[piIndex[i] + VerArrStart2].bBoneWeight[bi];
			}
		}

		//�@��, �e�N�X�`���[���W�ǂݍ���
		FbxVector2 UV;
		FbxLayerElementUV *pUV = 0;
		pUV = pFbxMesh->GetLayer(0)->GetUVs();
		const char *uvname = pUV->GetName();
		FbxLayerElement::EMappingMode mappingMode = pUV->GetMappingMode();
		bool UnMap = TRUE;
		if (mappingMode == FbxLayerElement::eByPolygonVertex)UnMap = FALSE;
		FbxVector4 Normal;

		int IndCount = 0;
		for (DWORD i = 0; i < pdwNumFace[m]; i++) {
			//int iStartIndex = pFbxMesh->GetPolygonVertexIndex(i);//�|���S�����\������ŏ��̃C���f�b�N�X�擾
			int pcs = pFbxMesh->GetPolygonSize(i);//�e�|���S���̒��_��
			if (pcs != 3 && pcs != 4)continue;//���_��3��,4�ȊO�̓X�L�b�v
			int iVert0Index, iVert1Index, iVert2Index, iVert3Index;

			//�@��,UV�ǂݍ���,index�œǂݍ���
			//���_��index�Ő���ς�
			iVert0Index = IndCount + VerArrStart;
			iVert1Index = IndCount + 1 + VerArrStart;
			iVert2Index = IndCount + 2 + VerArrStart;
			iVert3Index = IndCount + 3 + VerArrStart;

			if (iVert0Index < 0)continue;
			pFbxMesh->GetPolygonVertexNormal(i, 0, Normal);//(polyInd, verInd, FbxVector4)
			pvVB[iVert0Index].vNorm.x = (float)Normal[0];
			pvVB[iVert0Index].vNorm.y = (float)Normal[1];
			pvVB[iVert0Index].vNorm.z = (float)Normal[2];
			pvVB[iVert0Index].vGeoNorm.x = (float)Normal[0];
			pvVB[iVert0Index].vGeoNorm.y = (float)Normal[1];
			pvVB[iVert0Index].vGeoNorm.z = (float)Normal[2];

			pFbxMesh->GetPolygonVertexUV(i, 0, uvname, UV, UnMap);//(polyInd, verInd, UV_Name, FbxVector2_UV, bool_UnMap)
			pvVB[iVert0Index].vTex.x = (float)UV[0];
			pvVB[iVert0Index].vTex.y = 1.0f - (float)UV[1];//(1.0f-UV)

			pFbxMesh->GetPolygonVertexNormal(i, 1, Normal);
			pvVB[iVert1Index].vNorm.x = (float)Normal[0];
			pvVB[iVert1Index].vNorm.y = (float)Normal[1];
			pvVB[iVert1Index].vNorm.z = (float)Normal[2];
			pvVB[iVert1Index].vGeoNorm.x = (float)Normal[0];
			pvVB[iVert1Index].vGeoNorm.y = (float)Normal[1];
			pvVB[iVert1Index].vGeoNorm.z = (float)Normal[2];

			pFbxMesh->GetPolygonVertexUV(i, 1, uvname, UV, UnMap);
			pvVB[iVert1Index].vTex.x = (float)UV[0];
			pvVB[iVert1Index].vTex.y = 1.0f - (float)UV[1];

			pFbxMesh->GetPolygonVertexNormal(i, 2, Normal);
			pvVB[iVert2Index].vNorm.x = (float)Normal[0];
			pvVB[iVert2Index].vNorm.y = (float)Normal[1];
			pvVB[iVert2Index].vNorm.z = (float)Normal[2];
			pvVB[iVert2Index].vGeoNorm.x = (float)Normal[0];
			pvVB[iVert2Index].vGeoNorm.y = (float)Normal[1];
			pvVB[iVert2Index].vGeoNorm.z = (float)Normal[2];

			pFbxMesh->GetPolygonVertexUV(i, 2, uvname, UV, UnMap);
			pvVB[iVert2Index].vTex.x = (float)UV[0];
			pvVB[iVert2Index].vTex.y = 1.0f - (float)UV[1];

			if (pcs == 4) {
				pFbxMesh->GetPolygonVertexNormal(i, 3, Normal);
				pvVB[iVert3Index].vNorm.x = (float)Normal[0];
				pvVB[iVert3Index].vNorm.y = (float)Normal[1];
				pvVB[iVert3Index].vNorm.z = (float)Normal[2];
				pvVB[iVert3Index].vGeoNorm.x = (float)Normal[0];
				pvVB[iVert3Index].vGeoNorm.y = (float)Normal[1];
				pvVB[iVert3Index].vGeoNorm.z = (float)Normal[2];

				pFbxMesh->GetPolygonVertexUV(i, 3, uvname, UV, UnMap);
				pvVB[iVert3Index].vTex.x = (float)UV[0];
				pvVB[iVert3Index].vTex.y = 1.0f - (float)UV[1];
				IndCount++;
			}
			IndCount += 3;
		}

		for (DWORD i = 0; i < m_pMaterialCount[m]; i++) {

			//�}�e���A�����擾
			FbxSurfaceMaterial *pMaterial = pNode->GetMaterial(i);
			FbxSurfacePhong *pPhong = (FbxSurfacePhong*)pMaterial;

			//�g�U���ˌ�
			FbxDouble3 d3Diffuse = pPhong->sDiffuseDefault;
			m_pMaterial[mInd].Kd.x = (float)d3Diffuse[0];
			m_pMaterial[mInd].Kd.y = (float)d3Diffuse[1];
			m_pMaterial[mInd].Kd.z = (float)d3Diffuse[2];
			m_pMaterial[mInd].Kd.w = 1.0f;//�������������ꍇ�������ǂ��ɂ�����
			//�X�y�L�����[
			FbxDouble3 d3Specular = pPhong->sSpecularDefault;
			m_pMaterial[mInd].Ks.x = (float)d3Specular[0];
			m_pMaterial[mInd].Ks.y = (float)d3Specular[1];
			m_pMaterial[mInd].Ks.z = (float)d3Specular[2];
			m_pMaterial[mInd].Ks.w = 0.0f;

			//�e�N�X�`���[
			FbxProperty lPropertyDif;
			FbxProperty lPropertyNor;
			lPropertyDif = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
			lPropertyNor = pMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);

			FbxTexture *textureNor = FbxCast<FbxTexture>(lPropertyNor.GetSrcObject<FbxTexture>(0));
			if (textureNor) {
				FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(textureNor);
				strcpy_s(m_pMaterial[mInd].norTextureName, fileTexture->GetFileName());
				//�t�@�C���������Ɋ��Ƀf�R�[�h�ς݂̃e�N�X�`���ԍ���ǂݍ���
				m_pMaterial[mInd].nortex_no = dx->GetTexNumber(m_pMaterial[mInd].norTextureName);
				texNum++;
			}
			FbxTexture *textureDif = FbxCast<FbxTexture>(lPropertyDif.GetSrcObject<FbxTexture>(0));
			if (textureDif) {
				FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(textureDif);
				strcpy_s(m_pMaterial[mInd].szTextureName, fileTexture->GetFileName());
				//�t�@�C���������Ɋ��Ƀf�R�[�h�ς݂̃e�N�X�`���ԍ���ǂݍ���
				m_pMaterial[mInd].tex_no = dx->GetTexNumber(m_pMaterial[mInd].szTextureName);
				texNum++;
			}
			else {
				strcpy_s(m_pMaterial[mInd].szTextureName, pMaterial->GetName());//�e�N�X�`�����������ꍇ�}�e���A��������
				m_pMaterial[mInd].tex_no = dx->GetTexNumber(m_pMaterial[mInd].szTextureName);
				texNum++;
			}

			int iCount = 0;//�ŏI�I��(������)index�J�E���g
			int vCount = 0;//���_�J�E���g
			int polygon_cnt = 0;
			for (DWORD k = 0; k < pdwNumFace[m]; k++) {
				FbxLayerElementMaterial* mat = pFbxMesh->GetLayer(0)->GetMaterials();//���C���[0�̃}�e���A���擾
				int matId = mat->GetIndexArray().GetAt(k);//�|���S��k�̊֘A����}�e���A���ԍ��擾
				int pcs = pFbxMesh->GetPolygonSize(k);//�e�|���S���̒��_��
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
			if (iCount > 0) CreateIndexBuffer(iCount, pIndex[mInd], mInd);
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
		sg.vDiffuse = m_pMaterial[i].Kd;//�f�B�t���[�Y�J���[���V�F�[�_�[�ɓn��
		sg.vSpeculer = m_pMaterial[i].Ks;//�X�y�L�����[���V�F�[�_�[�ɓn��
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

void SkinMesh::CreateFromFBX(bool disp) {

	//�C���f�b�N�X�o�b�t�@����2�i���, �ꎞ�i�[�z����
	for (int i = 0; i < MateAllpcs; i++) {
		CreateIndexBuffer2(pIndex[i], i);
		ARR_DELETE(pIndex[i]);
	}
	ARR_DELETE(pIndex);

	//�o�[�e�b�N�X�o�b�t�@�[�쐬
	const UINT vbByteSize = (UINT)IndexCount34MeAll * sizeof(MY_VERTEX_S);

	D3DCreateBlob(vbByteSize, &Vview->VertexBufferCPU);
	CopyMemory(Vview->VertexBufferCPU->GetBufferPointer(), pvVB, vbByteSize);
	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
		mCommandList, pvVB, vbByteSize, Vview->VertexBufferUploader);
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

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	mPSO = CreatePsoVsPs(vs, ps, mRootSignature.Get(), dx->pVertexLayout_SKIN, alpha, blend);
	mPSO_B = CreatePsoVsHsDsPs(vsB, hs, ds, psB, mRootSignature.Get(), dx->pVertexLayout_SKIN, alpha, blend);

	GetTexture();
}

void SkinMesh::CreateFromFBX() {
	CreateFromFBX(FALSE);
}

HRESULT SkinMesh::GetFbxSub(CHAR* szFileName, int ind) {
	if (ind <= 0) {
		MessageBox(0, L"FBX���[�_�[���������s", NULL, MB_OK);
		return E_FAIL;
	}

	if (FAILED(InitFBX(szFileName, ind))) {
		MessageBox(0, L"FBX���[�_�[���������s", NULL, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}

HRESULT SkinMesh::GetBuffer_Sub(int ind, float end_frame) {

	FbxScene *pScene = GetScene(ind);//�V�[���擾
	FbxNode *pNodeRoot = pScene->GetRootNode();//���[�g�m�[�h�擾
	fbx[ind].end_frame = end_frame;

	int BoneNum;
	WAIT(BoneNum = SearchNodeCount(pNodeRoot, FbxNodeAttribute::eSkeleton));
	if (BoneNum == 0) {
		MessageBox(0, L"FBX���[�_�[���������s", NULL, MB_OK);
		return E_FAIL;
	}
	WAIT(SearchNodeCount(NULL, FbxNodeAttribute::eSkeleton));//���Z�b�g

	if (!m_ppSubAnimationBone) {
		m_ppSubAnimationBone = new FbxNode*[(FBX_PCS - 1) * m_iNumBone];
	}
	return S_OK;
}

void SkinMesh::CreateFromFBX_SubAnimation(int ind) {

	FbxScene *pScene = GetScene(ind);//�V�[���擾
	FbxNode *pNodeRoot = pScene->GetRootNode();//���[�g�m�[�h�擾

	int loopind = 0;
	int searchCount = 0;
	int name2Num = 0;
	while (loopind < m_iNumBone) {
		int sa_ind = (ind - 1) * m_iNumBone + loopind;
		WAIT(m_ppSubAnimationBone[sa_ind] = SearchNode(pNodeRoot, FbxNodeAttribute::eSkeleton, searchCount));
		searchCount++;
		SearchNode(NULL, FbxNodeAttribute::eSkeleton, 0);//���Z�b�g
		const char *name = m_ppSubAnimationBone[sa_ind]->GetName();
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

void SkinMesh::CreateIndexBuffer(int cnt, int *pIndex, int IviewInd) {

	const UINT ibByteSize = (UINT)cnt * sizeof(UINT);

	D3DCreateBlob(ibByteSize, &Iview[IviewInd].IndexBufferCPU);
	CopyMemory(Iview[IviewInd].IndexBufferCPU->GetBufferPointer(), pIndex, ibByteSize);

	Iview[IviewInd].IndexFormat = DXGI_FORMAT_R32_UINT;
	Iview[IviewInd].IndexBufferByteSize = ibByteSize;
	Iview[IviewInd].IndexCount = cnt;
}

void SkinMesh::CreateIndexBuffer2(int *pIndex, int IviewInd) {

	Iview[IviewInd].IndexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
		mCommandList, pIndex, Iview[IviewInd].IndexBufferByteSize, Iview[IviewInd].IndexBufferUploader);
}

//�{�[�������̃|�[�Y�ʒu�ɃZ�b�g����
bool SkinMesh::SetNewPoseMatrices(float ti, int ind) {

	stSetNewPose_ON = TRUE;
	if (stInitFBX_ON) { stSetNewPose_ON = FALSE; return FALSE; }//FBX���������̓A�j���[�V�������Ȃ�
	if (AnimLastInd == -1)AnimLastInd = ind;//�ŏ��ɕ`�悷��A�j���[�V�����ԍ��ŏ�����
	bool ind_change = FALSE;
	if (AnimLastInd != ind) {//�A�j���[�V�������؂�ւ����
		ind_change = TRUE; AnimLastInd = ind;
		fbx[ind].current_frame = 0.0f;//�A�j���[�V�������؂�ւ���Ă�̂�Matrix�X�V�O�Ƀt���[����0�ɏ�����
	}
	bool frame_end = FALSE;
	fbx[ind].current_frame += ti;
	if (fbx[ind].end_frame <= fbx[ind].current_frame) frame_end = TRUE;

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
	FbxTime time;
	time.SetTime(0, 0, 0, frame / 10, 0, FbxTime::eFrames60);

	bool subanm = TRUE;
	if (ind <= 0 || ind > FBX_PCS - 1)subanm = FALSE;

	FbxMatrix matf0;
	if (!subanm) {
		matf0 = m_ppCluster[0]->GetLink()->EvaluateGlobalTransform(time);
	}
	else {
		matf0 = m_ppSubAnimationBone[(ind - 1) * m_iNumBone]->EvaluateGlobalTransform(time);
	}

	MATRIX mat0_mov;
	MatrixIdentity(&mat0_mov);
	if (fbx[ind].offset) {
		MatrixTranslation(&mat0_mov, fbx[ind].cx, fbx[ind].cy, fbx[ind].cz);
	}
	else {
		MatrixTranslation(&mat0_mov, (float)-matf0.Get(3, 0) + fbx[ind].cx,
			(float)-matf0.Get(3, 1) + fbx[ind].cy,
			(float)-matf0.Get(3, 2) + fbx[ind].cz);
	}

	MATRIX pose;
	for (int i = 0; i < m_iNumBone; i++) {
		FbxMatrix mat;
		if (!subanm) {
			mat = m_ppCluster[i]->GetLink()->EvaluateGlobalTransform(time);
		}
		else {
			mat = m_ppSubAnimationBone[(ind - 1) * m_iNumBone + i]->EvaluateGlobalTransform(time);
		}

		//�����p��test���s���ꍇ,������m_ppCluster[i]->GetTransformLinkMatrix(mat);������(mat��FbxAMatrix)

		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				if (fbx[ind].centering) {
					pose.m[y][x] = (float)mat.Get(y, x);
				}
				else {
					m_BoneArray[i].mNewPose.m[y][x] = (float)mat.Get(y, x);
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
	stSetNewPose_ON = FALSE;
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

void SkinMesh::GetTexture() {

	TextureNo *te = new TextureNo[MateAllpcs];
	for (int i = 0; i < MateAllpcs; i++) {
		te[i].diffuse = m_pMaterial[i].tex_no;
		te[i].normal = m_pMaterial[i].nortex_no;
		te[i].movie = m_on;
	}
	mSrvHeap = CreateSrvHeap(MateAllpcs, texNum, te);

	ARR_DELETE(te);
}

void SkinMesh::CbSwap() {
	Lock();
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = TRUE;//cb,2�v�f����X�V�I��
	}
	sw = 1 - sw;//cb�X���b�v
	dx->ins_no = 0;
	Unlock();
	DrawOn = TRUE;
}

bool SkinMesh::Update(float time, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size) {
	return Update(0, time, x, y, z, r, g, b, a, thetaZ, thetaY, thetaX, size);
}

bool SkinMesh::Update(int ind, float ti, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp) {

	bool frame_end = FALSE;
	dx->MatrixMap(&cb[sw], x, y, z, r, g, b, a, thetaZ, thetaY, thetaX, size, disp, 1.0f, 1.0f, 1.0f, 1.0f);
	if (ti != -1.0f)frame_end = SetNewPoseMatrices(ti, ind);
	MatrixMap_Bone(&sgb[sw]);
	CbSwap();
	return frame_end;
}

void SkinMesh::DrawOff() {
	DrawOn = FALSE;
}

void SkinMesh::Draw() {

	if (!UpOn | !DrawOn)return;

	Lock();
	mObjectCB0->CopyData(0, cb[1 - sw]);
	mObject_BONES->CopyData(0, sgb[1 - sw]);
	Unlock();

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < MateAllpcs; i++) {
		//�g�p����Ă��Ȃ��}�e���A���΍�
		if (m_pMaterial[i].dwNumFace == 0)
		{
			continue;
		}
		mCommandList->IASetIndexBuffer(&Iview[i].IndexBufferView());

		mCommandList->SetGraphicsRootDescriptorTable(0, tex);//(slotRootParameterIndex(shader��registerIndex), DESCRIPTOR_HANDLE)
		tex.Offset(1, dx->mCbvSrvUavDescriptorSize);//�f�X�N���v�^�q�[�v�̃A�h���X�ʒu�I�t�Z�b�g�Ŏ��̃e�N�X�`����ǂݍ��܂���
		if (m_pMaterial[i].nortex_no != -1) {
			mCommandList->IASetPrimitiveTopology(primType_drawB);
			//normalMap�����݂���ꍇdiffuse�̎��Ɋi�[����Ă���
			mCommandList->SetGraphicsRootDescriptorTable(1, tex);
			tex.Offset(1, dx->mCbvSrvUavDescriptorSize);
			mCommandList->SetPipelineState(mPSO_B.Get());//normalMap�L�薳����PSO�؂�ւ�
		}
		else {
			mCommandList->IASetPrimitiveTopology(primType_draw);
			mCommandList->SetPipelineState(mPSO.Get());
		}

		mCommandList->SetGraphicsRootConstantBufferView(2, mObjectCB0->Resource()->GetGPUVirtualAddress());
		UINT mElementByteSize = (sizeof(CONSTANT_BUFFER2) + 255) & ~255;
		mCommandList->SetGraphicsRootConstantBufferView(3, mObjectCB1->Resource()->GetGPUVirtualAddress() + mElementByteSize * i);
		mCommandList->SetGraphicsRootConstantBufferView(4, mObject_BONES->Resource()->GetGPUVirtualAddress());

		mCommandList->DrawIndexedInstanced(Iview[i].IndexCount, 1, 0, 0, 0);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}


