//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Commonクラス                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx12ProcessCore.h"

void Common::SetCommandList(int no) {
	com_no = no;
	mCommandList = dx->dx_sub[com_no].mCommandList.Get();
}

void Common::CopyResource(ID3D12Resource *Intexture, D3D12_RESOURCE_STATES res) {
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Intexture,
		res, D3D12_RESOURCE_STATE_COPY_SOURCE));
	
	mCommandList->CopyResource(texture, Intexture);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Intexture,
		D3D12_RESOURCE_STATE_COPY_SOURCE, res));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

HRESULT Common::TextureInit(int width, int height) {

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	HRESULT hr;
	hr = dx->md3dDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&texture));
	if (FAILED(hr)) {
		ErrorMessage("Common::TextureInit Error!!"); return hr;
	}

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

	hr = dx->md3dDevice->CreateCommittedResource(&HeapPropsUp, D3D12_HEAP_FLAG_NONE,
		&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&textureUp));
	if (FAILED(hr)) {
		ErrorMessage("Common::TextureInit Error!!"); return hr;
	}

	UINT64  total_bytes = 0;
	dx->md3dDevice->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &total_bytes);

	memset(&dest, 0, sizeof(dest));
	dest.pResource = texture;
	dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest.SubresourceIndex = 0;

	memset(&src, 0, sizeof(src));
	src.pResource = textureUp;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint = footprint;

	m_on = true;

	return S_OK;
}

HRESULT Common::SetTextureMPixel(UINT **m_pix, BYTE r, BYTE g, BYTE b, BYTE a, BYTE Threshold) {

	D3D12_RESOURCE_DESC texdesc;
	texdesc = texture->GetDesc();
	//テクスチャの横サイズ取得
	int width = (int)texdesc.Width;
	//テクスチャの縦サイズ取得
	int height = texdesc.Height;

	D3D12_SUBRESOURCE_DATA texResource;

	D3D12_RESOURCE_BARRIER BarrierDesc;
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = texture;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	mCommandList->ResourceBarrier(1, &BarrierDesc);

	HRESULT hr;
	hr = textureUp->Map(0, nullptr, reinterpret_cast<void**>(&texResource));
	if (FAILED(hr)) {
		ErrorMessage("Common::SetTextureMPixel Error!!"); return hr;
	}

	UCHAR *ptex = (UCHAR*)texResource.pData;
	texResource.RowPitch = footprint.Footprint.RowPitch;

	for (int j = 0; j < height; j++) {
		UINT j1 = (UINT)(j * texResource.RowPitch);//RowPitchデータの行ピッチ、行幅、または物理サイズ (バイト単位)
		for (int i = 0; i < width; i++) {
			UINT ptexI = i * 4 + j1;
			ptex[ptexI + 2] = m_pix[j][i] >> 16 & r;
			ptex[ptexI + 1] = m_pix[j][i] >> 8 & g;
			ptex[ptexI + 0] = m_pix[j][i] & b;

			if ((m_pix[j][i] >> 16 & b) < Threshold && (m_pix[j][i] >> 8 & g) < Threshold && (m_pix[j][i] & r) < Threshold) {
				ptex[ptexI + 3] = 0;
			}
			else {
				ptex[ptexI + 3] = a;
			}
		}
	}
	textureUp->Unmap(0, nullptr);

	mCommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	mCommandList->ResourceBarrier(1, &BarrierDesc);

	return S_OK;
}

ID3D12DescriptorHeap *Common::CreateSrvHeap(int MaterialNum, int texNum, TextureNo *to, ID3D12Resource *movietex)
{
	ID3D12DescriptorHeap *srv;

	//使用テクスチャ数だけDescriptorを用意する
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = texNum;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr;
	hr = dx->md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srv));
	if (FAILED(hr)) {
		ErrorMessage("Common::CreateSrvHeap Error!!"); return nullptr;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(srv->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	for (int i = 0; i < MaterialNum; i++) {
		//diffuse(movietex使用の場合スキップ)
		if (!to[i].movie && to[i].diffuse != -1) {
			srvDesc.Format = dx->texture[to[i].diffuse]->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = dx->texture[to[i].diffuse]->GetDesc().MipLevels;
			dx->md3dDevice->CreateShaderResourceView(dx->texture[to[i].diffuse], &srvDesc, hDescriptor);
			hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
		}
		//movietex使用
		if (to[i].movie) {
			srvDesc.Format = movietex->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = movietex->GetDesc().MipLevels;
			dx->md3dDevice->CreateShaderResourceView(movietex, &srvDesc, hDescriptor);
			hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
		}
		//normalMapが存在する場合
		if (to[i].normal != -1) {
			srvDesc.Format = dx->texture[to[i].normal]->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = dx->texture[to[i].normal]->GetDesc().MipLevels;
			dx->md3dDevice->CreateShaderResourceView(dx->texture[to[i].normal], &srvDesc, hDescriptor);
			hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
		}
	}

	return srv;
}

ID3D12RootSignature *Common::CreateRsCommon(CD3DX12_ROOT_SIGNATURE_DESC *rootSigDesc)
{
	ID3D12RootSignature *rs;

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr;
	hr = D3D12SerializeRootSignature(rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	if (FAILED(hr)) {
		ErrorMessage("Common::CreateRsCommon Error!!"); return nullptr;
	}

	//RootSignature生成
	hr = dx->md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rs));

	if (FAILED(hr)) {
		ErrorMessage("Common::CreateRsCommon Error!!"); return nullptr;
	}

	return rs;
}

ID3D12RootSignature* Common::CreateRs(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter)
{
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		1, &linearWrap,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	return CreateRsCommon(&rootSigDesc);
}

ID3D12RootSignature *Common::CreateRsStreamOutput(int paramNum, CD3DX12_ROOT_PARAMETER *slotRootParameter)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		NULL, NULL,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	return CreateRsCommon(&rootSigDesc);
}

ID3D12RootSignature *Common::CreateRsCompute(int paramNum, CD3DX12_ROOT_PARAMETER *slotRootParameter)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	return CreateRsCommon(&rootSigDesc);
}

ID3D12PipelineState *Common::CreatePSO(ID3DBlob *vs, ID3DBlob *hs,
	ID3DBlob *ds, ID3DBlob *ps, ID3DBlob *gs,
	ID3D12RootSignature *mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC> *pVertexLayout,
	std::vector<D3D12_SO_DECLARATION_ENTRY> *pDeclaration,
	bool STREAM_OUTPUT, UINT StreamSize, bool alpha, bool blend,
	PrimitiveType type)
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	ID3D12PipelineState *pso;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { pVertexLayout->data(), (UINT)pVertexLayout->size() };
	psoDesc.pRootSignature = mRootSignature;
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	if (hs != nullptr) {
		psoDesc.HS =
		{
			reinterpret_cast<BYTE*>(hs->GetBufferPointer()),
			hs->GetBufferSize()
		};
		topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	}
	if (ds != nullptr) {
		psoDesc.DS =
		{
			reinterpret_cast<BYTE*>(ds->GetBufferPointer()),
			ds->GetBufferSize()
		};
	}
	if (gs != nullptr) {
		psoDesc.GS =
		{
			reinterpret_cast<BYTE*>(gs->GetBufferPointer()),
			gs->GetBufferSize()
		};
		topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	}
	if (!STREAM_OUTPUT) {
		psoDesc.PS =
		{
			reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
			ps->GetBufferSize()
		};
	}
	if (STREAM_OUTPUT) {
		psoDesc.StreamOutput.pSODeclaration = pDeclaration->data();
		psoDesc.StreamOutput.NumEntries = 4;  //D3D12_SO_DECLARATION_ENTRYの個数
		psoDesc.StreamOutput.pBufferStrides = &StreamSize;
		psoDesc.StreamOutput.NumStrides = 1;//バッファの数？
		psoDesc.StreamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;
	}
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.IndependentBlendEnable = false;
	psoDesc.BlendState.AlphaToCoverageEnable = alpha;//アルファテストon/off
	psoDesc.BlendState.RenderTarget[0].BlendEnable = blend;//ブレンドon/off
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;

	switch (type) {
	case SQUARE:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case POINt:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case LINE_L:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case LINE_S:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case CONTROL_POINT:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;
	case NUL:
		psoDesc.PrimitiveTopologyType = topology;
		break;
	}

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = dx->mBackBufferFormat;
	psoDesc.SampleDesc.Count = dx->m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = dx->m4xMsaaState ? (dx->m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = dx->mDepthStencilFormat;

	HRESULT hr;
	hr = dx->md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		ErrorMessage("Common::CreatePSO Error!!"); return nullptr;
	}

	return pso;
}

ID3D12PipelineState *Common::CreatePsoVsPs(ID3DBlob *vs, ID3DBlob *ps,
	ID3D12RootSignature *mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend,
	PrimitiveType type)
{
	return CreatePSO(vs, nullptr, nullptr, ps, nullptr, mRootSignature, &pVertexLayout, nullptr, false, 0, alpha, blend, type);
}

ID3D12PipelineState *Common::CreatePsoVsHsDsPs(ID3DBlob *vs, ID3DBlob *hs, ID3DBlob *ds, ID3DBlob *ps,
	ID3D12RootSignature *mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend,
	PrimitiveType type)
{
	return CreatePSO(vs, hs, ds, ps, nullptr, mRootSignature, &pVertexLayout, nullptr, false, 0, alpha, blend, type);
}

ID3D12PipelineState *Common::CreatePsoStreamOutput(ID3DBlob *vs, ID3DBlob *gs,
	ID3D12RootSignature *mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	std::vector<D3D12_SO_DECLARATION_ENTRY>& pDeclaration, UINT StreamSize)
{
	return CreatePSO(vs, nullptr, nullptr, nullptr, gs, mRootSignature, &pVertexLayout, &pDeclaration, true, StreamSize, true, true, NUL);
}

ID3D12PipelineState *Common::CreatePsoParticle(ID3DBlob *vs, ID3DBlob *ps, ID3DBlob *gs,
	ID3D12RootSignature *mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend)
{
	return CreatePSO(vs, nullptr, nullptr, ps, gs, mRootSignature, &pVertexLayout, nullptr, false, 0, alpha, blend, NUL);
}

ID3D12PipelineState *Common::CreatePsoCompute(ID3DBlob *cs,
	ID3D12RootSignature *mRootSignature)
{
	ID3D12PipelineState *pso;

	D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc = {};
	PsoDesc.pRootSignature = mRootSignature;
	PsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(cs->GetBufferPointer()),
		cs->GetBufferSize()
	};
	PsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr;
	hr = dx->md3dDevice->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		ErrorMessage("Common::CreatePsoCompute Error!!"); return nullptr;
	}

	return pso;
}

ID3D12Resource *Common::GetSwapChainBuffer() {
	return dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get();
}

ID3D12Resource *Common::GetDepthStencilBuffer() {
	return dx->mDepthStencilBuffer.Get();
}

ID3D12Resource *Common::GetTexture(int Num) {
	return dx->texture[Num];
}

D3D12_RESOURCE_STATES Common::GetTextureStates() {
	return D3D12_RESOURCE_STATE_GENERIC_READ;
}

ID3D12Resource *Common::GetTextureUp(int Num) {
	return dx->textureUp[Num];
}

Microsoft::WRL::ComPtr<ID3DBlob> Common::CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName) {
	return dx->CompileShader(szFileName, size, szFuncName, szProfileName);
}

void Common::drawsub(drawPara para) {
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	ID3D12DescriptorHeap* descriptorHeaps[] = { para.srv };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(para.rootSignature);

	mCommandList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(para.srv->GetGPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.material[i].dwNumFace == 0)
		{
			continue;
		}
		mCommandList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());

		mCommandList->SetGraphicsRootDescriptorTable(0, tex);//(slotRootParameterIndex(shader内registerIndex), DESCRIPTOR_HANDLE)
		tex.Offset(1, dx->mCbvSrvUavDescriptorSize);//デスクリプタヒープのアドレス位置オフセットで次のテクスチャを読み込ませる
		if (para.material[i].nortex_no != -1) {
			mCommandList->IASetPrimitiveTopology(para.haveNortexTOPOLOGY);
			//normalMapが存在する場合diffuseの次に格納されている
			mCommandList->SetGraphicsRootDescriptorTable(1, tex);
			tex.Offset(1, dx->mCbvSrvUavDescriptorSize);
			mCommandList->SetPipelineState(para.haveNortexPSO);//normalMap有り無しでPSO切り替え
		}
		else {
			mCommandList->IASetPrimitiveTopology(para.notHaveNortexTOPOLOGY);
			mCommandList->SetPipelineState(para.notHaveNortexPSO);
		}

		mCommandList->SetGraphicsRootConstantBufferView(2, para.cbRes0->GetGPUVirtualAddress());
		UINT mElementByteSize = (sizeof(CONSTANT_BUFFER2) + 255) & ~255;
		mCommandList->SetGraphicsRootConstantBufferView(3, para.cbRes1->GetGPUVirtualAddress() + mElementByteSize * i);
		UINT viewIndex = 4;
		if (para.cbRes2 != nullptr)
			mCommandList->SetGraphicsRootConstantBufferView(viewIndex++, para.cbRes2->GetGPUVirtualAddress());
		if (para.sRes0 != nullptr)
			mCommandList->SetGraphicsRootShaderResourceView(viewIndex++, para.sRes0->GetGPUVirtualAddress());
		if (para.sRes1 != nullptr)
			mCommandList->SetGraphicsRootShaderResourceView(viewIndex++, para.sRes1->GetGPUVirtualAddress());

		mCommandList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}
