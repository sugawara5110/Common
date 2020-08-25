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
		//diffuse, 動画テクスチャ
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
		//specularMapが存在する場合
		if (to[i].specular >= 0) {
			InternalTexture* tex = &dx->texture[to[i].specular];
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

void Common::CreateSrvTexture(ID3D12DescriptorHeap* heap, int offsetHeap, ID3D12Resource** tex, int texNum)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(offsetHeap, dx->mCbvSrvUavDescriptorSize);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	for (int i = 0; i < texNum; i++) {
		srvDesc.Format = tex[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex[i]->GetDesc().MipLevels;
		dx->md3dDevice->CreateShaderResourceView(tex[i], &srvDesc, hDescriptor);
		hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
	}
}

void Common::CreateSrvBuffer(ID3D12DescriptorHeap* heap, int offsetHeap, ID3D12Resource** buffer, int bufNum,
	UINT* StructureByteStride)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(offsetHeap, dx->mCbvSrvUavDescriptorSize);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	for (int i = 0; i < bufNum; i++) {
		srvDesc.Buffer.StructureByteStride = StructureByteStride[i];
		srvDesc.Buffer.NumElements = (UINT)buffer[i]->GetDesc().Width / StructureByteStride[i];
		dx->md3dDevice->CreateShaderResourceView(buffer[i], &srvDesc, hDescriptor);
		hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
	}
}

void Common::CreateCbv(ID3D12DescriptorHeap* heap, int offsetHeap,
	D3D12_GPU_VIRTUAL_ADDRESS* virtualAddress, UINT* sizeInBytes, int bufNum)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(offsetHeap, dx->mCbvSrvUavDescriptorSize);
	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	for (int i = 0; i < bufNum; i++) {
		bufferDesc.SizeInBytes = sizeInBytes[i];
		bufferDesc.BufferLocation = virtualAddress[i];
		dx->md3dDevice->CreateConstantBufferView(&bufferDesc, hDescriptor);
		hDescriptor.Offset(1, dx->mCbvSrvUavDescriptorSize);
	}
}

ComPtr <ID3D12RootSignature> Common::CreateRootSignature(UINT numSrv, UINT numCbv) {

	UINT numPara = numSrv + numCbv;

	std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE[]>Table = nullptr;
	Table = std::make_unique<CD3DX12_DESCRIPTOR_RANGE[]>(numPara);
	int cnt = 0;
	for (UINT i = 0; i < numSrv; i++)
		Table[cnt++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i);//Descriptor 1個,(ti)
	for (UINT i = 0; i < numCbv; i++)
		Table[cnt++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i);//Descriptor 1個,(bi)

	std::unique_ptr<CD3DX12_ROOT_PARAMETER[]>slotRootParameter = nullptr;
	slotRootParameter = std::make_unique<CD3DX12_ROOT_PARAMETER[]>(numPara);

	for (UINT i = 0; i < numPara; i++)
		slotRootParameter[i].InitAsDescriptorTable(1, &Table[i], D3D12_SHADER_VISIBILITY_ALL);

	return CreateRs(numPara, slotRootParameter.get());
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

	return dx->CreateRsCommon(&rootSigDesc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsStreamOutput(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		NULL, NULL,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	return dx->CreateRsCommon(&rootSigDesc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsCompute(int paramNum, CD3DX12_ROOT_PARAMETER* slotRootParameter)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(paramNum, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	return dx->CreateRsCommon(&rootSigDesc);
}

ComPtr<ID3D12RootSignature> Common::CreateRootSignatureStreamOutput(UINT numSrv, UINT numCbv) {

	UINT numPara = numSrv + numCbv;

	std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE[]>Table = nullptr;
	Table = std::make_unique<CD3DX12_DESCRIPTOR_RANGE[]>(numPara);
	int cnt = 0;
	for (UINT i = 0; i < numSrv; i++)
		Table[cnt++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i);//Descriptor 1個,(ti)
	for (UINT i = 0; i < numCbv; i++)
		Table[cnt++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i);//Descriptor 1個,(bi)

	std::unique_ptr<CD3DX12_ROOT_PARAMETER[]>slotRootParameter = nullptr;
	slotRootParameter = std::make_unique<CD3DX12_ROOT_PARAMETER[]>(numPara);

	for (UINT i = 0; i < numPara; i++)
		slotRootParameter[i].InitAsDescriptorTable(1, &Table[i], D3D12_SHADER_VISIBILITY_ALL);

	return CreateRsStreamOutput(numPara, slotRootParameter.get());
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
	std::vector<D3D12_SO_DECLARATION_ENTRY>& pDeclaration, UINT StreamSize, PrimitiveType type)
{
	return CreatePSO(vs, nullptr, nullptr, nullptr, gs, mRootSignature, &pVertexLayout, &pDeclaration, true, StreamSize, true, true, type);
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

void Common::drawsubNonSOS(drawPara& para, ParameterDXR& dxr) {
	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCommandList->SetGraphicsRootSignature(para.rootSignature.Get());
	mCommandList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());

	CD3DX12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.Iview[i].IndexCount <= 0)continue;

		mCommandList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCommandList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCommandList->SetPipelineState(para.PSO[i].Get());

		for (int descInd = 0; descInd < para.numDesc; descInd++) {
			mCommandList->SetGraphicsRootDescriptorTable(descInd, heap);//(slotRootParameterIndex, DESCRIPTOR_HANDLE)
			heap.Offset(1, dx->mCbvSrvUavDescriptorSize);
		}
		mCommandList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);
	}

	dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void Common::drawsubSOS(drawPara& para, ParameterDXR& dxr) {

	CD3DX12_GPU_DESCRIPTOR_HANDLE heap(para.descHeap.Get()->GetGPUDescriptorHandleForHeapStart());
	for (int i = 0; i < para.NumMaterial; i++) {
		//使用されていないマテリアル対策
		if (para.Iview[i].IndexCount <= 0)continue;

		dx->Bigin(com_no);

		dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		ID3D12DescriptorHeap* descriptorHeaps[] = { para.descHeap.Get() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		mCommandList->SetGraphicsRootSignature(para.rootSignature.Get());
		mCommandList->IASetVertexBuffers(0, 1, &(para.Vview)->VertexBufferView());
		mCommandList->IASetIndexBuffer(&(para.Iview[i]).IndexBufferView());
		mCommandList->IASetPrimitiveTopology(para.TOPOLOGY);
		mCommandList->SetPipelineState(para.PSO[i].Get());

		for (int descInd = 0; descInd < para.numDesc; descInd++) {
			mCommandList->SetGraphicsRootDescriptorTable(descInd, heap);//(slotRootParameterIndex, DESCRIPTOR_HANDLE)
			heap.Offset(1, dx->mCbvSrvUavDescriptorSize);
		}

		mCommandList->SOSetTargets(0, 1, &dxr.SviewDXR[i].StreamBufferView());

		mCommandList->DrawIndexedInstanced(para.Iview[i].IndexCount, para.insNum, 0, 0, 0);

		dx->dx_sub[com_no].ResourceBarrier(dxr.VviewDXR[i].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		dx->dx_sub[com_no].ResourceBarrier(dxr.SviewDXR[i].StreamBufferGPU.Get(),
			D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);

		mCommandList->CopyResource(dxr.VviewDXR[i].VertexBufferGPU.Get(), dxr.SviewDXR[i].StreamBufferGPU.Get());

		dx->dx_sub[com_no].ResourceBarrier(dxr.VviewDXR[i].VertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		dx->dx_sub[com_no].ResourceBarrier(dxr.SviewDXR[i].StreamBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);

		mCommandList->SOSetTargets(0, 1, nullptr);

		dx->dx_sub[com_no].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		dx->End(com_no);
		dx->WaitFenceCurrent();
	}
}

void Common::drawsub(drawPara& para, ParameterDXR& dxr) {
	if (dx->DXR_ON) {
		drawsubSOS(para, dxr);
	}
	else {
		drawsubNonSOS(para, dxr);
	}
}
