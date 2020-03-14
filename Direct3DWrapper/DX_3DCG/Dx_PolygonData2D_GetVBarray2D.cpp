//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          PolygonData2Dクラス                               **//
//**                                    GetVBarray2D関数                                 **//
//*****************************************************************************************//

#include "Dx12ProcessCore.h"

float PolygonData2D::magnificationX = 1.0f;
float PolygonData2D::magnificationY = 1.0f;

void PolygonData2D::Pos2DCompute(VECTOR3 *p) {
	MATRIX VP, VP_VP;
	//入力3D座標から変換行列取得
	MatrixMultiply(&VP, &Dx12Process::GetInstance()->mView, &Dx12Process::GetInstance()->mProj);
	MatrixMultiply(&VP_VP, &VP, &Dx12Process::GetInstance()->Vp);
	//変換後の座標取得
	VectorMatrixMultiply(p, &VP_VP);
	p->x /= magnificationX;
	p->y /= magnificationY;
}

void PolygonData2D::SetMagnification(float x, float y) {
	magnificationX = x;
	magnificationY = y;
}

PolygonData2D::PolygonData2D() {
	dx = Dx12Process::GetInstance();
	mCommandList = dx->dx_sub[0].mCommandList.Get();
	com_no = 0;
	d2varray = nullptr;
	d2varrayI = nullptr;
	magX = magnificationX;
	magY = magnificationY;
}

void PolygonData2D::DisabledMagnification() {
	magX = 1.0f;
	magY = 1.0f;
}

PolygonData2D::~PolygonData2D() {
	free(d2varray);
	d2varray = nullptr;
	free(d2varrayI);
	d2varrayI = nullptr;
	S_DELETE(mObjectCB);
	RELEASE(texture);
	RELEASE(textureUp);
}

void PolygonData2D::InstancedSetConstBf(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY) {
	InstancedSetConstBf(x, y, 0.0f, r, g, b, a, sizeX, sizeY);
}

void PolygonData2D::InstancedSetConstBf(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY) {

	int sw = dx->cBuffSwap[0];
	cb2[sw].Pos[ins_no].as(x * magX, y * magY, z, 0.0f);
	cb2[sw].Color[ins_no].as(r, g, b, a);
	cb2[sw].sizeXY[ins_no].as(sizeX * magX, sizeY * magY, 0.0f, 0.0f);
	cb2[sw].WidHei.as((float)dx->mClientWidth, (float)dx->mClientHeight, 0.0f, 0.0f);
	ins_no++;
}

void PolygonData2D::SetConstBf(CONSTANT_BUFFER2D *cb, float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY) {

	cb->Pos[ins_no].as(x * magX, y * magY, z, 0.0f);
	cb->Color[ins_no].as(r, g, b, a);
	cb->sizeXY[ins_no].as(sizeX * magX, sizeY * magY, 0.0f, 0.0f);
	cb->WidHei.as((float)dx->mClientWidth, (float)dx->mClientHeight, 0.0f, 0.0f);
	ins_no++;
}

void PolygonData2D::SetTextParameter(int width, int height, int textCount,
	TEXTMETRIC **TM, GLYPHMETRICS **GM, BYTE **ptr, DWORD **allsize) {

	Twidth = width;
	Theight = height;
	Tcount = textCount;
	Tm = new TEXTMETRIC[Tcount]();
	memcpy(Tm, *TM, sizeof(TEXTMETRIC)*Tcount);
	Gm = new GLYPHMETRICS[Tcount]();
	memcpy(Gm, *GM, sizeof(GLYPHMETRICS)*Tcount);
	Allsize = new DWORD[Tcount]();
	memcpy(Allsize, *allsize, sizeof(DWORD)*Tcount);
	Ptr = new BYTE[Allsize[Tcount - 1]]();
	memcpy(Ptr, *ptr, sizeof(BYTE)*Allsize[Tcount - 1]);
	CreateTextOn = true;
}

void PolygonData2D::SetText() {

	if (!CreateTextOn)return;

	RELEASE(texture);
	RELEASE(textureUp);

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = Twidth;
	texDesc.Height = Theight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	dx->md3dDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture));

	//upload
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture, 0, 1);
	D3D12_HEAP_PROPERTIES HeapPropsUp;
	HeapPropsUp.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapPropsUp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapPropsUp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapPropsUp.CreationNodeMask = 1;
	HeapPropsUp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc;
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = uploadBufferSize;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	dx->md3dDevice->CreateCommittedResource(&HeapPropsUp, D3D12_HEAP_FLAG_NONE,
		&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&textureUp));

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT  footprint;
	UINT64  total_bytes = 0;
	dx->md3dDevice->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &total_bytes);

	D3D12_TEXTURE_COPY_LOCATION dest, src;

	memset(&dest, 0, sizeof(dest));
	dest.pResource = texture;
	dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest.SubresourceIndex = 0;

	memset(&src, 0, sizeof(src));
	src.pResource = textureUp;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint = footprint;

	dx->dx_sub[com_no].ResourceBarrier(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_SUBRESOURCE_DATA texResource;
	textureUp->Map(0, nullptr, reinterpret_cast<void**>(&texResource));
	BYTE* pBits = (BYTE*)texResource.pData;
	texResource.RowPitch = footprint.Footprint.RowPitch;
	memset(pBits, 0, texResource.RowPitch * Theight);//0埋め
	for (int cnt = 0; cnt < Tcount; cnt++) {

		UINT temp = (UINT)(texResource.RowPitch / Tcount / 4);
		UINT s_rowPitch = temp * 4;
		UINT offset1 = s_rowPitch * cnt;//4の倍数になっている事

		// フォント情報の書き込み
		// iOfs_x, iOfs_y : 書き出し位置(左上)
		// iBmp_w, iBmp_h : フォントビットマップの幅高
		// Level : α値の段階 (GGO_GRAY4_BITMAPなので17段階)
		int iOfs_x = Gm[cnt].gmptGlyphOrigin.x;
		int iOfs_y = Tm[cnt].tmAscent - Gm[cnt].gmptGlyphOrigin.y;
		int iBmp_w = Gm[cnt].gmBlackBoxX + (4 - (Gm[cnt].gmBlackBoxX % 4)) % 4;
		int iBmp_h = Gm[cnt].gmBlackBoxY;
		int Level = 17;
		int x, y;
		DWORD Alpha, Color;

		for (y = iOfs_y; y < iOfs_y + iBmp_h; y++) {
			for (x = iOfs_x; x < iOfs_x + iBmp_w; x++) {
				int offset2;
				if (cnt == 0)offset2 = 0; else {
					offset2 = Allsize[cnt - 1];
				}
				Alpha = (255 * Ptr[(x - iOfs_x + iBmp_w * (y - iOfs_y)) + offset2]) / (Level - 1);
				Color = 0x00ffffff | (Alpha << 24);
				memcpy((BYTE*)pBits + texResource.RowPitch * y + 4 * x + offset1, &Color, sizeof(DWORD));
			}
		}
	}
	textureUp->Unmap(0, nullptr);

	mCommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

	dx->dx_sub[com_no].ResourceBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	dx->md3dDevice->CreateShaderResourceView(texture, &srvDesc, hDescriptor);

	ARR_DELETE(Tm);
	ARR_DELETE(Gm);
	ARR_DELETE(Ptr);
	ARR_DELETE(Allsize);

	CreateTextOn = false;
}

ID3D12PipelineState *PolygonData2D::GetPipelineState() {
	return mPSO.Get();
}

void PolygonData2D::GetVBarray2D(int pcs) {
	ver = pcs * 4;
	d2varray = (MY_VERTEX2*)malloc(sizeof(MY_VERTEX2) * ver);
	d2varrayI = (std::uint16_t*)malloc(sizeof(std::uint16_t) * (int)(ver * 1.5));
	mObjectCB = new ConstantBuffer<CONSTANT_BUFFER2D>(1);
	Vview = std::make_unique<VertexView>();
	Iview = std::make_unique<IndexView>();
}

void PolygonData2D::TexOn() {
	tex_on = true;
}

void PolygonData2D::GetShaderByteCode() {

	if (!tex_on) {
		vs = dx->pVertexShader_2D.Get();
		ps = dx->pPixelShader_2D.Get();
	}
	else {
		vs = dx->pVertexShader_2DTC.Get();
		ps = dx->pPixelShader_2DTC.Get();
	}
}

bool PolygonData2D::CreateBox(float x, float y, float z, float sizex, float sizey, float r, float g, float b, float a, bool blend, bool alpha) {

	//verが4の時のみ実行可
	if (ver != 4)return false;
	d2varray[0].Pos.as(x * magX, y * magY, z);
	d2varray[0].color.as(r, g, b, a);
	d2varray[0].tex.as(0.0f, 0.0f);

	d2varray[1].Pos.as(x * magX, (y + sizey) * magY, z);
	d2varray[1].color.as(r, g, b, a);
	d2varray[1].tex.as(0.0f, 1.0f);

	d2varray[2].Pos.as((x + sizex) * magX, (y + sizey) * magY, z);
	d2varray[2].color.as(r, g, b, a);
	d2varray[2].tex.as(1.0f, 1.0f);

	d2varray[3].Pos.as((x + sizex) * magX, y * magY, z);
	d2varray[3].color.as(r, g, b, a);
	d2varray[3].tex.as(1.0f, 0.0f);

	d2varrayI[0] = 0;
	d2varrayI[1] = 3;
	d2varrayI[2] = 1;
	d2varrayI[3] = 1;
	d2varrayI[4] = 3;
	d2varrayI[5] = 2;

	return Create(blend, alpha);
}

bool PolygonData2D::Create(bool blend, bool alpha) {

	GetShaderByteCode();

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[1].InitAsConstantBufferView(0);

	mRootSignature = CreateRs(2, slotRootParameter);
	if (mRootSignature == nullptr)return false;

	TextureNo te;
	te.diffuse = -1;
	te.normal = -1;
	te.movie = m_on;
	mSrvHeap = CreateSrvHeap(1, 1, &te, texture);
	if (mSrvHeap == nullptr)return false;

	const UINT vbByteSize = ver * sizeof(MY_VERTEX2);
	const UINT ibByteSize = (int)(ver * 1.5) * sizeof(std::uint16_t);

	Vview->VertexBufferGPU = dx->CreateDefaultBuffer(com_no, d2varray, vbByteSize, Vview->VertexBufferUploader);

	Iview->IndexBufferGPU = dx->CreateDefaultBuffer(com_no, d2varrayI, ibByteSize, Iview->IndexBufferUploader);

	Vview->VertexByteStride = sizeof(MY_VERTEX2);
	Vview->VertexBufferByteSize = vbByteSize;
	Iview->IndexFormat = DXGI_FORMAT_R16_UINT;
	Iview->IndexBufferByteSize = ibByteSize;
	Iview->IndexCount = (int)(ver * 1.5);

	//パイプラインステートオブジェクト生成
	mPSO = CreatePsoVsPs(vs, ps, mRootSignature.Get(), dx->pVertexLayout_2D, alpha, blend);
	if (mPSO == nullptr)return false;

	return true;
}

void PolygonData2D::CbSwap() {
	if (!UpOn) {
		upCount++;
		if (upCount > 1)UpOn = true;//cb,2要素初回更新終了
	}
	insNum[dx->cBuffSwap[0]] = ins_no;
	ins_no = 0;
	DrawOn = true;
}

void PolygonData2D::InstanceUpdate() {
	CbSwap();
}

void PolygonData2D::Update(float x, float y, float r, float g, float b, float a, float sizeX, float sizeY) {
	Update(x, y, 0.0f, r, g, b, a, sizeX, sizeY);
}

void PolygonData2D::Update(float x, float y, float z, float r, float g, float b, float a, float sizeX, float sizeY) {
	SetConstBf(&cb2[dx->cBuffSwap[0]], x, y, z, r, g, b, a, sizeX, sizeY);
	CbSwap();
}

void PolygonData2D::DrawOff() {
	DrawOn = false;
}

void PolygonData2D::Draw() {

	if (!UpOn | !DrawOn)return;

	mObjectCB->CopyData(0, cb2[dx->cBuffSwap[1]]);

	mCommandList->SetPipelineState(mPSO.Get());

	//mSwapChainBuffer PRESENT→RENDER_TARGET
	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);//テクスチャ無しの場合このままで良いのやら・・エラーは無し

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &Vview->VertexBufferView());
	mCommandList->IASetIndexBuffer(&Iview->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootConstantBufferView(1, mObjectCB->Resource()->GetGPUVirtualAddress());

	mCommandList->DrawIndexedInstanced(Iview->IndexCount, insNum[dx->cBuffSwap[1]], 0, 0, 0);

	//mSwapChainBuffer RENDER_TARGET→PRESENT
	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}



