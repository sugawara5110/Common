//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Waveクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Wave.h"

Wave::Wave() {
	sg.vDiffuse.x = 1.0f;
	sg.vDiffuse.y = 1.0f;
	sg.vDiffuse.z = 1.0f;
	sg.vDiffuse.w = 1.0f;

	sg.vSpeculer.x = 1.0f;
	sg.vSpeculer.y = 1.0f;
	sg.vSpeculer.z = 1.0f;
	sg.vSpeculer.w = 1.0f;

	sg.vAmbient.x = 0.0f;
	sg.vAmbient.y = 0.0f;
	sg.vAmbient.z = 0.0f;
	sg.vAmbient.w = 0.0f;

	divArr[0].distance = 1000.0f;
	divArr[0].divide = 2;//頂点数 3 → 3 * 6 = 18
	divArr[1].distance = 500.0f;
	divArr[1].divide = 48;//頂点数 3 → 3 * 3456 = 10368
	divArr[2].distance = 300.0f;
	divArr[2].divide = 96;//頂点数 3 → 3 * 13824 = 41472
	numDiv = 3;
}

Wave::~Wave() {
	S_DELETE(mObjectCB_WAVE);
}

void Wave::SetVertex(Vertex* vertexArr, int numVer, UINT* ind, int numInd) {
	mObj.setVertex(vertexArr, numVer, ind, numInd);
}

void Wave::GetVBarray(int numMaxInstance) {
	mObjectCB_WAVE = new ConstantBuffer<CONSTANT_BUFFER_WAVE>(1);
	mObj.getBuffer(1, numMaxInstance);
}

void Wave::GetShaderByteCode() {
	Dx12Process* dx = mObj.dx;
	Dx_ShaderHolder* sh = dx->shaderH.get();
	mObj.cs = sh->pComputeShader_Wave.Get();
	mObj.vs = sh->pVertexShader_MESH_D.Get();
	mObj.ps = sh->pPixelShader_3D.Get();
	mObj.ps_NoMap = sh->pPixelShader_3D_NoNormalMap.Get();
	mObj.hs = sh->pHullShaderTriangle.Get();
	mObj.ds = sh->pDomainShader_Wave.Get();
	mObj.gs = sh->pGeometryShader_Before_ds.Get();
	mObj.gs_NoMap = sh->pGeometryShader_Before_ds_NoNormalMap.Get();
}

bool Wave::ComCreate() {

	//CSからDSへの受け渡し用
	int divide = (int)(cbw.wHei_divide.y * cbw.wHei_divide.y);
	div = (int)(cbw.wHei_divide.y / 32.0f);//32はCS内スレッド数
	std::vector<WaveData> wdata(divide);
	for (int i = 0; i < divide; ++i)
	{
		wdata[i].sinWave = 0.0f;
		wdata[i].theta = (float)(i % 360);
	}
	Dx12Process* dx = mObj.dx;
	UINT64 byteSize = wdata.size() * sizeof(WaveData);

	mInputBuffer = dx->CreateDefaultBuffer(mObj.com_no, wdata.data(), byteSize, mInputUploadBuffer, true);
	if (!mInputBuffer) {
		ErrorMessage("Wave::ComCreate Error!!");
		return false;
	}

	if (FAILED(dx->device->createDefaultResourceBuffer_UNORDERED_ACCESS(&mOutputBuffer, byteSize))) {
		ErrorMessage("Wave::ComCreate Error!!");
		return false;
	}

	dx->dx_sub[mObj.com_no].ResourceBarrier(mOutputBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//ルートシグネチャ
	D3D12_ROOT_PARAMETER rootParameter[3] = {};
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParameter[0].Descriptor.ShaderRegister = 0;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParameter[1].Descriptor.ShaderRegister = 1;
	rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[2].Descriptor.ShaderRegister = 0;
	rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	mRootSignatureCom = mObj.CreateRsCompute(3, rootParameter);
	if (mRootSignatureCom == nullptr)return false;

	//PSO
	mPSOCom = mObj.CreatePsoCompute(mObj.cs, mRootSignatureCom.Get());
	if (mPSOCom == nullptr)return false;

	return true;
}

void Wave::SetCol(float difR, float difG, float difB, float speR, float speG, float speB,
	float amR, float amG, float amB) {
	sg.vDiffuse.x = difR;
	sg.vDiffuse.y = difG;
	sg.vDiffuse.z = difB;

	sg.vSpeculer.x = speR;
	sg.vSpeculer.y = speG;
	sg.vSpeculer.z = speB;

	sg.vAmbient.x = amR;
	sg.vAmbient.y = amG;
	sg.vAmbient.z = amB;
}

bool Wave::DrawCreate(int texNo, int nortNo, bool blend, bool alpha, float divideBufferMagnification) {
	mObj.dpara.material[0].diftex_no = texNo;
	mObj.dpara.material[0].nortex_no = nortNo;
	mObj.dpara.material[0].spetex_no = mObj.dx->GetTexNumber("dummyDifSpe.");
	mObj.dpara.TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	mObj.mObjectCB1->CopyData(0, sg);
	const int numSrvTex = 3;
	const int numSrvBuf = 1;
	const int numCbv = 3;
	mObj.primType_create = CONTROL_POINT;
	mObj.setDivideArr(divArr, numDiv);

	UINT* indexCntArr = new UINT[mObj.dpara.NumMaterial];
	for (int m = 0; m < mObj.dpara.NumMaterial; m++) {
		indexCntArr[m] = mObj.dpara.Iview[m].IndexCount;
	}
	Dx_Util::createTangent(mObj.dpara.NumMaterial, indexCntArr,
		mObj.ver, mObj.index, sizeof(VertexM), 0, 12 * 4, 6 * 4);
	ARR_DELETE(indexCntArr);

	mObj.createDefaultBuffer(mObj.ver, mObj.index);
	VertexM* vm = (VertexM*)mObj.ver;
	ARR_DELETE(vm);
	ARR_DELETE(mObj.index);
	int numUav = 0;
	mObj.createParameterDXR(alpha, blend, divideBufferMagnification);
	mObj.setColorDXR(0, sg);
	if (!mObj.createPSO(mObj.dx->shaderH->pVertexLayout_MESH, numSrvTex + numSrvBuf, numCbv, numUav, blend, alpha))return false;
	if (!mObj.createPSO_DXR(mObj.dx->shaderH->pVertexLayout_MESH, numSrvTex + numSrvBuf, numCbv, numUav))return false;
	UINT cbSize = mObjectCB_WAVE->getSizeInBytes();
	D3D12_GPU_VIRTUAL_ADDRESS ad = mObjectCB_WAVE->Resource()->GetGPUVirtualAddress();
	ID3D12Resource* res[1] = {};
	res[0] = mOutputBuffer.Get();
	UINT buSize[1] = {};
	buSize[0] = sizeof(WaveData);
	if (!mObj.setDescHeap(numSrvTex, numSrvBuf, res, buSize, numCbv, ad, cbSize))return false;
	return true;
}

void Wave::setMaterialType(MaterialType type) {
	mObj.dxrPara.mType[0] = type;
}

bool Wave::Create(int texNo, bool blend, bool alpha, float waveHeight, float divide) {
	return Create(texNo, -1, blend, alpha, waveHeight, divide);
}

bool Wave::Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide) {
	cbw.wHei_divide.as(waveHeight, divide, 0.0f, 0.0f);
	mObjectCB_WAVE->CopyData(0, cbw);
	GetShaderByteCode();
	if (!ComCreate())return false;
	return DrawCreate(texNo, nortNo, blend, alpha);
}

void Wave::Instancing(float speed, CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size) {
	cbw.speed = speed;
	mObjectCB_WAVE->CopyData(0, cbw);
	mObj.Instancing(pos, angle, size);
}

void Wave::InstancingUpdate(CoordTf::VECTOR4 Color, float disp, float shininess,
	float px, float py, float mx, float my) {

	mObj.InstancingUpdate(Color, disp, shininess, px, py, mx, my);
}

void Wave::Update(float speed, CoordTf::VECTOR3 pos, CoordTf::VECTOR4 Color,
	CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size,
	float disp, float shininess,
	float px, float py, float mx, float my) {

	cbw.speed = speed;
	mObjectCB_WAVE->CopyData(0, cbw);
	mObj.Update(pos, Color, angle, size, disp, shininess, px, py, mx, my);
}

void Wave::DrawOff() {
	mObj.DrawOff();
}

void Wave::Compute(int com) {

	ID3D12GraphicsCommandList* mCList = mObj.dx->dx_sub[com].mCommandList.Get();
	mCList->SetPipelineState(mPSOCom.Get());

	mCList->SetComputeRootSignature(mRootSignatureCom.Get());

	mCList->SetComputeRootUnorderedAccessView(0, mInputBuffer->GetGPUVirtualAddress());
	mCList->SetComputeRootUnorderedAccessView(1, mOutputBuffer->GetGPUVirtualAddress());
	mCList->SetComputeRootConstantBufferView(2, mObjectCB_WAVE->Resource()->GetGPUVirtualAddress());

	mCList->Dispatch(div, div, 1);

	auto tmp = mInputBuffer;
	mInputBuffer = mOutputBuffer;
	mOutputBuffer = tmp;
}

void Wave::Draw(int com_no) {
	if (!mObj.firstCbSet[mObj.dx->cBuffSwap[1]] | !mObj.DrawOn)return;
	Compute(com_no);
	mObj.Draw(com_no);
}

void Wave::StreamOutput(int com_no) {
	Dx12Process* dx = mObj.dx;
	Compute(com_no);
	mObj.StreamOutput(com_no);
}

void Wave::Draw() {
	Draw(mObj.com_no);
}

void Wave::StreamOutput() {
	StreamOutput(mObj.com_no);
}

void Wave::SetCommandList(int no) {
	mObj.SetCommandList(no);
}

void Wave::CopyResource(ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index) {
	mObj.CopyResource(texture, res, index);
}

void Wave::TextureInit(int width, int height, int index) {
	mObj.TextureInit(width, height, index);
}

HRESULT Wave::SetTextureMPixel(BYTE* frame, int index) {
	return mObj.SetTextureMPixel(frame, index);
}