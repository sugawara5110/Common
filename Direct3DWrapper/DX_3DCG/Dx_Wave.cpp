//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Waveクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Wave.h"
#include "./Core/ShaderCG/ShaderWaveCom.h"
#include "./Core/ShaderCG/ShaderWaveDraw.h"

namespace {
	ComPtr<ID3DBlob> pComputeShader_Wave = nullptr;
	ComPtr<ID3DBlob> pDomainShader_Wave = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;

	bool createShaderDone = false;
}

void Wave::createShader() {

	if (createShaderDone)return;
	addChar Wave;
	Wave.addStr(mObj.getShaderCommonParameters(), ShaderWaveDraw);

	//Wave
	pComputeShader_Wave = mObj.CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "CS", "cs_5_1");
	pDomainShader_Wave = mObj.CompileShader(Wave.str, Wave.size, "DSWave", "ds_5_1");
	//メッシュレイアウト
	pVertexLayout_MESH =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "GEO_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	createShaderDone = true;
}

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
	Vertex* v = vertexArr;
	ver = new VertexM[numVer];
	for (int i = 0; i < numVer; i++) {
		ver[i].Pos.as(v[i].Pos.x, v[i].Pos.y, v[i].Pos.z);
		ver[i].normal.as(v[i].normal.x, v[i].normal.y, v[i].normal.z);
		ver[i].geoNormal.as(v[i].normal.x, v[i].normal.y, v[i].normal.z);
		ver[i].tex.as(v[i].tex.x, v[i].tex.y);
	}
	mObj.getVertexBuffer(sizeof(VertexM), numVer);
	index = new UINT[numInd];
	memcpy(index, ind, sizeof(UINT) * numInd);
	mObj.getIndexBuffer(0, sizeof(UINT) * numInd, numInd);
}

void Wave::GetVBarray(int numMaxInstance) {
	mObjectCB_WAVE = new ConstantBuffer<CONSTANT_BUFFER_WAVE>(1);
	mObj.getBuffer(1, numMaxInstance);
}

void Wave::GetShaderByteCode(bool smooth) {
	createShader();
	mObj.GetShaderByteCode(CONTROL_POINT, true, smooth, false, nullptr, pDomainShader_Wave.Get());
	cs = pComputeShader_Wave.Get();
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
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!");
		return false;
	}

	if (FAILED(Dx_Device::GetInstance()->createDefaultResourceBuffer_UNORDERED_ACCESS(&mOutputBuffer, byteSize))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!");
		return false;
	}
	mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", mObj.objName));
	mInputUploadBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputUploadBuffer", mObj.objName));
	mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", mObj.objName));

	mObj.comObj->ResourceBarrier(mOutputBuffer.Get(),
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
	mPSOCom = mObj.CreatePsoCompute(cs, mRootSignatureCom.Get());
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

bool Wave::DrawCreate(int texNo, int nortNo, bool blend, bool alpha, bool smooth, float divideBufferMagnification) {
	mObj.dpara.material[0].diftex_no = texNo;
	mObj.dpara.material[0].nortex_no = nortNo;
	mObj.dpara.material[0].spetex_no = mObj.dx->GetTexNumber("dummyDifSpe.");
	mObj.mObjectCB1->CopyData(0, sg);
	const int numSrvTex = 3;
	const int numSrvBuf = 1;
	const int numCbv = 3;
	mObj.setDivideArr(divArr, numDiv);

	UINT* indexCntArr = new UINT[mObj.dpara.NumMaterial];
	for (int m = 0; m < mObj.dpara.NumMaterial; m++) {
		indexCntArr[m] = mObj.dpara.Iview[m].IndexCount;
	}
	Dx_Util::createTangent(mObj.dpara.NumMaterial, indexCntArr,
		ver, &index, sizeof(VertexM), 0, 12 * 4, 6 * 4);
	ARR_DELETE(indexCntArr);

	mObj.createDefaultBuffer(ver, &index);
	ARR_DELETE(ver);
	ARR_DELETE(index);
	int numUav = 0;
	mObj.createParameterDXR(alpha, blend, divideBufferMagnification);
	mObj.setColorDXR(0, sg);
	if (!mObj.createPSO(pVertexLayout_MESH, numSrvTex + numSrvBuf, numCbv, numUav, blend, alpha))return false;
	if (!mObj.createPSO_DXR(pVertexLayout_MESH, numSrvTex + numSrvBuf, numCbv, numUav, smooth))return false;
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

bool Wave::Create(int texNo, bool blend, bool alpha, float waveHeight, float divide, bool smooth) {
	return Create(texNo, -1, blend, alpha, waveHeight, divide, smooth);
}

bool Wave::Create(int texNo, int nortNo, bool blend, bool alpha, float waveHeight, float divide, bool smooth,
	float divideBufferMagnification) {

	cbw.wHei_divide.as(waveHeight, divide, 0.0f, 0.0f);
	mObjectCB_WAVE->CopyData(0, cbw);
	GetShaderByteCode(smooth);
	if (!ComCreate())return false;
	return DrawCreate(texNo, nortNo, blend, alpha, smooth, divideBufferMagnification);
}

void Wave::Instancing(float speed, CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color) {
	cbw.speed = speed;
	mObjectCB_WAVE->CopyData(0, cbw);
	mObj.Instancing(pos, angle, size, Color);
}

void Wave::InstancingUpdate(float disp, float shininess,
	float px, float py, float mx, float my) {

	mObj.InstancingUpdate(disp, shininess, px, py, mx, my);
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

	mObj.SetCommandList(com);
	ID3D12GraphicsCommandList* mCList = mObj.mCommandList;
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
	if (!mObj.firstCbSet[mObj.cBuffSwapDrawOrStreamoutputIndex()] | !mObj.DrawOn)return;
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

HRESULT Wave::SetTextureMPixel(int com_no, BYTE* frame, int index) {
	return mObj.SetTextureMPixel(com_no, frame, index);
}