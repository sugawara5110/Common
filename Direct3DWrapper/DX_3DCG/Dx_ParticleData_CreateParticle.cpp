//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@         ParticleData�N���X                                 **//
//**                                  CreateParticle�֐�                                 **//
//*****************************************************************************************//

#include "Dx_ParticleData.h"

std::mutex ParticleData::mtx;

ParticleData::ParticleData() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	ver = 0;
	P_pos = NULL;
}

void ParticleData::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

ParticleData::~ParticleData() {
	S_DELETE(mObjectCB);
	if (P_pos != NULL) {
		free(P_pos);
		P_pos = NULL;
	}
	RELEASE(texture);
	RELEASE(textureUp);
}

void ParticleData::GetShaderByteCode() {
	gsSO = dx->pGeometryShader_PSO.Get();
	gs = dx->pGeometryShader_P.Get();
	vsSO = dx->pVertexShader_PSO.Get();
	vs = dx->pVertexShader_P.Get();
	ps = dx->pPixelShader_P.Get();
}

void ParticleData::MatrixMap(CONSTANT_BUFFER_P *cb, float x, float y, float z, float theta, float size, float speed, bool tex) {
	MATRIX mov;
	MATRIX rot;
	MATRIX world;

	MatrixRotationZ(&rot, theta);
	MatrixTranslation(&mov, x, y, z);
	MatrixMultiply(&world, &rot, &mov);
	MatrixMultiply(&cb->WV, &world, &dx->mView);
	cb->Proj = dx->mProj;
	MatrixTranspose(&cb->WV);
	MatrixTranspose(&cb->Proj);
	cb->size.x = size;
	cb->size.z = speed;
	if (tex)cb->size.w = 1.0f; else cb->size.w = 0.0f;
}

void ParticleData::MatrixMap2(CONSTANT_BUFFER_P *cb, bool init) {
	if (init)cb->size.y = 1.0f; else cb->size.y = 0.0f;
}

void ParticleData::GetVbColarray(int texture_no, float size, float density) {
	UINT64  total_bytes = 0;
	dx->md3dDevice->GetCopyableFootprints(&dx->texture[texture_no]->GetDesc(), 0, 1, 0, &footprint, nullptr, nullptr, &total_bytes);

	//�e�N�X�`���̉��T�C�Y�擾
	float width = (float)dx->texture[texture_no]->GetDesc().Width;
	//�e�N�X�`���̏c�T�C�Y�擾
	float height = (float)dx->texture[texture_no]->GetDesc().Height;

	//�X�e�b�v��
	float step = 1 / size / density;

	//RESOURCE_BARRIER�̓e�N�X�`���擾��WICTextureLoader����D3D12_RESOURCE_STATE_GENERIC_READ�ɂȂ��Ă�̂ł��̂܂�

	//���_���J�E���g
	D3D12_SUBRESOURCE_DATA texResource;
	dx->textureUp[texture_no]->Map(0, nullptr, reinterpret_cast<void**>(&texResource));

	texResource.RowPitch = footprint.Footprint.RowPitch;
	UCHAR *ptex = (UCHAR*)texResource.pData;
	for (float j = 0; j < height; j += step) {
		UINT j1 = (UINT)j * texResource.RowPitch;//RowPitch�f�[�^�̍s�s�b�`�A�s���A�܂��͕����T�C�Y (�o�C�g�P��)
		for (float i = 0; i < width; i += step) {
			UINT ptexI = (UINT)i * 4 + j1;
			if (ptex[ptexI + 3] > 0)ver++;//�A���t�@�l0��荂���ꍇ�J�E���g
		}
	}

	dx->textureUp[texture_no]->Unmap(0, nullptr);

	//�p�[�e�B�N���z��m��
	P_pos = (PartPos*)malloc(sizeof(PartPos) * ver);

	//�s�N�Z���f�[�^�ǂݍ���
	dx->textureUp[texture_no]->Map(0, nullptr, reinterpret_cast<void**>(&texResource));

	texResource.RowPitch = footprint.Footprint.RowPitch;
	ptex = (UCHAR*)texResource.pData;
	int P_no = 0;
	float ws = width * size / 2;//���S��0,0,0�ɂ����
	float hs = height * size / 2;
	for (float j = 0; j < height; j += step) {
		UINT j1 = (UINT)j * texResource.RowPitch;//RowPitch�f�[�^�̍s�s�b�`�A�s���A�܂��͕����T�C�Y (�o�C�g�P��)
		for (float i = 0; i < width; i += step) {
			UINT ptexI = (UINT)i * 4 + j1;
			float yp = (float)(j * size - hs);
			if (ptex[ptexI + 3] > 0) {
				float xp = (float)(i * size - ws);
				P_pos[P_no].Col.as(ptex[ptexI + 0], ptex[ptexI + 1], ptex[ptexI + 2], ptex[ptexI + 3]);//�F
				float xst = xp + ((rand() % 500) * size) - ((rand() % 500) * size);
				float yst = yp + ((rand() % 500) * size) - ((rand() % 500) * size);
				float zst = (rand() % 500) * size;
				P_pos[P_no].CurrentPos.as(xst, yst, zst);
				P_pos[P_no].PosSt.as(xst, yst, zst);
				P_pos[P_no].PosEnd.as(xp, yp, 0.0f);//0,0,0�𒆐S�ɂ���
				P_no++;
			}
		}
	}

	dx->textureUp[texture_no]->Unmap(0, nullptr);
}

void ParticleData::GetBufferParticle(int texture_no, float size, float density) {
	mObjectCB = new UploadBuffer<CONSTANT_BUFFER_P>(dx->md3dDevice.Get(), 1, true);
	Vview = std::make_unique<VertexView>();
	Sview1 = std::make_unique<StreamView[]>(2);
	Sview2 = std::make_unique<StreamView[]>(2);
	GetVbColarray(texture_no, size, density);
}

void ParticleData::GetBufferBill(int v) {
	ver = v;
	P_pos = (PartPos*)malloc(sizeof(PartPos) * ver);
	mObjectCB = new UploadBuffer<CONSTANT_BUFFER_P>(dx->md3dDevice.Get(), 1, true);
	Vview = std::make_unique<VertexView>();
	Sview1 = std::make_unique<StreamView[]>(2);
	Sview2 = std::make_unique<StreamView[]>(2);
}

void ParticleData::CreateVbObj() {
	const UINT vbByteSize = ver * sizeof(PartPos);

	D3DCreateBlob(vbByteSize, &Vview->VertexBufferCPU);
	CopyMemory(Vview->VertexBufferCPU->GetBufferPointer(), P_pos, vbByteSize);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(dx->md3dDevice.Get(),
		mCommandList, P_pos, vbByteSize, Vview->VertexBufferUploader);

	Vview->VertexByteStride = sizeof(PartPos);
	Vview->VertexBufferByteSize = vbByteSize;

	for (int i = 0; i < 2; i++) {
		Sview1[i].StreamBufferGPU = dx->CreateStreamBuffer(dx->md3dDevice.Get(), vbByteSize);
		Sview2[i].StreamBufferGPU = dx->CreateStreamBuffer(dx->md3dDevice.Get(), vbByteSize);

		Sview1[i].StreamByteStride = Sview2[i].StreamByteStride = sizeof(PartPos);
		Sview1[i].StreamBufferByteSize = Sview2[i].StreamBufferByteSize = vbByteSize;
		Sview1[i].BufferFilledSizeLocation = Sview2[i].StreamBufferGPU->GetGPUVirtualAddress();
	}
}

void ParticleData::CreatePartsCom() {

	CD3DX12_ROOT_PARAMETER slotRootParameter_com[1];
	slotRootParameter_com[0].InitAsConstantBufferView(0);

	mRootSignature_com = CreateRsStreamOutput(1, slotRootParameter_com);

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����(STREAM_OUTPUT)
	mPSO_com = CreatePsoStreamOutput(vsSO, gsSO, mRootSignature_com.Get(),
		dx->pVertexLayout_P, dx->pDeclaration_PSO, Sview1[0].StreamByteStride);
}

void ParticleData::CreatePartsDraw(int texpar) {

	if (texpar != -1)texpar_on = TRUE;

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter_draw[2];
	slotRootParameter_draw[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);// DescriptorRange�̐���1��, DescriptorRange�̐擪�A�h���X
	slotRootParameter_draw[1].InitAsConstantBufferView(0);

	mRootSignature_draw = CreateRs(2, slotRootParameter_draw);

	TextureNo te;
	te.diffuse = texpar;
	te.normal = -1;
	te.movie = m_on;
	mSrvHeap = CreateSrvHeap(1, 1, &te, texture);

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g����(Draw)
	mPSO_draw = CreatePsoParticle(vs, ps, gs, mRootSignature_draw.Get(), dx->pVertexLayout_P, TRUE, TRUE);
}

void ParticleData::CreateParticle(int texpar) {
	GetShaderByteCode();
	CreateVbObj();
	CreatePartsCom();
	CreatePartsDraw(texpar);
}

void ParticleData::CreateBillboard() {
	GetShaderByteCode();
	CreateVbObj();
	CreatePartsDraw(-1);
}

void ParticleData::DrawParts0() {

	//mSwapChainBuffer PRESENT��RENDER_TARGET
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void ParticleData::DrawParts1() {
	mCommandList->SetPipelineState(mPSO_com.Get());

	mCommandList->SetGraphicsRootSignature(mRootSignature_com.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCommandList->SOSetTargets(0, 1, &Sview1[svInd].StreamBufferView());

	mCommandList->DrawInstanced(ver, 1, 0, 0);

	svInd = 1 - svInd;
	if (firstDraw) {
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
		mCommandList->CopyResource(Vview->VertexBufferGPU.Get(), Sview1[svInd].StreamBufferGPU.Get());
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Vview->VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	mCommandList->SOSetTargets(0, 1, nullptr);
}

void ParticleData::DrawParts2() {
	mCommandList->SetPipelineState(mPSO_draw.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature_draw.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootConstantBufferView(1, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCommandList->DrawInstanced(ver, 1, 0, 0);
}

void ParticleData::CbSwap(bool init) {
	Lock();
	if (!parInit)parInit = init;//parInit==TRUE�̏ꍇ�܂��������I�����Ă��Ȃ��׃t���O�����������Ȃ�
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = TRUE;//cb,2�v�f����X�V�I��
	}
	sw = 1 - sw;//cb�X���b�v
	Unlock();
	DrawOn = TRUE;
}

void ParticleData::Update(float x, float y, float z, float theta, float size, bool init, float speed) {
	MatrixMap(&cbP[sw], x, y, z, theta, size, speed, texpar_on | m_on);
	CbSwap(init);
}

void ParticleData::DrawOff() {
	DrawOn = FALSE;
}

void ParticleData::Draw() {

	if (!UpOn | !DrawOn)return;

	Lock();
	bool init = parInit;
	//����init == TRUE �œ��streamOut��������
	if (init == TRUE) { streamInitcount = 1; }
	else {
		if (streamInitcount > 2) { streamInitcount = 0; }
		if (streamInitcount != 0) { init = TRUE; streamInitcount++; }
	}
	MatrixMap2(&cbP[1 - sw], init);
	mObjectCB->CopyData(0, cbP[1 - sw]);
	Unlock();

	DrawParts0();
	DrawParts1();
	if (firstDraw)DrawParts2();
	//mSwapChainBuffer RENDER_TARGET��PRESENT
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	firstDraw = TRUE;
	parInit = FALSE;//parInit==TRUE�̏ꍇ�����ɗ������_�ŏ������I��
}

void ParticleData::Update(float size) {
	MatrixMap(&cbP[sw], 0, 0, 0, 0, size, 1.0f, texpar_on | m_on);
	CbSwap(TRUE);
}

void ParticleData::DrawBillboard() {

	if (!UpOn | !DrawOn)return;

	Lock();
	MatrixMap2(&cbP[1 - sw], TRUE);
	mObjectCB->CopyData(0, cbP[1 - sw]);
	Unlock();

	DrawParts0();
	DrawParts2();
	//mSwapChainBuffer RENDER_TARGET��PRESENT
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void ParticleData::SetVertex(int i,
	float vx, float vy, float vz,
	float r, float g, float b, float a) {
	P_pos[i].CurrentPos.as(vx, vy, vz);
	P_pos[i].Col.as(r, g, b, a);
}