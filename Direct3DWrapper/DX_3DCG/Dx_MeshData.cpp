//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          MeshDataクラス                                    **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_MeshData.h"

MeshData::MeshData() {
	addDiffuse = 0.0f;
	addSpecular = 0.0f;
	addAmbient = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;
	divArr[1].distance = 500.0f;
	divArr[1].divide = 6;
	divArr[2].distance = 300.0f;
	divArr[2].divide = 12;
	numDiv = 3;
}

MeshData::~MeshData() {

}

ID3D12PipelineState* MeshData::GetPipelineState(int index) {
	return mObj.dpara.PSO[index].Get();
}

bool MeshData::LoadMaterialFromFile(char* FileName, int numMaxInstance) {
	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	//マテリアルファイルを開いて内容を読み込む
	errno_t error;
	FILE* fp = nullptr;
	error = fopen_s(&fp, FileName, "rt");
	if (error != 0) {
		Dx_Util::ErrorMessage("MeshData::LoadMaterialFromFile fopen_s Error");
		return false;
	}
	char line[200] = { 0 };//1行読み込み用
	char key[110] = { 0 };//1単語読み込み用
	CoordTf::VECTOR4 v = { 0, 0, 0, 1 };

	//マテリアル数を調べる
	while (!feof(fp))
	{
		//キーワード読み込み
		fgets(line, sizeof(line), fp);
		sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));
		//マテリアル名
		if (strcmp(key, "newmtl") == 0)
		{
			mObj.dpara.NumMaterial++;
		}
	}

	mObj.getBuffer(mObj.dpara.NumMaterial, numMaxInstance, divArr, numDiv);
	mMat = std::make_unique<meshMaterial[]>(mObj.dpara.NumMaterial);

	//本読み込み	
	fseek(fp, 0, SEEK_SET);
	INT iMCount = -1;

	while (!feof(fp))
	{
		//キーワード読み込み
		fgets(line, sizeof(line), fp);//1行読み込みlineに格納,FILEポインタ1行進む
		sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));//読み込んだ1行から"%s"最初の文字列1個読み込み
		//マテリアル名
		if (strcmp(key, "newmtl") == 0)
		{
			iMCount++;
			sscanf_s(&line[7], "%s ", key, (unsigned int)sizeof(key));//lineの7要素目(newmtl)の直後から1個目の文字列をkeyに格納
			strcpy_s(mMat[iMCount].szName, key);
		}
		//Kd　ディフューズ
		if (strcmp(key, "Kd") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
			mObj.dpara.material[iMCount].diffuse = v;
		}
		//Ks　スペキュラー
		if (strcmp(key, "Ks") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
			mObj.dpara.material[iMCount].specular = v;
		}
		//Ka　アンビエント
		if (strcmp(key, "Ka") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
			mObj.dpara.material[iMCount].ambient = v;
		}
		//map_Kd　テクスチャー
		if (strcmp(key, "map_Kd") == 0)
		{
			sscanf_s(&line[7], "%s", &mMat[iMCount].szTextureName, (unsigned int)sizeof(mMat[iMCount].szTextureName));
			mObj.dpara.material[iMCount].diftex_no = dx->GetTexNumber(mMat[iMCount].szTextureName);
		}
		//map_bump　テクスチャー
		if (strcmp(key, "map_bump") == 0)
		{
			sscanf_s(&line[7], "%s", &mMat[iMCount].norTextureName, (unsigned int)sizeof(mMat[iMCount].norTextureName));
			mObj.dpara.material[iMCount].nortex_no = dx->GetTexNumber(mMat[iMCount].norTextureName);
		}
	}
	fclose(fp);
	return true;
}

void MeshData::SetState(bool al, bool bl, bool di, float diffuse, float specu, float ambi) {
	alpha = al;
	blend = bl;
	disp = di;
	addDiffuse = diffuse;
	addSpecular = specu;
	addAmbient = ambi;
}

bool MeshData::GetBuffer(char* FileName, int numMaxInstance) {

	if (0 != strcpy_s(mFileName, FileName)) {
		Dx_Util::ErrorMessage("MeshData::GetBuffer strcpy_s Error");
		return false;
	}

	int VCount = 0;//読み込みカウンター
	int VNCount = 0;//読み込みカウンター
	int VTCount = 0;//読み込みカウンター
	FaceCount = 0;//ポリゴン数カウンター

	char line[200] = { 0 };
	char key[200] = { 0 };
	//OBJファイルを開いて内容を読み込む
	errno_t error;
	FILE* fp = nullptr;
	error = fopen_s(&fp, mFileName, "rt");
	if (error != 0) {
		Dx_Util::ErrorMessage("MeshData::GetBuffer fopen_s Error");
		return false;
	}

	//事前に頂点数、ポリゴン数を調べる
	while (!feof(fp))
	{
		//キーワード読み込み
		fgets(line, sizeof(line), fp);
		sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));
		//マテリアル読み込み
		if (strcmp(key, "mtllib") == 0)
		{
			sscanf_s(&line[7], "%s ", key, (unsigned int)sizeof(key));
			if (!LoadMaterialFromFile(key, numMaxInstance))return false;
		}
		//頂点
		if (strcmp(key, "v") == 0)
		{
			VCount++;
		}
		//法線
		if (strcmp(key, "vn") == 0)
		{
			VNCount++;
		}
		//テクスチャー座標
		if (strcmp(key, "vt") == 0)
		{
			VTCount++;
		}
		//フェイス（ポリゴン）
		if (strcmp(key, "f") == 0)
		{
			FaceCount++;
		}
	}
	fclose(fp);

	//一時的なメモリ確保
	using namespace CoordTf;
	pvCoord = NEW VECTOR3[VCount];
	pvNormal = NEW VECTOR3[VNCount];
	pvTexture = NEW VECTOR2[VTCount];

	piFaceBuffer = NEW UINT * [mObj.dpara.NumMaterial];
	for (int i = 0; i < mObj.dpara.NumMaterial; i++)
		piFaceBuffer[i] = NEW UINT[FaceCount * 3];//3頂点なので3インデックス * Material個数
	pvVertexBuffer = NEW VertexM[FaceCount * 3];

	return true;
}

bool MeshData::SetVertex() {

	float x, y, z;
	int v1 = 0, v2 = 0, v3 = 0;
	int vn1 = 0, vn2 = 0, vn3 = 0;
	int vt1 = 0, vt2 = 0, vt3 = 0;
	int VCount = 0;//読み込みカウンター
	int VNCount = 0;//読み込みカウンター
	int VTCount = 0;//読み込みカウンター

	char line[200] = { 0 };
	char key[200] = { 0 };
	//OBJファイルを開いて内容を読み込む
	errno_t error;
	FILE* fp = nullptr;
	error = fopen_s(&fp, mFileName, "rt");
	if (error != 0) {
		Dx_Util::ErrorMessage("MeshData::SetVertex fopen_s Error");
		return false;
	}
	//本読み込み	
	while (!feof(fp))
	{
		//キーワード 読み込み
		ZeroMemory(key, sizeof(key));
		fgets(line, sizeof(line), fp);
		sscanf_s(line, "%s", key, (unsigned int)sizeof(key));

		//頂点 読み込み
		if (strcmp(key, "v") == 0)
		{
			sscanf_s(&line[2], "%f %f %f", &x, &y, &z);
			pvCoord[VCount].x = x;
			pvCoord[VCount].y = z;
			pvCoord[VCount].z = y;
			VCount++;
		}

		//法線 読み込み
		if (strcmp(key, "vn") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &x, &y, &z);
			pvNormal[VNCount].x = x;
			pvNormal[VNCount].y = z;
			pvNormal[VNCount].z = y;
			VNCount++;
		}

		//テクスチャー座標 読み込み
		if (strcmp(key, "vt") == 0)
		{
			sscanf_s(&line[3], "%f %f", &x, &y);
			pvTexture[VTCount].x = x;
			pvTexture[VTCount].y = 1 - y;//OBJファイルはY成分が逆なので合わせる
			VTCount++;
		}
	}

	using namespace CoordTf;
	//同一座標頂点リスト
	SameVertexList* svList = NEW SameVertexList[VCount];

	for (int i = 0; i < mObj.dpara.NumMaterial; i++) {
		CONSTANT_BUFFER2 sg;

		VECTOR4* diffuse = &mObj.dpara.material[i].diffuse;
		diffuse->x += addDiffuse;
		diffuse->y += addDiffuse;
		diffuse->z += addDiffuse;
		VECTOR4* specular = &mObj.dpara.material[i].specular;
		specular->x += addSpecular;
		specular->y += addSpecular;
		specular->z += addSpecular;
		VECTOR4* ambient = &mObj.dpara.material[i].ambient;
		ambient->x += addAmbient;
		ambient->y += addAmbient;
		ambient->z += addAmbient;
		sg.vDiffuse = *diffuse;//ディフューズカラーをシェーダーに渡す
		sg.vSpeculer = *specular;//スペキュラーをシェーダーに渡す
		sg.vAmbient = *ambient;//アンビエントをシェーダーに渡す
		mObj.mObjectCB2->CopyData(i, sg);
		if (Dx_Device::GetInstance()->getDxrCreateResourceState()) {
			mObj.setColorDXR(i, sg);
		}
	}

	//フェイス　読み込み　バラバラに収録されている可能性があるので、マテリアル名を頼りにつなぎ合わせる
	bool boFlag = false;

	int FCount = 0;
	int dwPartFCount = 0;
	for (int i = 0; i < mObj.dpara.NumMaterial; i++)
	{
		dwPartFCount = 0;
		fseek(fp, 0, SEEK_SET);

		while (!feof(fp))
		{
			//キーワード 読み込み
			ZeroMemory(key, sizeof(key));
			fgets(line, sizeof(line), fp);
			sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));

			//フェイス 読み込み→頂点インデックスに
			if (strcmp(key, "usemtl") == 0)
			{
				sscanf_s(&line[7], "%s ", key, (unsigned int)sizeof(key));
				if (strcmp(key, mMat[i].szName) == 0)
				{
					boFlag = true;
				}
				else
				{
					boFlag = false;
				}
			}
			if (strcmp(key, "f") == 0 && boFlag == true)
			{
				if (mObj.dpara.material[i].diftex_no != -1)//テクスチャーありサーフェイス
				{
					sscanf_s(&line[2], "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				}
				else//テクスチャー無しサーフェイス
				{
					sscanf_s(&line[2], "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
					vt1 = vt2 = vt3 = 1;//↓エラー防止
				}

				//インデックスバッファー
				piFaceBuffer[i][dwPartFCount * 3] = FCount * 3;
				piFaceBuffer[i][dwPartFCount * 3 + 1] = FCount * 3 + 1;
				piFaceBuffer[i][dwPartFCount * 3 + 2] = FCount * 3 + 2;
				//頂点構造体に代入
				pvVertexBuffer[FCount * 3].Pos = pvCoord[v1 - 1];
				pvVertexBuffer[FCount * 3].normal = pvNormal[vn1 - 1];
				pvVertexBuffer[FCount * 3].geoNormal = pvNormal[vn1 - 1];
				pvVertexBuffer[FCount * 3].tex = pvTexture[vt1 - 1];
				svList[v1 - 1].Push(FCount * 3);

				pvVertexBuffer[FCount * 3 + 1].Pos = pvCoord[v2 - 1];
				pvVertexBuffer[FCount * 3 + 1].normal = pvNormal[vn2 - 1];
				pvVertexBuffer[FCount * 3 + 1].geoNormal = pvNormal[vn2 - 1];
				pvVertexBuffer[FCount * 3 + 1].tex = pvTexture[vt2 - 1];
				svList[v2 - 1].Push(FCount * 3 + 1);

				pvVertexBuffer[FCount * 3 + 2].Pos = pvCoord[v3 - 1];
				pvVertexBuffer[FCount * 3 + 2].normal = pvNormal[vn3 - 1];
				pvVertexBuffer[FCount * 3 + 2].geoNormal = pvNormal[vn3 - 1];
				pvVertexBuffer[FCount * 3 + 2].tex = pvTexture[vt3 - 1];
				svList[v3 - 1].Push(FCount * 3 + 2);

				dwPartFCount++;
				FCount++;
			}
		}
		if (dwPartFCount == 0)//使用されていないマテリアル対策が必要な場合処理追加。Drawにも
		{
			continue;
		}

		const UINT ibByteSize = (UINT)dwPartFCount * 3 * sizeof(UINT);
		mObj.getIndexBuffer(i, ibByteSize, dwPartFCount * 3);
	}
	fclose(fp);

	//同一座標頂点の法線統一化(テセレーション用)
	SameVertexListNormalization svn;
	svn.Normalization(pvVertexBuffer, sizeof(VertexM), sizeof(VECTOR3) * 3, VCount, svList);

	mObj.getVertexBuffer(sizeof(VertexM), FCount * 3);

	//一時的変数解放
	ARR_DELETE(pvCoord);
	ARR_DELETE(pvNormal);
	ARR_DELETE(pvTexture);
	ARR_DELETE(svList);

	return true;
}

void MeshData::setMaterialType(MaterialType type, int materialIndex) {
	if (materialIndex == -1) {
		mObj.dxrPara.setAllMaterialType(type);
		return;
	}
	mObj.dxrPara.mType[materialIndex] = type;
}

void MeshData::setPointLight(int materialIndex, int InstanceIndex, bool on_off,
	float range, CoordTf::VECTOR3 atten, float plight_RandArea, int plight_numRandRay) {

	Dx_Device* dev = Dx_Device::GetInstance();
	mObj.dxrPara.setPointLight(dev->dxrBuffSwapIndex(), 0, materialIndex, InstanceIndex, on_off, range, atten,
		plight_RandArea, plight_numRandRay);
}

void MeshData::setPointLightAll(bool on_off,
	float range, CoordTf::VECTOR3 atten, float plight_RandArea, int plight_numRandRay) {

	Dx_Device* dev = Dx_Device::GetInstance();
	mObj.dxrPara.setPointLightAll(dev->dxrBuffSwapIndex(), on_off, range, atten,
		plight_RandArea, plight_numRandRay);
}

bool MeshData::CreateMesh(int comIndex, bool smooth, float divideBufferMagnification) {

	if (disp) {
		mObj.GetShaderByteCode(CONTROL_POINT, true, smooth, false, nullptr, nullptr);
	}
	else {
		mObj.GetShaderByteCode(SQUARE, true, smooth, false, nullptr, nullptr);
	}

	const int numSrvTex = 3;
	const int numCbv = 1;
	mObj.setDivideArr(divArr, numDiv);

	UINT* indexCntArr = NEW UINT[mObj.dpara.NumMaterial];
	for (int m = 0; m < mObj.dpara.NumMaterial; m++) {
		indexCntArr[m] = mObj.dpara.Iview[m].IndexCount;
	}
	Dx_Util::createTangent(mObj.dpara.NumMaterial, indexCntArr,
		pvVertexBuffer, piFaceBuffer, sizeof(VertexM), 0, 3 * 4, 12 * 4, 6 * 4);
	ARR_DELETE(indexCntArr);

	mObj.createDefaultBuffer(comIndex, pvVertexBuffer, piFaceBuffer);
	ARR_DELETE(pvVertexBuffer);
	for (int i = 0; i < mObj.dpara.NumMaterial; i++) {
		ARR_DELETE(piFaceBuffer[i]);
	}
	ARR_DELETE(piFaceBuffer);
	int numUav = 0;

	if (!mObj.createTexResource(comIndex))return false;
	if (!mObj.createPSO(Dx_ShaderHolder::pVertexLayout_MESH, numSrvTex, numCbv, numUav, blend, alpha))return false;

	if (Dx_Device::GetInstance()->getDxrCreateResourceState()) {
		mObj.setParameterDXR(alpha);
		if (!mObj.createStreamOutputResource(comIndex, divideBufferMagnification))return false;
		if (!mObj.dxrPara.tessellationF)mObj.dxrPara.updateF = false;
		if (!mObj.createPSO_DXR(Dx_ShaderHolder::pVertexLayout_MESH, numSrvTex, numCbv, numUav, smooth))return false;
		mObj.setTextureDXR();
	}

	return mObj.setDescHeap(numSrvTex, 0, nullptr, nullptr, numCbv, 0, 0);
}

void MeshData::Instancing(CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color) {
	mObj.Instancing(pos, angle, size, Color);
}

void MeshData::InstancingUpdate(float disp, float SmoothRange, float SmoothRatio, float shininess) {
	mObj.InstancingUpdate(disp, SmoothRange, SmoothRatio, shininess);
}

void MeshData::Update(CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
	CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
	float disp, float SmoothRange, float SmoothRatio, float shininess) {

	mObj.Update(pos, Color, angle, size, disp, SmoothRange, SmoothRatio, shininess);
}

void MeshData::DrawOff() {
	mObj.DrawOff();
}

void MeshData::Draw(int com_no) {
	mObj.Draw(com_no);
}

void MeshData::StreamOutput(int com_no) {
	mObj.StreamOutput(com_no);
}

void MeshData::CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index) {
	mObj.CopyResource(comIndex, texture, res, index);
}

void MeshData::TextureInit(int width, int height, int index) {
	mObj.TextureInit(width, height, index);
}

HRESULT MeshData::SetTextureMPixel(int com_no, BYTE* frame, int index) {
	return mObj.SetTextureMPixel(com_no, frame, index);
}