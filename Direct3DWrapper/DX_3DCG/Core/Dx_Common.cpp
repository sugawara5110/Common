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
	if (movOnSize < index + 1) {
		if (!movOn) {
			movOn = new MovieTexture[index + 1];
		}
		else {
			MovieTexture* movOnT = new MovieTexture[index + 1];
			memcpy(movOnT, movOn, sizeof(MovieTexture) * movOnSize);
			ARR_DELETE(movOn);
			movOn = new MovieTexture[index + 1];
			memcpy(movOn, movOnT, sizeof(MovieTexture) * index + 1);
			ARR_DELETE(movOnT);
		}
		movOnSize = index + 1;
	}
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

	textureUp = std::make_unique<ComPtr<ID3D12Resource>[]>(MaterialNum * 3);
	texture = std::make_unique<ComPtr<ID3D12Resource>[]>(MaterialNum * 3);
	if (!movOn) {
		movOn = new MovieTexture[MaterialNum];
	}
	else {
		MovieTexture* movOnT = new MovieTexture[MaterialNum];
		if (movOnSize > MaterialNum)movOnSize = MaterialNum;
		memcpy(movOnT, movOn, sizeof(MovieTexture) * movOnSize);
		ARR_DELETE(movOn);
		movOn = new MovieTexture[MaterialNum];
		memcpy(movOn, movOnT, sizeof(MovieTexture) * MaterialNum);
		ARR_DELETE(movOnT);
	}
	movOnSize = MaterialNum;

	HRESULT hr = S_OK;
	int resCnt = resourceStartIndex - 1;
	for (int i = 0; i < MaterialNum; i++) {
		//diffuse, 動画テクスチャ
		if (to[i].diffuse < 0 || movOn[i].m_on) {
			hr = dx->device->textureInit(movOn[i].width, movOn[i].height,
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
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.ptr += offsetHeap * dx->mCbvSrvUavDescriptorSize;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	for (int i = 0; i < texNum; i++) {
		srvDesc.Format = tex[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex[i]->GetDesc().MipLevels;
		dx->getDevice()->CreateShaderResourceView(tex[i], &srvDesc, hDescriptor);
		hDescriptor.ptr += dx->mCbvSrvUavDescriptorSize;
	}
}

void Common::CreateSrvBuffer(ID3D12DescriptorHeap* heap, int offsetHeap, ID3D12Resource** buffer, int bufNum,
	UINT* StructureByteStride)
{
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.ptr += offsetHeap * dx->mCbvSrvUavDescriptorSize;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	for (int i = 0; i < bufNum; i++) {
		srvDesc.Buffer.StructureByteStride = StructureByteStride[i];
		srvDesc.Buffer.NumElements = (UINT)buffer[i]->GetDesc().Width / StructureByteStride[i];
		dx->getDevice()->CreateShaderResourceView(buffer[i], &srvDesc, hDescriptor);
		hDescriptor.ptr += dx->mCbvSrvUavDescriptorSize;
	}
}

void Common::CreateCbv(ID3D12DescriptorHeap* heap, int offsetHeap,
	D3D12_GPU_VIRTUAL_ADDRESS* virtualAddress, UINT* sizeInBytes, int bufNum)
{
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.ptr += offsetHeap * dx->mCbvSrvUavDescriptorSize;
	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	for (int i = 0; i < bufNum; i++) {
		bufferDesc.SizeInBytes = sizeInBytes[i];
		bufferDesc.BufferLocation = virtualAddress[i];
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, hDescriptor);
		hDescriptor.ptr += dx->mCbvSrvUavDescriptorSize;
	}
}

void Common::CreateUavBuffer(ID3D12DescriptorHeap* heap, int offsetHeap,
	ID3D12Resource** buffer, UINT* byteStride, UINT* bufferSize, int bufNum) {

	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(heap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.ptr += offsetHeap * dx->mCbvSrvUavDescriptorSize;
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.CounterOffsetInBytes = 0;//pCounterResourceがnullptrの場合0
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	for (int i = 0; i < bufNum; i++) {
		uavDesc.Buffer.StructureByteStride = byteStride[i];
		uavDesc.Buffer.NumElements = bufferSize[i];
		dx->getDevice()->CreateUnorderedAccessView(buffer[i], nullptr, &uavDesc, hDescriptor);
		hDescriptor.ptr += dx->mCbvSrvUavDescriptorSize;
	}
}

static void createROOT_PARAMETER(UINT numSrv, UINT numCbv, UINT numUav,
	std::vector<D3D12_DESCRIPTOR_RANGE>& range,
	std::vector<D3D12_ROOT_PARAMETER>& rootParams,
	UINT numCbvPara, UINT RegisterStNoCbv) {

	UINT numRange = numSrv + numCbv + numUav;
	range.resize(numRange);
	int cnt = 0;
	for (UINT i = 0; i < numSrv; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[cnt];
		r.BaseShaderRegister = i;
		r.NumDescriptors = 1;
		r.RegisterSpace = 0;
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt++;
	}
	for (UINT i = 0; i < numCbv; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[cnt];
		r.BaseShaderRegister = i;
		r.NumDescriptors = 1;
		r.RegisterSpace = 0;
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt++;
	}
	for (UINT i = 0; i < numUav; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[cnt];
		r.BaseShaderRegister = i;
		r.NumDescriptors = 1;
		r.RegisterSpace = 0;
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt++;
	}

	UINT numPara = numCbvPara + 1;
	rootParams.resize(numPara);
	int cntp = 0;
	rootParams[cntp].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[cntp].DescriptorTable.NumDescriptorRanges = numRange;
	rootParams[cntp].DescriptorTable.pDescriptorRanges = range.data();
	rootParams[cntp].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	cntp++;
	for (UINT i = 0; i < numCbvPara; i++) {
		rootParams[cntp].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[cntp].Descriptor.ShaderRegister = RegisterStNoCbv + i;
		rootParams[cntp].Descriptor.RegisterSpace = 0;
		rootParams[cntp].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		cntp++;
	}
}

ComPtr <ID3D12RootSignature> Common::CreateRootSignature(UINT numSrv, UINT numCbv, UINT numUav,
	UINT numCbvPara, UINT RegisterStNoCbv) {

	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	createROOT_PARAMETER(numSrv, numCbv, numUav, range, rootParams, numCbvPara, RegisterStNoCbv);
	return CreateRs(numCbvPara + 1, rootParams.data());
}

static D3D12_STATIC_SAMPLER_DESC getSampler() {
	D3D12_STATIC_SAMPLER_DESC linearWrap = {};
	linearWrap.ShaderRegister = 0;
	linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearWrap.MipLODBias = 0;
	linearWrap.MaxAnisotropy = 16;
	linearWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	linearWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	linearWrap.MinLOD = 0.0f;
	linearWrap.MaxLOD = D3D12_FLOAT32_MAX;
	linearWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	linearWrap.RegisterSpace = 0;
	return linearWrap;
}

ComPtr <ID3D12RootSignature> Common::CreateRs(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter)
{
	D3D12_STATIC_SAMPLER_DESC linearWrap = getSampler();
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = paramNum;
	desc.pParameters = slotRootParameter;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &linearWrap;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return dx->device->CreateRsCommon(&desc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsStreamOutput(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter)
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = paramNum;
	desc.pParameters = slotRootParameter;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return dx->device->CreateRsCommon(&desc);
}

ComPtr<ID3D12RootSignature> Common::CreateRsStreamOutputSampler(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter)
{
	D3D12_STATIC_SAMPLER_DESC linearWrap = getSampler();
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = paramNum;
	desc.pParameters = slotRootParameter;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &linearWrap;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return dx->device->CreateRsCommon(&desc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsCompute(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter)
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = paramNum;
	desc.pParameters = slotRootParameter;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	return dx->device->CreateRsCommon(&desc);
}

ComPtr<ID3D12RootSignature> Common::CreateRootSignatureCompute(UINT numSrv, UINT numCbv, UINT numUav,
	UINT numCbvPara, UINT RegisterStNoCbv) {

	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	createROOT_PARAMETER(numSrv, numCbv, numUav, range, rootParams, numCbvPara, RegisterStNoCbv);

	return CreateRsCompute(numCbvPara + 1, rootParams.data());
}

ComPtr<ID3D12RootSignature> Common::CreateRootSignatureStreamOutput(UINT numSrv, UINT numCbv, UINT numUav,
	bool sampler, UINT numCbvPara, UINT RegisterStNoCbv) {

	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	createROOT_PARAMETER(numSrv, numCbv, numUav, range, rootParams, numCbvPara, RegisterStNoCbv);

	if (sampler)
		return CreateRsStreamOutputSampler(numCbvPara + 1, rootParams.data());
	else
		return CreateRsStreamOutput(numCbvPara + 1, rootParams.data());
}

ComPtr <ID3D12PipelineState> Common::CreatePSO(ID3DBlob* vs, ID3DBlob* hs,
	ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>* pVertexLayout,
	bool STREAM_OUTPUT,
	std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
	UINT numDECLARATION,
	UINT* StreamSizeArr,
	UINT NumStrides,
	bool alpha, bool blend,
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
		psoDesc.StreamOutput.NumEntries = numDECLARATION;  //D3D12_SO_DECLARATION_ENTRYの個数
		psoDesc.StreamOutput.pBufferStrides = StreamSizeArr;
		psoDesc.StreamOutput.NumStrides = NumStrides;//バッファの数？
		psoDesc.StreamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;
	}

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	if (dx->wireframe)
		rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	else
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.RasterizerState = rasterizerDesc;

	D3D12_BLEND_DESC blendDesc = {};
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

	psoDesc.BlendState = blendDesc;

	psoDesc.BlendState.IndependentBlendEnable = false;
	psoDesc.BlendState.AlphaToCoverageEnable = alpha;//アルファテストon/off
	psoDesc.BlendState.RenderTarget[0].BlendEnable = blend;//ブレンドon/off
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	D3D12_DEPTH_STENCIL_DESC stencilDesc = {};
	stencilDesc.DepthEnable = TRUE;
	stencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	stencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	stencilDesc.StencilEnable = FALSE;
	stencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	stencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	stencilDesc.FrontFace = defaultStencilOp;
	stencilDesc.BackFace = defaultStencilOp;

	psoDesc.DepthStencilState = stencilDesc;
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
	hr = dx->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
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
	return CreatePSO(vs, nullptr, nullptr, ps, nullptr, mRootSignature, &pVertexLayout,
		false, nullptr, 0, nullptr, 0, alpha, blend, type);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoVsHsDsPs(ID3DBlob* vs, ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* ps, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend,
	PrimitiveType type)
{
	return CreatePSO(vs, hs, ds, ps, gs, mRootSignature, &pVertexLayout,
		false, nullptr, 0, nullptr, 0, alpha, blend, type);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoStreamOutput(ID3DBlob* vs, ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	std::vector<D3D12_SO_DECLARATION_ENTRY>* pDeclaration,
	UINT numDECLARATION,
	UINT* StreamSizeArr,
	UINT NumStrides,
	PrimitiveType type)
{
	return CreatePSO(vs, hs, ds, nullptr, gs, mRootSignature, &pVertexLayout,
		true, pDeclaration, numDECLARATION, StreamSizeArr, NumStrides, true, true, type);
}

ComPtr <ID3D12PipelineState> Common::CreatePsoParticle(ID3DBlob* vs, ID3DBlob* ps, ID3DBlob* gs,
	ID3D12RootSignature* mRootSignature,
	std::vector<D3D12_INPUT_ELEMENT_DESC>& pVertexLayout,
	bool alpha, bool blend)
{
	return CreatePSO(vs, nullptr, nullptr, ps, gs, mRootSignature, &pVertexLayout,
		false, nullptr, 0, nullptr, 0, alpha, blend, POINt);
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
	hr = dx->getDevice()->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&pso));
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
	return dx->shaderH->CompileShader(szFileName, size, szFuncName, szProfileName);
}

