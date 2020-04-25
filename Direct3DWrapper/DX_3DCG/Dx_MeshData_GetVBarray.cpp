//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          MeshData�N���X                                    **//
//**                                   GetVBarray�֐�                                    **//
//*****************************************************************************************//

#include "Dx_MeshData.h"

MeshData::MeshData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
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

MeshData::~MeshData() {
	S_DELETE(mObjectCB);
	S_DELETE(mObject_MESHCB);
}

ID3D12PipelineState* MeshData::GetPipelineState() {
	return dpara.PSO.Get();
}

void MeshData::GetShaderByteCode(bool disp) {

	if (disp) {
		vs = dx->pVertexShader_MESH_D.Get();
		hs = dx->pHullShaderTriangle.Get();
		ds = dx->pDomainShaderTriangle.Get();
		gs = dx->pGeometryShader_Before_ds.Get();
	}
	else {
		vs = dx->pVertexShader_MESH.Get();
		gs = dx->pGeometryShader_Before_vs.Get();
	}
	ps = dx->pPixelShader_3D.Get();
}

bool MeshData::LoadMaterialFromFile(char* FileName) {

	//�}�e���A���t�@�C�����J���ē��e��ǂݍ���
	errno_t error;
	FILE* fp = nullptr;
	error = fopen_s(&fp, FileName, "rt");
	if (error != 0) {
		ErrorMessage("MeshData::LoadMaterialFromFile fopen_s Error");
		return false;
	}
	char line[200] = { 0 };//1�s�ǂݍ��ݗp
	char key[110] = { 0 };//1�P��ǂݍ��ݗp
	VECTOR4 v = { 0, 0, 0, 1 };

	//�}�e���A�����𒲂ׂ�
	while (!feof(fp))
	{
		//�L�[���[�h�ǂݍ���
		fgets(line, sizeof(line), fp);
		sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));
		//�}�e���A����
		if (strcmp(key, "newmtl") == 0)
		{
			dpara.NumMaterial++;
		}
	}

	dpara.material = std::make_unique<MY_MATERIAL_S[]>(dpara.NumMaterial);
	mMat = std::make_unique<meshMaterial[]>(dpara.NumMaterial);

	//�{�ǂݍ���	
	fseek(fp, 0, SEEK_SET);
	INT iMCount = -1;

	while (!feof(fp))
	{
		//�L�[���[�h�ǂݍ���
		fgets(line, sizeof(line), fp);//1�s�ǂݍ���line�Ɋi�[,FILE�|�C���^1�s�i��
		sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));//�ǂݍ���1�s����"%s"�ŏ��̕�����1�ǂݍ���
		//�}�e���A����
		if (strcmp(key, "newmtl") == 0)
		{
			iMCount++;
			sscanf_s(&line[7], "%s ", key, (unsigned int)sizeof(key));//line��7�v�f��(newmtl)�̒��ォ��1�ڂ̕������key�Ɋi�[
			strcpy_s(mMat[iMCount].szName, key);
		}
		//Kd�@�f�B�t���[�Y
		if (strcmp(key, "Kd") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
			dpara.material[iMCount].diffuse = v;
		}
		//Ks�@�X�y�L�����[
		if (strcmp(key, "Ks") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
			dpara.material[iMCount].specular = v;
		}
		//Ka�@�A���r�G���g
		if (strcmp(key, "Ka") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
			dpara.material[iMCount].ambient = v;
		}
		//map_Kd�@�e�N�X�`���[
		if (strcmp(key, "map_Kd") == 0)
		{
			sscanf_s(&line[7], "%s", &mMat[iMCount].szTextureName, (unsigned int)sizeof(mMat[iMCount].szTextureName));
			dpara.material[iMCount].diftex_no = dx->GetTexNumber(mMat[iMCount].szTextureName);
		}
		//map_bump�@�e�N�X�`���[
		if (strcmp(key, "map_bump") == 0)
		{
			sscanf_s(&line[7], "%s", &mMat[iMCount].norTextureName, (unsigned int)sizeof(mMat[iMCount].norTextureName));
			dpara.material[iMCount].nortex_no = dx->GetTexNumber(mMat[iMCount].norTextureName);
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

	if (disp) {
		primType_create = CONTROL_POINT;
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}
	else {
		primType_create = SQUARE;
		dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

bool MeshData::GetBuffer(char* FileName) {

	if (0 != strcpy_s(mFileName, FileName)) {
		ErrorMessage("MeshData::GetBuffer strcpy_s Error");
		return false;
	}

	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER>(1);

	int VCount = 0;//�ǂݍ��݃J�E���^�[
	int VNCount = 0;//�ǂݍ��݃J�E���^�[
	int VTCount = 0;//�ǂݍ��݃J�E���^�[
	FaceCount = 0;//�|���S�����J�E���^�[

	char line[200] = { 0 };
	char key[200] = { 0 };
	//OBJ�t�@�C�����J���ē��e��ǂݍ���
	errno_t error;
	FILE* fp = nullptr;
	error = fopen_s(&fp, mFileName, "rt");
	if (error != 0) {
		ErrorMessage("MeshData::GetBuffer fopen_s Error");
		return false;
	}

	//���O�ɒ��_���A�|���S�����𒲂ׂ�
	while (!feof(fp))
	{
		//�L�[���[�h�ǂݍ���
		fgets(line, sizeof(line), fp);
		sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));
		//�}�e���A���ǂݍ���
		if (strcmp(key, "mtllib") == 0)
		{
			sscanf_s(&line[7], "%s ", key, (unsigned int)sizeof(key));
			if (!LoadMaterialFromFile(key))return false;
		}
		//���_
		if (strcmp(key, "v") == 0)
		{
			VCount++;
		}
		//�@��
		if (strcmp(key, "vn") == 0)
		{
			VNCount++;
		}
		//�e�N�X�`���[���W
		if (strcmp(key, "vt") == 0)
		{
			VTCount++;
		}
		//�t�F�C�X�i�|���S���j
		if (strcmp(key, "f") == 0)
		{
			FaceCount++;
		}
	}
	fclose(fp);

	//�ꎞ�I�ȃ������m��
	pvCoord = new VECTOR3[VCount]();
	pvNormal = new VECTOR3[VNCount]();
	pvTexture = new VECTOR2[VTCount]();

	mObject_MESHCB = new ConstantBuffer<CONSTANT_BUFFER2>(dpara.NumMaterial);//�A�h���X���炵�ĊeMaterial�A�N�Z�X
	dpara.Vview = std::make_unique<VertexView>();
	dpara.Iview = std::make_unique<IndexView[]>(dpara.NumMaterial);

	piFaceBuffer = new int[dpara.NumMaterial * FaceCount * 3]();//3���_�Ȃ̂�3�C���f�b�N�X * Material��
	pvVertexBuffer = new VertexM[FaceCount * 3]();

	return true;
}

bool MeshData::SetVertex() {

	float x, y, z;
	int v1 = 0, v2 = 0, v3 = 0;
	int vn1 = 0, vn2 = 0, vn3 = 0;
	int vt1 = 0, vt2 = 0, vt3 = 0;
	int VCount = 0;//�ǂݍ��݃J�E���^�[
	int VNCount = 0;//�ǂݍ��݃J�E���^�[
	int VTCount = 0;//�ǂݍ��݃J�E���^�[

	char line[200] = { 0 };
	char key[200] = { 0 };
	//OBJ�t�@�C�����J���ē��e��ǂݍ���
	errno_t error;
	FILE* fp = nullptr;
	error = fopen_s(&fp, mFileName, "rt");
	if (error != 0) {
		ErrorMessage("MeshData::SetVertex fopen_s Error");
		return false;
	}
	//�{�ǂݍ���	
	while (!feof(fp))
	{
		//�L�[���[�h �ǂݍ���
		ZeroMemory(key, sizeof(key));
		fgets(line, sizeof(line), fp);
		sscanf_s(line, "%s", key, (unsigned int)sizeof(key));

		//���_ �ǂݍ���
		if (strcmp(key, "v") == 0)
		{
			sscanf_s(&line[2], "%f %f %f", &x, &y, &z);
			pvCoord[VCount].x = x;
			pvCoord[VCount].y = z;
			pvCoord[VCount].z = y;
			VCount++;
		}

		//�@�� �ǂݍ���
		if (strcmp(key, "vn") == 0)
		{
			sscanf_s(&line[3], "%f %f %f", &x, &y, &z);
			pvNormal[VNCount].x = x;
			pvNormal[VNCount].y = z;
			pvNormal[VNCount].z = y;
			VNCount++;
		}

		//�e�N�X�`���[���W �ǂݍ���
		if (strcmp(key, "vt") == 0)
		{
			sscanf_s(&line[3], "%f %f", &x, &y);
			pvTexture[VTCount].x = x;
			pvTexture[VTCount].y = 1 - y;//OBJ�t�@�C����Y�������t�Ȃ̂ō��킹��
			VTCount++;
		}
	}

	//������W���_���X�g
	SameVertexList* svList = new SameVertexList[VCount];

	for (int i = 0; i < dpara.NumMaterial; i++) {
		CONSTANT_BUFFER2 sg;
		dpara.material[i].diffuse.x += addDiffuse;
		dpara.material[i].diffuse.y += addDiffuse;
		dpara.material[i].diffuse.z += addDiffuse;
		dpara.material[i].specular.x += addSpecular;
		dpara.material[i].specular.y += addSpecular;
		dpara.material[i].specular.z += addSpecular;
		dpara.material[i].ambient.x += addAmbient;
		dpara.material[i].ambient.y += addAmbient;
		dpara.material[i].ambient.z += addAmbient;
		sg.vDiffuse = dpara.material[i].diffuse;//�f�B�t���[�Y�J���[���V�F�[�_�[�ɓn��
		sg.vDiffuse = dpara.material[i].specular;//�X�y�L�����[���V�F�[�_�[�ɓn��
		sg.vAmbient = dpara.material[i].ambient;//�A���r�G���g���V�F�[�_�[�ɓn��
		mObject_MESHCB->CopyData(i, sg);
	}

	//�t�F�C�X�@�ǂݍ��݁@�o���o���Ɏ��^����Ă���\��������̂ŁA�}�e���A�����𗊂�ɂȂ����킹��
	bool boFlag = false;

	int FCount = 0;
	int dwPartFCount = 0;
	for (int i = 0; i < dpara.NumMaterial; i++)
	{
		dwPartFCount = 0;
		fseek(fp, 0, SEEK_SET);

		while (!feof(fp))
		{
			//�L�[���[�h �ǂݍ���
			ZeroMemory(key, sizeof(key));
			fgets(line, sizeof(line), fp);
			sscanf_s(line, "%s ", key, (unsigned int)sizeof(key));

			//�t�F�C�X �ǂݍ��݁����_�C���f�b�N�X��
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
				if (dpara.material[i].diftex_no != -1)//�e�N�X�`���[����T�[�t�F�C�X
				{
					sscanf_s(&line[2], "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				}
				else//�e�N�X�`���[�����T�[�t�F�C�X
				{
					sscanf_s(&line[2], "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
					vt1 = vt2 = vt3 = 1;//���G���[�h�~
				}

				//�C���f�b�N�X�o�b�t�@�[
				piFaceBuffer[FaceCount * 3 * i + dwPartFCount * 3] = FCount * 3;
				piFaceBuffer[FaceCount * 3 * i + dwPartFCount * 3 + 1] = FCount * 3 + 1;
				piFaceBuffer[FaceCount * 3 * i + dwPartFCount * 3 + 2] = FCount * 3 + 2;
				//���_�\���̂ɑ��
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
		if (dwPartFCount == 0)//�g�p����Ă��Ȃ��}�e���A���΍􂪕K�v�ȏꍇ�����ǉ��BDraw�ɂ�
		{
			continue;
		}

		const UINT ibByteSize = (UINT)dwPartFCount * 3 * sizeof(UINT);

		dpara.Iview[i].IndexFormat = DXGI_FORMAT_R32_UINT;
		dpara.Iview[i].IndexBufferByteSize = ibByteSize;
		dpara.Iview[i].IndexCount = dwPartFCount * 3;
	}

	//������W���_�̖@�����ꉻ(�e�Z���[�V�����p)
	for (int i = 0; i < VCount; i++) {
		int indVB = 0;
		VECTOR3 geo[50];
		int indVb[50];
		int indGeo = 0;
		while (1) {
			indVB = svList[i].Pop();
			if (indVB == -1)break;
			indVb[indGeo] = indVB;
			geo[indGeo++] = pvVertexBuffer[indVB].geoNormal;
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
			pvVertexBuffer[indVb[i1]].geoNormal = ave;
		}
	}

	fclose(fp);

	const UINT vbByteSize = (UINT)FCount * 3 * sizeof(VertexM);

	dpara.Vview->VertexByteStride = sizeof(VertexM);
	dpara.Vview->VertexBufferByteSize = vbByteSize;

	//�ꎞ�I�ϐ����
	ARR_DELETE(pvCoord);
	ARR_DELETE(pvNormal);
	ARR_DELETE(pvTexture);
	ARR_DELETE(svList);

	return true;
}

bool MeshData::CreateMesh() {

	GetShaderByteCode(disp);

	dpara.rootSignature = CreateRootSignature("MeshData");
	if (dpara.rootSignature == nullptr)return false;

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	dpara.PSO = CreatePsoVsHsDsPs(vs, hs, ds, ps, gs, dpara.rootSignature.Get(), dx->pVertexLayout_MESH, alpha, blend, primType_create);
	if (dpara.PSO == nullptr)return false;

	return true;
}

bool MeshData::GetTexture() {

	TextureNo* te = new TextureNo[dpara.NumMaterial];
	for (int i = 0; i < dpara.NumMaterial; i++) {
		if (dpara.material[i].diftex_no < 0)te[i].diffuse = dx->GetTexNumber("dummyDifSpe.");
		else
			te[i].diffuse = dpara.material[i].diftex_no;

		if (dpara.material[i].nortex_no < 0)te[i].normal = dx->GetTexNumber("dummyNor.");
		else
			te[i].normal = dpara.material[i].nortex_no;

		te[i].specular = dx->GetTexNumber("dummyDifSpe.");
	}

	createTextureResource(0, dpara.NumMaterial, te);
	int numTex = dpara.NumMaterial * 3;
	dpara.numDesc = 3;
	dpara.descHeap = CreateDescHeap(numTex);
	if (dpara.descHeap == nullptr)return false;
	CreateSrvTexture(dpara.descHeap.Get(), 0, texture->GetAddressOf(), numTex);

	for (int i = 0; i < dpara.NumMaterial; i++)
		dpara.Iview[i].IndexBufferGPU = dx->CreateDefaultBuffer(com_no, &piFaceBuffer[FaceCount * 3 * i],
			dpara.Iview[i].IndexBufferByteSize, dpara.Iview[i].IndexBufferUploader);

	ARR_DELETE(te);
	ARR_DELETE(piFaceBuffer);

	dpara.Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, pvVertexBuffer,
		dpara.Vview->VertexBufferByteSize, dpara.Vview->VertexBufferUploader);
	ARR_DELETE(pvVertexBuffer);

	return true;
}

void MeshData::InstancedMap(float x, float y, float z, float thetaZ, float thetaY, float thetaX, float size) {
	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, thetaZ, thetaY, thetaX, size);
}

void MeshData::CbSwap() {
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2�v�f����X�V�I��
	}
	insNum[dx->cBuffSwap[0]] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void MeshData::InstanceUpdate(float r, float g, float b, float a, float disp, float shininess) {
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f, divArr, numDiv, shininess);
	CbSwap();
}

void MeshData::Update(float x, float y, float z, float r, float g, float b, float a, float thetaZ, float thetaY, float thetaX, float size, float disp, float shininess) {
	dx->InstancedMap(ins_no, &cb[dx->cBuffSwap[0]], x, y, z, thetaZ, thetaY, thetaX, size);
	dx->MatrixMap(&cb[dx->cBuffSwap[0]], r, g, b, a, disp, 1.0f, 1.0f, 1.0f, 1.0f, divArr, numDiv, shininess);
	CbSwap();
}

void MeshData::DrawOff() {
	DrawOn = false;
}

void MeshData::Draw() {

	if (!UpOn | !DrawOn)return;

	mObjectCB->CopyData(0, cb[dx->cBuffSwap[1]]);

	dpara.cbRes0 = mObjectCB->Resource();
	dpara.cbRes1 = mObject_MESHCB->Resource();
	dpara.cbRes2 = nullptr;
	dpara.insNum = insNum[dx->cBuffSwap[1]];
	drawsub(dpara);
}