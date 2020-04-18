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

void Common::CopyResource(ID3D12Resource* Intexture, D3D12_RESOURCE_STATES res, int index) {
	dx->dx_sub[com_no].ResourceBarrier(texture[index].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	dx->dx_sub[com_no].ResourceBarrier(Intexture, res, D3D12_RESOURCE_STATE_COPY_SOURCE);

	mCommandList->CopyResource(texture[index].Get(), Intexture);

	dx->dx_sub[com_no].ResourceBarrier(Intexture, D3D12_RESOURCE_STATE_COPY_SOURCE, res);
	dx->dx_sub[com_no].ResourceBarrier(texture[index].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void Common::TextureInit(int width, int height, int index) {
	movOn[index].m_on = true;
	movOn[index].width = width;
	movOn[index].height = height;
}

HRESULT Common::SetTextureMPixel(BYTE* frame, int ind) {

	int index = movOn[ind].resIndex;

	D3D12_RESOURCE_DESC texdesc;
	texdesc = texture[index].Get()->GetDesc();
	//テクスチャの横サイズ取得
	int width = (int)texdesc.Width;

	dx->dx_sub[com_no].ResourceBarrier(texture[index].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	HRESULT hr = dx->CopyResourcesToGPU(com_no, textureUp[index].Get(), texture[index].Get(), frame, width * 4);
	if (FAILED(hr)) {
		ErrorMessage("Common::SetTextureMPixel Error!!");
		return hr;
	}
	dx->dx_sub[com_no].ResourceBarrier(texture[index].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	return S_OK;
}

HRESULT Common::createTextureResource(int resourceStartIndex, int MaterialNum, TextureNo* to) {

	HRESULT hr = S_OK;
	int resCnt = resourceStartIndex - 1;
	for (int i = 0; i < MaterialNum; i++) {
		//diffuse
		if (to[i].diffuse < 0 || movOn[i].m_on) {
			hr = dx->textureInit(movOn[i].width, movOn[i].height,
				textureUp[++resCnt].GetAddressOf(), texture[resCnt].GetAddressOf(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				D3D12_RESOURCE_STATE_GENERIC_READ);
			movOn[i].resIndex = resCnt;
		}
		else
		{
			InternalTexture* tex = &dx->texture[to[i].diffuse];
			hr = dx->createTexture(com_no, tex->byteArr, tex->format,
				textureUp[++resCnt].GetAddressOf(), texture[resCnt].GetAddressOf(),
				tex->width, tex->RowPitch, tex->height);
		}
		if (FAILED(hr)) {
			ErrorMessage("Common::createTextureResource Error!!");
			return hr;
		}
		//normalMapが存在する場合
		if (to[i].normal >= 0) {
			InternalTexture* tex = &dx->texture[to[i].normal];
			hr = dx->createTexture(com_no, tex->byteArr, tex->format,
				textureUp[++resCnt].GetAddressOf(), texture[resCnt].GetAddressOf(),
				tex->width, tex->RowPitch, tex->height);
		}
		if (FAILED(hr)) {
			ErrorMessage("Common::createTextureResource Error!!");
			return hr;
		}
	}
	return S_OK;
}

ComPtr <ID3D12DescriptorHeap> Common::CreateSrvHeap(int resourceStartIndex, int texNum, TextureNo* to)
{
	ComPtr <ID3D12DescriptorHeap>srv;

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
	for (int i = resourceStartIndex; i < texNum + resourceStartIndex; i++) {
		srvDesc.Format = texture[i].Get()->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture[i].Get()->GetDesc().MipLevels;
		dx->md3dDevice->CreateShaderResourceView(texture[i].Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);

	}
	return srv;
}

ComPtr <ID3D12RootSignature>Common::CreateRsCommon(CD3DX12_ROOT_SIGNATURE_DESC* rootSigDesc)
{
	ComPtr <ID3D12RootSignature>rs;

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

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

ComPtr <ID3D12RootSignature> Common::CreateRs(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter)
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

ComPtr <ID3D12RootSignature> Common::CreateRsStreamOutput(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		NULL, NULL,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	return CreateRsCommon(&rootSigDesc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsCompute(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	return CreateRsCommon(&rootSigDesc);
}

ComPtr <ID3D12PipelineState> Common::CreatePSO(ID3DBlob* vs, ID3DBlob* hs,
	ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>* pVertexLayout,
	std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
	bool STREAM_OUTPUT, UINT StreamSize, bool alpha, bool blend,
	PrimitiveType type)
{
	ComPtr <ID3D12PipelineState> pso;

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

ComPtr <ID3D12PipelineState> Common::CreatePsoVsPs(ID3DBlob* vs, ID3DBlob* ps,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend,
	PrimitiveType type)
{
	return CreatePSO(vs, nullptr, nullptr, ps, nullptr, mRootSignature, &pVertexLayout, nullptr, false, 0, alpha, blend, type);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoVsHsDsPs(ID3DBlob* vs, ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend,
	PrimitiveType type)
{
	return CreatePSO(vs, hs, ds, ps, gs, mRootSignature, &pVertexLayout, nullptr, false, 0, alpha, blend, type);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoStreamOutput(ID3DBlob* vs, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	std::vector<D3D12_SO_DECLARATION_ENTRY>& pDeclaration, UINT StreamSize)
{
	return CreatePSO(vs, nullptr, nullptr, nullptr, gs, mRootSignature, &pVertexLayout, &pDeclaration, true, StreamSize, true, true, POINt);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoParticle(ID3DBlob* vs, ID3DBlob* ps, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend)
{
	return CreatePSO(vs, nullptr, nullptr, ps, gs, mRootSignature, &pVertexLayout, nullptr, false, 0, alpha, blend, POINt);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoCompute(ID3DBlob* cs,
	ID3D12RootSignature* mRootSignature)
{
	ComPtr <ID3D12PipelineState>pso;

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

D3D12_RESOURCE_STATES Common::GetTextureStates() {
	return D3D12_RESOURCE_STATE_GENERIC_READ;
}

ComPtr<ID3DBlob> Common::CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName) {
	return dx->CompileShader(szFileName, size, szFuncName, szProfileName);
}

void Common::drawsub(drawPara& para) {
	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	ID3D12DescriptorHeap* descriptorHeaps[] = { para.srvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(para.rootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(para.srvHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.Iview[i].IndexCount <= 0)continue;

		mCommandList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());

		mCommandList->SetGraphicsRootDescriptorTable(0, tex);//(slotRootParameterIndex(shader内registerIndex), DESCRIPTOR_HANDLE)
		tex.Offset(1, dx->mCbvSrvUavDescriptorSize);//デスクリプタヒープのアドレス位置オフセットで次のテクスチャを読み込ませる
		mCommandList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCommandList->SetGraphicsRootDescriptorTable(1, tex);
		tex.Offset(1, dx->mCbvSrvUavDescriptorSize);
		mCommandList->SetPipelineState(para.PSO.Get());

		mCommandList->SetGraphicsRootConstantBufferView(2, para.cbRes0.Get()->GetGPUVirtualAddress());
		UINT mElementByteSize = (sizeof(CONSTANT_BUFFER2) + 255) & ~255;
		mCommandList->SetGraphicsRootConstantBufferView(3, para.cbRes1.Get()->GetGPUVirtualAddress() + mElementByteSize * i);
		UINT viewIndex = 4;
		if (para.cbRes2 != nullptr)
			mCommandList->SetGraphicsRootConstantBufferView(viewIndex++, para.cbRes2.Get()->GetGPUVirtualAddress());
		if (para.sRes0 != nullptr)
			mCommandList->SetGraphicsRootShaderResourceView(viewIndex++, para.sRes0.Get()->GetGPUVirtualAddress());
		if (para.sRes1 != nullptr)
			mCommandList->SetGraphicsRootShaderResourceView(viewIndex++, para.sRes1.Get()->GetGPUVirtualAddress());

		mCommandList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);
	}

	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}
