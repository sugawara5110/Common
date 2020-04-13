//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　        SkinMeshクラス(FbxLoader)                           **//
//**                                                                                     **//
//*****************************************************************************************//

//Smoothie3Dでモデル→Blenderでアニメーションの場合,-Z前方,Y上で出力する
//makehumanでモデル→Blenderでアニメーション(カーネギーメロン大学でGETしたモーションキャプチャデータ使用)
//の場合,Y前方,Z上で出力する

#define _CRT_SECURE_NO_WARNINGS
#define FBX_PCS 5
#include "Dx_SkinMesh.h"
#include <string.h>

using namespace std;

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
	addDiffuse = 0.0f;
	addSpecular = 0.0f;
	addAmbient = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;
	divArr[1].distance = 500.0f;
	divArr[1].divide = 5;
	divArr[2].distance = 300.0f;
	divArr[2].divide = 10;
}

SkinMesh::~SkinMesh() {
	ARR_DELETE(pvVB);
	ARR_DELETE(m_pClusterName);
	ARR_DELETE(m_ppSubAnimationBone);
	ARR_DELETE(m_pLastBoneMatrix);
	ARR_DELETE(m_pdwNumVert);
	ARR_DELETE(m_BoneArray);
	ARR_DELETE(m_ppCluster);

	S_DELETE(mObjectCB0);
	S_DELETE(mObjectCB1);
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

void SkinMesh::ReadSkinInfo(MY_VERTEX_S* pvVB) {
	FbxLoader* fbl = fbx[0].fbxL;
	FbxMeshNode* mesh = fbl->getFbxMeshNode(0);//行列関係はどのメッシュからでも同じ物を得られる

	for (int i = 0; i < m_iNumBone; i++) {
		m_ppCluster[i] = mesh->getDeformer(i);
		const char* name = m_ppCluster[i]->getName();
		strcpy(&m_pClusterName[i * 255], name);//ボーンの名前保持
	}

	int VertexStart = 0;
	for (int m = 0; m < NodeArraypcs; m++) {
		FbxMeshNode* meshBone = fbl->getFbxMeshNode(m);

		//各Boneのウエイト,インデックスを調べ頂点配列に加える
		for (int i = 0; i < m_iNumBone; i++) {
			Deformer* defo = meshBone->getDeformer(i);
			int iNumIndex = defo->getIndicesCount();//このボーンに影響を受ける頂点インデックス数
			int* piIndex = defo->getIndices();     //このボーンに影響を受ける頂点のインデックス配列
			double* pdWeight = defo->getWeights();//このボーンに影響を受ける頂点のウエイト配列

			for (int k = 0; k < iNumIndex; k++) {
				int index = piIndex[k];      //影響を受ける頂点
				double weight = pdWeight[k];//ウエイト
				for (int m = 0; m < 4; m++) {
					//各Bone毎に影響を受ける頂点のウエイトを一番大きい数値に更新していく
					if (weight > pvVB[index + VertexStart].bBoneWeight[m]) {//調べたウエイトの方が大きい
						pvVB[index + VertexStart].bBoneIndex[m] = i;//Boneインデックス登録
						pvVB[index + VertexStart].bBoneWeight[m] = (float)weight;//ウエイト登録
						break;
					}
				}
			}
		}
		VertexStart += m_pdwNumVert[m];
	}

	//ウエイト正規化
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
		//初期姿勢行列読み込み
		//GetCurrentPoseMatrixで使う
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
	//FBXローダーを初期化
	if (FAILED(InitFBX(szFileName, 0)))
	{
		MessageBox(0, L"FBXローダー初期化失敗", nullptr, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}

void SkinMesh::GetBuffer(float end_frame) {

	mObjectCB0 = new ConstantBuffer<CONSTANT_BUFFER>(1);
	mObject_BONES = new ConstantBuffer<SHADER_GLOBAL_BONES>(1);

	fbx[0].end_frame = end_frame;
	FbxLoader* fbL = fbx[0].fbxL;
	NodeArraypcs = fbL->getNumFbxMeshNode();

	//事前にメッシュ毎頂点数、ポリゴン数マテリアル数等を調べる
	m_pdwNumVert = new DWORD[NodeArraypcs];
	m_pMaterialCount = new DWORD[NodeArraypcs];
	VerAllpcs = 0;//頂点全数
	pdwNumFace = new DWORD[NodeArraypcs];

	//カウント
	for (int i = 0; i < NodeArraypcs; i++) {
		FbxMeshNode* mesh = fbL->getFbxMeshNode(i);
		m_pdwNumVert[i] = mesh->getNumVertices();//メッシュ毎頂点数
		m_pMaterialCount[i] = mesh->getNumMaterial();//メッシュ毎マテリアル数
		VerAllpcs += m_pdwNumVert[i];               //頂点全数カウント
		dpara.NumMaterial += m_pMaterialCount[i];        //マテリアル全数カウント
		pdwNumFace[i] = mesh->getNumPolygon();   //メッシュ毎ポリゴン数
	}

	//メッシュ毎4頂点分割前インデックス配列生成
	IndexCount34Me = new int[NodeArraypcs];
	//マテリアル毎4頂点分割後インデックス配列生成
	IndexCount3M = new int[dpara.NumMaterial];

	//マテリアル配列生成
	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);

	//マテリアルコンスタントバッファ
	mObjectCB1 = new ConstantBuffer<CONSTANT_BUFFER2>(dpara.NumMaterial);

	//VBO
	dpara.Vview = std::make_unique<VertexView>();
	//IBO マテリアル毎に生成する
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);

	//インデックス数計算(ポリゴン1個に付き頂点 2個～5個が存在することが有るが
	//fbxからIndex配列コピー時に全てコピーするのでそのままの個数をカウントする)
	IndexCount34MeAll = 0;
	for (int k = 0; k < NodeArraypcs; k++) {
		IndexCount34Me[k] = 0;
		FbxMeshNode* mesh = fbL->getFbxMeshNode(k);
		for (DWORD i = 0; i < pdwNumFace[k]; i++) {
			IndexCount34Me[k] += mesh->getPolygonSize(i);//メッシュ毎の分割前頂点インデックス数
		}
		IndexCount34MeAll += IndexCount34Me[k];
	}

	int mInd = 0;
	for (int k = 0; k < NodeArraypcs; k++) {
		FbxMeshNode* mesh = fbL->getFbxMeshNode(k);
		for (DWORD j = 0; j < m_pMaterialCount[k]; j++) {
			IndexCount3M[mInd] = 0;
			for (DWORD i = 0; i < pdwNumFace[k]; i++) {
				int matId = mesh->getMaterialNoOfPolygon(i);//ポリゴンiの関連するマテリアル番号取得
				if (matId == j) {//現在処理中のマテリアルと一致した場合以下処理実行(1Meshに2以上のマテリアルが有る場合の処理)
					int pcs = mesh->getPolygonSize(i);//各ポリゴンの頂点インデックス数
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

	pIndex = new int* [dpara.NumMaterial];
	for (int i = 0; i < dpara.NumMaterial; i++) {
		pIndex[i] = new int[IndexCount3M[i]];
	}

	FbxMeshNode* mesh = fbL->getFbxMeshNode(0);
	m_iNumBone = mesh->getNumDeformer();//どのメッシュからでも同じボーン数が得られているようだ
	m_ppCluster = new  Deformer * [m_iNumBone];//ボーン情報配列
	m_pClusterName = new char[m_iNumBone * 255];

	//ボーンを生成
	m_BoneArray = new BONE[m_iNumBone];
	m_pLastBoneMatrix = new MATRIX[m_iNumBone];
}

void SkinMesh::SetVertex() {
	//4頂点分割前状態でのインデックス数で頂点配列生成
	//4頂点ポリゴンは分割後法線変化無しの為, 頂点は3頂点,4頂点混在状態の要素数で生成
	pvVB = new MY_VERTEX_S[IndexCount34MeAll];

	MY_VERTEX_S* tmpVB = new MY_VERTEX_S[VerAllpcs];
	//スキン情報(ジョイント, ウェイト)
	ReadSkinInfo(tmpVB);

	//同一座標頂点リスト
	SameVertexList* svList = new SameVertexList[VerAllpcs];

	FbxLoader* fbL = fbx[0].fbxL;
	//メッシュ毎に配列格納処理
	int mInd = 0;//マテリアル内カウント
	DWORD VerArrStart = 0;//分割前インデックス数(最終的な頂点数)
	DWORD VerArrStart2 = 0;//読み込み時頂点数
	for (int m = 0; m < NodeArraypcs; m++) {
		FbxMeshNode* mesh = fbL->getFbxMeshNode(m);
		int* piIndex = mesh->getPolygonVertices();//fbxから頂点インデックス配列取得

		//頂点配列をfbxからコピー
		double* pCoord = mesh->getVertices();
		for (int i = 0; i < IndexCount34Me[m]; i++) {
			//fbxから読み込んだindex順で頂点を整列しながら頂点格納
			pvVB[i + VerArrStart].vPos.x = (float)pCoord[piIndex[i] * 3 + 0];
			pvVB[i + VerArrStart].vPos.y = (float)pCoord[piIndex[i] * 3 + 1];
			pvVB[i + VerArrStart].vPos.z = (float)pCoord[piIndex[i] * 3 + 2];
			svList[piIndex[i] + VerArrStart2].Push(i + VerArrStart);
			for (int bi = 0; bi < 4; bi++) {
				//ReadSkinInfo(tmpVB)で読み込んだ各パラメータコピー
				pvVB[i + VerArrStart].bBoneIndex[bi] = tmpVB[piIndex[i] + VerArrStart2].bBoneIndex[bi];
				pvVB[i + VerArrStart].bBoneWeight[bi] = tmpVB[piIndex[i] + VerArrStart2].bBoneWeight[bi];
			}
		}

		//法線, テクスチャー座標読み込み
		int IndCount = 0;
		int NorCount = 0;
		int UVCount = 0;
		double* Normal = mesh->getNormal();
		double* UV = mesh->getAlignedUV();
		for (DWORD i = 0; i < pdwNumFace[m]; i++) {
			//int iStartIndex = pFbxMesh->GetPolygonVertexIndex(i);//ポリゴンを構成する最初のインデックス取得
			int pcs = mesh->getPolygonSize(i);//各ポリゴンの頂点数
			if (pcs != 3 && pcs != 4)continue;//頂点数3個,4個以外はスキップ
			int iVert0Index, iVert1Index, iVert2Index, iVert3Index;

			//法線,UV読み込み,indexで読み込み
			//頂点はindexで整列済み
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
			//マテリアル情報取得
			//拡散反射光
			dpara.material[mInd].Kd.x = (float)mesh->getDiffuseColor(0, 0);
			dpara.material[mInd].Kd.y = (float)mesh->getDiffuseColor(0, 1);
			dpara.material[mInd].Kd.z = (float)mesh->getDiffuseColor(0, 2);
			dpara.material[mInd].Kd.w = 0.0f;//使用してない
			//スペキュラー
			dpara.material[mInd].Ks.x = (float)mesh->getSpecularColor(0, 0);
			dpara.material[mInd].Ks.y = (float)mesh->getSpecularColor(0, 1);
			dpara.material[mInd].Ks.z = (float)mesh->getSpecularColor(0, 2);
			dpara.material[mInd].Ks.w = 0.0f;//使用してない
			//アンビエント
			dpara.material[mInd].Ka.x = (float)mesh->getAmbientColor(0, 0);
			dpara.material[mInd].Ka.y = (float)mesh->getAmbientColor(0, 1);
			dpara.material[mInd].Ka.z = (float)mesh->getAmbientColor(0, 2);
			dpara.material[mInd].Ka.w = 0.0f;//使用してない

			//テクスチャー
			if (mesh->getNormalTextureName(i)) {
				strcpy_s(dpara.material[mInd].norTextureName, mesh->getNormalTextureName(i));
				//ファイル名を元に既にデコード済みのテクスチャ番号を読み込む
				dpara.material[mInd].nortex_no = dx->GetTexNumber(dpara.material[mInd].norTextureName);
			}
			if (mesh->getDiffuseTextureName(i)) {
				strcpy_s(dpara.material[mInd].szTextureName, mesh->getDiffuseTextureName(i));
				//ファイル名を元に既にデコード済みのテクスチャ番号を読み込む
				dpara.material[mInd].tex_no = dx->GetTexNumber(dpara.material[mInd].szTextureName);
			}
			else {
				strcpy_s(dpara.material[mInd].szTextureName, mesh->getMaterialName());//テクスチャ名が無い場合マテリアル名から
				dpara.material[mInd].tex_no = dx->GetTexNumber(dpara.material[mInd].szTextureName);
			}

			int iCount = 0;//最終的な(分割後)indexカウント
			int vCount = 0;//頂点カウント
			int polygon_cnt = 0;
			for (DWORD k = 0; k < pdwNumFace[m]; k++) {
				int matId = mesh->getMaterialNoOfPolygon(k);//ポリゴンkの関連するマテリアル番号取得
				int pcs = mesh->getPolygonSize(k);//各ポリゴンの頂点数
				if (pcs != 3 && pcs != 4)continue;//頂点数3個,4個以外はスキップ
				if (matId == i)//現在処理中のマテリアルと一致した場合以下処理実行(1Meshに2以上のマテリアルが有る場合の処理)
				{
					pIndex[mInd][iCount] = vCount + VerArrStart;//vpVBは分割前index順に整列済み
					pIndex[mInd][iCount + 1] = vCount + 1 + VerArrStart;
					pIndex[mInd][iCount + 2] = vCount + 2 + VerArrStart;
					iCount += 3;

					//4頂点は{ 0, 1, 2 }, { 0, 2, 3 }で分割するよう処理
					if (pcs == 4) {
						pIndex[mInd][iCount] = vCount + VerArrStart;
						pIndex[mInd][iCount + 1] = vCount + 2 + VerArrStart;
						pIndex[mInd][iCount + 2] = vCount + 3 + VerArrStart;
						iCount += 3;//4頂点を分割後はインデックスは3頂点の倍
						vCount++;
					}
					vCount += 3;
					polygon_cnt++;
				}
			}

			//インデックスバッファ生成1段回目
			if (iCount > 0) CreateIndexBuffer(iCount, mInd);
			dpara.material[mInd].dwNumFace = polygon_cnt;//そのマテリアル内のポリゴン数	
			mInd++;
		}
		VerArrStart += IndexCount34Me[m];
		VerArrStart2 += m_pdwNumVert[m];
	}

	//同一座標頂点の法線統一化(テセレーション用)
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

	//各一時格納用配列解放
	ARR_DELETE(m_pMaterialCount);
	ARR_DELETE(IndexCount34Me);
	ARR_DELETE(IndexCount3M);
	ARR_DELETE(pdwNumFace);
	ARR_DELETE(tmpVB);
	ARR_DELETE(svList);

	for (int i = 0; i < dpara.NumMaterial; i++) {
		CONSTANT_BUFFER2 sg;
		dpara.material[i].Kd.x += addDiffuse;
		dpara.material[i].Kd.y += addDiffuse;
		dpara.material[i].Kd.z += addDiffuse;
		dpara.material[i].Ks.x += addSpecular;
		dpara.material[i].Ks.y += addSpecular;
		dpara.material[i].Ks.z += addSpecular;
		dpara.material[i].Ka.x += addAmbient;
		dpara.material[i].Ka.y += addAmbient;
		dpara.material[i].Ka.z += addAmbient;
		sg.vDiffuse = dpara.material[i].Kd;//ディフューズカラーをシェーダーに渡す
		sg.vSpeculer = dpara.material[i].Ks;//スペキュラーをシェーダーに渡す
		sg.vAmbient = dpara.material[i].Ka;//アンビエントをシェーダーに渡す
		mObjectCB1->CopyData(i, sg);
	}
}

void SkinMesh::SetDiffuseTextureName(char* textureName, int materialIndex) {
	if (dpara.material[materialIndex].tex_no != -1)return;//既に設定済みの場合無効
	strcpy_s(dpara.material[materialIndex].szTextureName, textureName);
	//ファイル名を元に既にデコード済みのテクスチャ番号を読み込む
	dpara.material[materialIndex].tex_no = dx->GetTexNumber(dpara.material[materialIndex].szTextureName);
}

void SkinMesh::SetNormalTextureName(char* textureName, int materialIndex) {
	if (dpara.material[materialIndex].nortex_no != -1)return;
	strcpy_s(dpara.material[materialIndex].norTextureName, textureName);
	//ファイル名を元に既にデコード済みのテクスチャ番号を読み込む
	dpara.material[materialIndex].nortex_no = dx->GetTexNumber(dpara.material[materialIndex].norTextureName);
}

bool SkinMesh::CreateFromFBX(bool disp) {
	//インデックスバッファ生成2段回目, 一時格納配列解放
	for (int i = 0; i < dpara.NumMaterial; i++) {
		CreateIndexBuffer2(pIndex[i], i);
		ARR_DELETE(pIndex[i]);
	}
	ARR_DELETE(pIndex);

	//バーテックスバッファー作成
	const UINT vbByteSize = (UINT)IndexCount34MeAll * sizeof(MY_VERTEX_S);

	dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, pvVB, vbByteSize, dpara.Vview->VertexBufferUploader);
	dpara.Vview->VertexByteStride = sizeof(MY_VERTEX_S);
	dpara.Vview->VertexBufferByteSize = vbByteSize;

	if (pvVB_delete_f)ARR_DELETE(pvVB);//使わない場合解放

	if (disp) {
		vs = dx->pVertexShader_SKIN_D.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
		primType_create = CONTROL_POINT;
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}
	else {
		vs = dx->pVertexShader_SKIN.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
		primType_create = SQUARE;
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	ps = dx->pPixelShader_3D.Get();

	CD3DX12_DESCRIPTOR_RANGE texTable, nortexTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//このDescriptorRangeはシェーダーリソースビュー,Descriptor 1個, shader内registerIndex
	nortexTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);//DescriptorRangeの数は1つ, DescriptorRangeの先頭アドレス
	slotRootParameter[1].InitAsDescriptorTable(1, &nortexTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[2].InitAsConstantBufferView(0);
	slotRootParameter[3].InitAsConstantBufferView(1);
	slotRootParameter[4].InitAsConstantBufferView(2);

	dpara.rootSignature = CreateRs(5, slotRootParameter);
	if (dpara.rootSignature == nullptr)return false;

	//パイプラインステートオブジェクト生成
	dpara.PSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(), dx->pVertexLayout_SKIN, alpha, blend, primType_create);
	if (dpara.PSO == nullptr)return false;

	return GetTexture();
}

bool SkinMesh::CreateFromFBX() {
	return CreateFromFBX(false);
}

HRESULT SkinMesh::GetFbxSub(CHAR* szFileName, int ind) {
	if (ind <= 0) {
		MessageBox(0, L"FBXローダー初期化失敗", nullptr, MB_OK);
		return E_FAIL;
	}

	if (FAILED(InitFBX(szFileName, ind))) {
		MessageBox(0, L"FBXローダー初期化失敗", nullptr, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}

HRESULT SkinMesh::GetBuffer_Sub(int ind, float end_frame) {
	fbx[ind].end_frame = end_frame;

	int BoneNum = fbx[ind].fbxL->getNumNoneMeshDeformer();
	if (BoneNum == 0) {
		MessageBox(0, L"FBXローダー初期化失敗", nullptr, MB_OK);
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
		char *name2 = &m_pClusterName[loopind * 255];//各Bone名の先頭アドレスを渡す
		//Bone名に空白が含まれている場合最後の空白以降の文字から終端までの文字を比較する為,
		//終端文字までポインタを進め, 終端から検査して空白位置手前まで比較する
		while (*name != '\0')name++;//終端文字までポインタを進める
		//終端文字までポインタを進める, 空白が含まれない文字の場合もあるので文字数カウントし,
		//文字数で比較完了を判断する
		while (*name2 != '\0') { name2++; name2Num++; }
		while (*(--name) == *(--name2) && *name2 != ' ' && (--name2Num) > 0);
		if (*name2 != ' ' && name2Num > 0) { name2Num = 0; continue; }
		name2Num = 0;
		loopind++;
	}
}

void SkinMesh::CreateIndexBuffer(int cnt, int IviewInd) {

	const UINT ibByteSize = (UINT)cnt * sizeof(UINT);

	dpara.Iview[IviewInd].IndexFormat = DXGI_FORMAT_R32_UINT;
	dpara.Iview[IviewInd].IndexBufferByteSize = ibByteSize;
	dpara.Iview[IviewInd].IndexCount = cnt;
}

void SkinMesh::CreateIndexBuffer2(int* pIndex, int IviewInd) {

	dpara.Iview[IviewInd].IndexBufferGPU = dx->CreateDefaultBuffer(com_no, pIndex,
		dpara.Iview[IviewInd].IndexBufferByteSize, dpara.Iview[IviewInd].IndexBufferUploader);
}

//ボーンを次のポーズ位置にセットする
bool SkinMesh::SetNewPoseMatrices(float ti, int ind) {
	if (AnimLastInd == -1)AnimLastInd = ind;//最初に描画するアニメーション番号で初期化
	bool ind_change = false;
	if (AnimLastInd != ind) {//アニメーションが切り替わった
		ind_change = true; AnimLastInd = ind;
		fbx[ind].current_frame = 0.0f;//アニメーションが切り替わってるのでMatrix更新前にフレームを0に初期化
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

//ポーズ行列を返す
MATRIX SkinMesh::GetCurrentPoseMatrix(int index) {
	MATRIX inv;
	MatrixIdentity(&inv);
	MatrixInverse(&inv, &m_BoneArray[index].mBindPose);//FBXのバインドポーズは初期姿勢（絶対座標）
	MATRIX ret;
	MatrixIdentity(&ret);
	MatrixMultiply(&ret, &inv, &m_BoneArray[index].mNewPose);//バインドポーズの逆行列とフレーム姿勢行列をかける

	return ret;
}

VECTOR3 SkinMesh::GetVertexPosition(int verNum, float adjustZ, float adjustY, float adjustX, float thetaZ, float thetaY, float thetaX, float scal) {

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

	TextureNo* te = new TextureNo[dpara.NumMaterial];
	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].tex_no < 0)te[i].diffuse = 0; else
			te[i].diffuse = dpara.material[i].tex_no;
		if (dpara.material[i].nortex_no < 0)te[i].normal = 0; else
			te[i].normal = dpara.material[i].nortex_no;
	}

	createTextureResource(dpara.NumMaterial, te);
	int numTex = dpara.NumMaterial * 2;
	dpara.srvHeap = CreateSrvHeap(numTex, te);

	ARR_DELETE(te);
	if (dpara.srvHeap == nullptr)return false;

	return true;
}

void SkinMesh::CbSwap() {
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	DrawOn = true;
}

bool SkinMesh::Update(float time, float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size) {
	return Update(0, time, x, y, z, r, g, b, a, thetaZ, thetaY, thetaX, size);
}

bool SkinMesh::Update(int ind, float ti, float x, float y, float z, float r, float g, float b, float a,
	float thetaZ, float thetaY, float thetaX, float size, float disp, float shininess) {

	bool frame_end = false;
	int insnum = 0;
	dx->InstancedMap(insnum, &cb[dx->cBuffSwap[0]], x, y, z, thetaZ, thetaY, thetaX, size);
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f, divArr, numDiv, shininess);
	if (ti != -1.0f)frame_end = SetNewPoseMatrices(ti, ind);
	MatrixMap_Bone(&sgb[dx->cBuffSwap[0]]);
	CbSwap();
	return frame_end;
}

void SkinMesh::DrawOff() {
	DrawOn = false;
}

void SkinMesh::Draw() {

	if (!UpOn | !DrawOn)return;

	mObjectCB0->CopyData(0, cb[dx->cBuffSwap[1]]);
	mObject_BONES->CopyData(0, sgb[dx->cBuffSwap[1]]);

	dpara.cbRes0 = mObjectCB0->Resource();
	dpara.cbRes1 = mObjectCB1->Resource();
	dpara.cbRes2 = mObject_BONES->Resource();
	dpara.sRes0 = nullptr;
	dpara.sRes1 = nullptr;
	dpara.insNum = 1;
	drawsub(dpara);
}


