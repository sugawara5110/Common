//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          Common�N���X                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx12ProcessCore.h"

Common::Common() {
	dx = Dx12Process::GetInstance();
	comObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(0);
	mCommandList = comObj->getCommandList();
}

Common::~Common() {
	ARR_DELETE(movOn);

	for (int i = 0; i < numTexRes; i++) {
		if (createRes[i]) {
			RELEASE(textureUp[i]);
			RELEASE(texture[i]);
		}
	}
}

void Common::SetName(char* name) {
	if (strlen(name) > 255)return;
	strcpy(objName, name);
}

void Common::SetCommandList(int no) {
	com_no = no;

	comObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(com_no);
	mCommandList = comObj->getCommandList();
}

void Common::CopyResource(ID3D12Resource* Intexture, D3D12_RESOURCE_STATES res, int index) {

	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(com_no);

	cObj->ResourceBarrier(texture[index], D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	cObj->ResourceBarrier(Intexture, res, D3D12_RESOURCE_STATE_COPY_SOURCE);

	mCommandList->CopyResource(texture[index], Intexture);

	cObj->ResourceBarrier(Intexture, D3D12_RESOURCE_STATE_COPY_SOURCE, res);
	cObj->ResourceBarrier(texture[index], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
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

HRESULT Common::SetTextureMPixel(int com_no, BYTE* frame, int ind) {

	int index = movOn[ind].resIndex;

	D3D12_RESOURCE_DESC texdesc;
	texdesc = texture[index]->GetDesc();
	//�e�N�X�`���̉��T�C�Y�擾
	int width = (int)texdesc.Width;

	Dx_CommandListObj* cObj = Dx_CommandManager::GetInstance()->getGraphicsComListObj(com_no);

	cObj->ResourceBarrier(texture[index], D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	HRESULT hr = dx->CopyResourcesToGPU(com_no, textureUp[index], texture[index], frame, width * 4);
	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Common::SetTextureMPixel Error!!");
		return hr;
	}
	cObj->ResourceBarrier(texture[index], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	return S_OK;
}

HRESULT Common::createTex(int tNo, int& resCnt, char* upName, char* defName, char* ObjName) {
	InternalTexture* tex = &dx->texture[tNo];
	HRESULT hr = S_OK;
	if (tex->createRes) {
		textureUp[resCnt] = tex->textureUp.Get();
		texture[resCnt] = tex->texture.Get();
	}
	else {
		hr = dx->createTexture(com_no, tex->byteArr, tex->format,
			&textureUp[resCnt], &texture[resCnt],
			tex->width, tex->RowPitch, tex->height);
		textureUp[resCnt]->SetName(Dx_Util::charToLPCWSTR(upName, ObjName));
		texture[resCnt]->SetName(Dx_Util::charToLPCWSTR(defName, ObjName));
		if (SUCCEEDED(hr))createRes[resCnt] = true;
	}
	resCnt++;
	return hr;
}
HRESULT Common::createTextureResource(int resourceStartIndex, int MaterialNum, TextureNo* to, char* ObjName) {

	numTexRes = MaterialNum * 3;
	createRes = std::make_unique<bool[]>(numTexRes);
	for (int i = 0; i < numTexRes; i++)createRes[i] = false;
	textureUp = std::make_unique<ID3D12Resource* []>(numTexRes);
	texture = std::make_unique<ID3D12Resource* []>(numTexRes);
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

	Dx_Device* device = Dx_Device::GetInstance();
	HRESULT hr = S_OK;
	int resCnt = resourceStartIndex;
	for (int i = 0; i < MaterialNum; i++) {
		//diffuse, ����e�N�X�`��
		if (to[i].diffuse < 0 || movOn[i].m_on) {
			hr = device->textureInit(movOn[i].width, movOn[i].height,
				&textureUp[resCnt], &texture[resCnt],
				DXGI_FORMAT_R8G8B8A8_UNORM,
				D3D12_RESOURCE_STATE_GENERIC_READ);
			if (SUCCEEDED(hr))createRes[resCnt] = true;
			movOn[i].resIndex = resCnt;
			textureUp[resCnt]->SetName(Dx_Util::charToLPCWSTR("movTexUp", ObjName));
			texture[resCnt]->SetName(Dx_Util::charToLPCWSTR("movTex", ObjName));
			resCnt++;
		}
		else
		{
			hr = createTex(to[i].diffuse, resCnt, "diffuseTexUp", "diffuseTex", ObjName);
		}
		if (FAILED(hr)) {
			Dx_Util::ErrorMessage("Common::createTextureResource Error!!");
			return hr;
		}
		//normalMap�����݂���ꍇ
		if (to[i].normal >= 0) {
			hr = createTex(to[i].normal, resCnt, "normalTexUp", "normalTex", ObjName);
		}
		if (FAILED(hr)) {
			Dx_Util::ErrorMessage("Common::createTextureResource Error!!");
			return hr;
		}
		//specularMap�����݂���ꍇ
		if (to[i].specular >= 0) {
			hr = createTex(to[i].specular, resCnt, "specularTexUp", "specularTex", ObjName);
		}
		if (FAILED(hr)) {
			Dx_Util::ErrorMessage("Common::createTextureResource Error!!");
			return hr;
		}
	}
	return S_OK;
}

static void createROOT_PARAMETER(UINT numSrv, UINT numCbv, UINT numUav,
	std::vector<D3D12_DESCRIPTOR_RANGE>& range,
	std::vector<D3D12_ROOT_PARAMETER>& rootParams,
	UINT numCbvPara, UINT RegisterStNoCbv,
	UINT numArrCbv, UINT* numDescriptors) {

	UINT numRange = numSrv + numCbv + numUav + numArrCbv;
	range.resize(numRange);
	int ind = 0;
	int cnt = 0;
	for (UINT i = 0; i < numArrCbv; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[ind];
		r.BaseShaderRegister = i;
		r.NumDescriptors = numDescriptors[i];
		r.RegisterSpace = i + 1;//�������z���1�ȏ�
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt += numDescriptors[i];
		ind++;
	}
	for (UINT i = 0; i < numSrv; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[ind];
		r.BaseShaderRegister = i;
		r.NumDescriptors = 1;
		r.RegisterSpace = 0;
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt++;
		ind++;
	}
	for (UINT i = 0; i < numCbv; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[ind];
		r.BaseShaderRegister = i;
		r.NumDescriptors = 1;
		r.RegisterSpace = 0;
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt++;
		ind++;
	}
	for (UINT i = 0; i < numUav; i++) {
		D3D12_DESCRIPTOR_RANGE& r = range[ind];
		r.BaseShaderRegister = i;
		r.NumDescriptors = 1;
		r.RegisterSpace = 0;
		r.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		r.OffsetInDescriptorsFromTableStart = cnt;
		cnt++;
		ind++;
	}

	UINT numPara = numCbvPara + 1;//+1��cntp=0�̕�
	rootParams.resize(numPara);
	int cntp = 0;
	rootParams[cntp].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[cntp].DescriptorTable.NumDescriptorRanges = numRange;
	rootParams[cntp].DescriptorTable.pDescriptorRanges = range.data();
	rootParams[cntp].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	cntp++;
	for (UINT i = 0; i < numCbvPara; i++) {
		//DESCRIPTOR_TABLE�ȊO�p
		rootParams[cntp].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[cntp].Descriptor.ShaderRegister = RegisterStNoCbv + i;
		rootParams[cntp].Descriptor.RegisterSpace = 0;
		rootParams[cntp].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		cntp++;
	}
}

ComPtr <ID3D12RootSignature> Common::CreateRootSignature(UINT numSrv, UINT numCbv, UINT numUav,
	UINT numCbvPara, UINT RegisterStNoCbv, UINT numArrCbv, UINT* numDescriptors) {

	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	createROOT_PARAMETER(numSrv, numCbv, numUav, range, rootParams,
		numCbvPara, RegisterStNoCbv, numArrCbv, numDescriptors);
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

	return Dx_Device::GetInstance()->CreateRsCommon(&desc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsStreamOutput(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter)
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = paramNum;
	desc.pParameters = slotRootParameter;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return Dx_Device::GetInstance()->CreateRsCommon(&desc);
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

	return Dx_Device::GetInstance()->CreateRsCommon(&desc);
}

ComPtr <ID3D12RootSignature> Common::CreateRsCompute(int paramNum, D3D12_ROOT_PARAMETER* slotRootParameter)
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = paramNum;
	desc.pParameters = slotRootParameter;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	return Dx_Device::GetInstance()->CreateRsCommon(&desc);
}

ComPtr<ID3D12RootSignature> Common::CreateRootSignatureCompute(UINT numSrv, UINT numCbv, UINT numUav,
	UINT numCbvPara, UINT RegisterStNoCbv, UINT numArrCbv, UINT* numDescriptors) {

	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	createROOT_PARAMETER(numSrv, numCbv, numUav, range, rootParams, numCbvPara, RegisterStNoCbv, numArrCbv, numDescriptors);

	return CreateRsCompute(numCbvPara + 1, rootParams.data());
}

ComPtr<ID3D12RootSignature> Common::CreateRootSignatureStreamOutput(UINT numSrv, UINT numCbv, UINT numUav,
	bool sampler, UINT numCbvPara, UINT RegisterStNoCbv, UINT numArrCbv, UINT* numDescriptors) {

	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	createROOT_PARAMETER(numSrv, numCbv, numUav, range, rootParams, numCbvPara, RegisterStNoCbv, numArrCbv, numDescriptors);

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
		psoDesc.StreamOutput.NumEntries = numDECLARATION;  //D3D12_SO_DECLARATION_ENTRY�̌�
		psoDesc.StreamOutput.pBufferStrides = StreamSizeArr;
		psoDesc.StreamOutput.NumStrides = NumStrides;//�o�b�t�@�̐��H
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
	psoDesc.BlendState.AlphaToCoverageEnable = alpha;//�A���t�@�e�X�gon/off
	psoDesc.BlendState.RenderTarget[0].BlendEnable = blend;//�u�����hon/off
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
	hr = Dx_Device::GetInstance()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Common::CreatePSO Error!!"); return nullptr;
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
	hr = Dx_Device::GetInstance()->getDevice()->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("Common::CreatePsoCompute Error!!"); return nullptr;
	}

	return pso;
}

ComPtr<ID3DBlob> Common::CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName) {
	return dx->shaderH->CompileShader(szFileName, size, szFuncName, szProfileName);
}
