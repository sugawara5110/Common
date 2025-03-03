//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          Waveクラス                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_Wave.h"
#include "Shader/ShaderWaveCom.h"
#include "Shader/ShaderWaveDraw.h"

namespace {
	ComPtr<ID3DBlob> pComputeShader_Wave[3] = {};
	ComPtr<ID3DBlob> pDomainShader_Wave = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;

	bool createShaderDone = false;
}

void Wave::createShader() {

	if (createShaderDone)return;

	addChar Com, Wave;
	Com.addStr(Dx_ShaderHolder::ShaderCommonParametersCopy.get(), Dx_ShaderHolder::ShaderNormalTangentCopy.get());
	Wave.addStr(Com.str, ShaderWaveDraw);

	//Wave
	pComputeShader_Wave[0] = Dx_ShaderHolder::CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "sinWavesCS", "cs_5_1");
	pComputeShader_Wave[1] = Dx_ShaderHolder::CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "DisturbWavesCS", "cs_5_1");
	pComputeShader_Wave[2] = Dx_ShaderHolder::CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "UpdateWavesCS", "cs_5_1");

	pDomainShader_Wave = Dx_ShaderHolder::CompileShader(Wave.str, Wave.size, "DSWave", "ds_5_1");
	
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
	divArr[2].divide = 64;//頂点数 3 → 3 * 13824 = 41472
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
	BasicPolygon::getVertexBuffer(sizeof(VertexM), numVer);
	index = new UINT[numInd];
	memcpy(index, ind, sizeof(UINT) * numInd);
	BasicPolygon::getIndexBuffer(0, sizeof(UINT) * numInd, numInd);
}

void Wave::GetVBarray(int numMaxInstance) {
	mObjectCB_WAVE = new ConstantBuffer<CONSTANT_BUFFER_WAVE>(1);
	BasicPolygon::getBuffer(1, numMaxInstance);
}

void Wave::GetShaderByteCode(bool smooth) {
	createShader();
	BasicPolygon::GetShaderByteCode(CONTROL_POINT, true, smooth, false, nullptr, pDomainShader_Wave.Get());
	cs[0] = pComputeShader_Wave[0].Get();
	cs[1] = pComputeShader_Wave[1].Get();
	cs[2] = pComputeShader_Wave[2].Get();
}

bool Wave::ComCreateSin(int comIndex) {
	//CSからDSへの受け渡し用
	int divide = width * width;

	std::vector<float> tdata(divide);

	for (int i = 0; i < divide; ++i)
	{
		tdata[i] = (float)(i % 360);
	}
	Dx_Device* device = Dx_Device::GetInstance();
	Dx_CommandManager* cMa = Dx_CommandManager::GetInstance();
	Dx_CommandListObj* cObj = cMa->getGraphicsComListObj(comIndex);
	UINT64 byteSize = tdata.size() * sizeof(float);

	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mInputBufferSin.GetAddressOf(), (UINT64)width, (UINT)width,
		D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT_R32_FLOAT))) {

		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	UINT64 uploadBufferSize = device->getRequiredIntermediateSize(mInputBufferSin.Get());
	if (FAILED(device->createUploadResource(mInputUploadBufferSin.GetAddressOf(), uploadBufferSize))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}

	cObj->ResourceBarrier(mInputBufferSin.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	if (FAILED(cObj->CopyResourcesToGPU(mInputUploadBufferSin.Get(), mInputBufferSin.Get(), tdata.data(), (LONG_PTR)width * sizeof(float)))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	cObj->ResourceBarrier(mInputBufferSin.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	mInputBufferSin.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBufferSin", BasicPolygon::objName));
	mInputUploadBufferSin.Get()->SetName(Dx_Util::charToLPCWSTR("mInputUploadBufferSin", BasicPolygon::objName));

	//PSO
	mPSOCom[0] = BasicPolygon::CreatePsoCompute(cs[0], mRootSignatureCom.Get());
	if (mPSOCom[0] == nullptr)return false;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->getDevice()->CreateUnorderedAccessView(mInputBufferSin.Get(), nullptr, &uavDesc, descHandleCPU);
	descHandleCPU.ptr += device->getCbvSrvUavDescriptorSize();

	return true;
}

bool Wave::ComCreateRipples(int comIndex) {

	Dx_Device* device = Dx_Device::GetInstance();
	int divide = width * width;
	std::vector<float> zdata(divide);
	for (int i = 0; i < divide; ++i)
	{
		zdata[i] = 0.0f;
	}

	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mInputBuffer.GetAddressOf(), (UINT64)width, (UINT)width,
		D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT_R32_FLOAT))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}

	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mPrevInputBuffer.GetAddressOf(), (UINT64)width, (UINT)width,
		D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT_R32_FLOAT))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}

	mInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mInputBuffer", BasicPolygon::objName));
	mPrevInputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mPrevInputBuffer", BasicPolygon::objName));

	UINT64 uploadBufferSize = device->getRequiredIntermediateSize(mInputBuffer.Get());
	if (FAILED(device->createUploadResource(mInputBufferUp.GetAddressOf(), uploadBufferSize))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	uploadBufferSize = device->getRequiredIntermediateSize(mPrevInputBuffer.Get());
	if (FAILED(device->createUploadResource(mPrevInputBufferUp.GetAddressOf(), uploadBufferSize))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	uploadBufferSize = device->getRequiredIntermediateSize(mOutputBuffer.Get());
	if (FAILED(device->createUploadResource(mOutputBufferUp.GetAddressOf(), uploadBufferSize))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}

	Dx_CommandManager* cMa = Dx_CommandManager::GetInstance();
	Dx_CommandListObj* cObj = cMa->getGraphicsComListObj(comIndex);

	cObj->ResourceBarrier(mInputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	if (FAILED(cObj->CopyResourcesToGPU(mInputBufferUp.Get(), mInputBuffer.Get(), zdata.data(), (LONG_PTR)width * sizeof(float)))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	cObj->ResourceBarrier(mInputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cObj->ResourceBarrier(mPrevInputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	if (FAILED(cObj->CopyResourcesToGPU(mPrevInputBufferUp.Get(), mPrevInputBuffer.Get(), zdata.data(), (LONG_PTR)width * sizeof(float)))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	cObj->ResourceBarrier(mPrevInputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cObj->ResourceBarrier(mOutputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	if (FAILED(cObj->CopyResourcesToGPU(mOutputBufferUp.Get(), mOutputBuffer.Get(), zdata.data(), (LONG_PTR)width * sizeof(float)))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	cObj->ResourceBarrier(mOutputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	mPSOCom[1] = BasicPolygon::CreatePsoCompute(cs[1], mRootSignatureCom.Get());
	if (mPSOCom[1] == nullptr)return false;
	mPSOCom[2] = BasicPolygon::CreatePsoCompute(cs[2], mRootSignatureCom.Get());
	if (mPSOCom[2] == nullptr)return false;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->getDevice()->CreateUnorderedAccessView(mInputBuffer.Get(), nullptr, &uavDesc, descHandleCPU);
	descHandleCPU.ptr += device->getCbvSrvUavDescriptorSize();
	device->getDevice()->CreateUnorderedAccessView(mOutputBuffer.Get(), nullptr, &uavDesc, descHandleCPU);
	descHandleCPU.ptr += device->getCbvSrvUavDescriptorSize();
	device->getDevice()->CreateUnorderedAccessView(mPrevInputBuffer.Get(), nullptr, &uavDesc, descHandleCPU);

	mInputHandleGPU.ptr += device->getCbvSrvUavDescriptorSize() * 1;
	mOutputHandleGPU.ptr += device->getCbvSrvUavDescriptorSize() * 2;
	mPrevInputHandleGPU.ptr += device->getCbvSrvUavDescriptorSize() * 3;

	return true;
}

bool Wave::ComCreate(int comIndex) {

	div = width / 16;//16はCS内スレッド数

	Dx_Device* device = Dx_Device::GetInstance();

	if (FAILED(device->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(&mOutputBuffer, (UINT64)width, (UINT)width,
		D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT_R32_FLOAT))) {
		Dx_Util::ErrorMessage("Wave::ComCreate Error!!"); return false;
	}
	mOutputBuffer.Get()->SetName(Dx_Util::charToLPCWSTR("mOutputBuffer", BasicPolygon::objName));
	mDescHeapCom = device->CreateDescHeap(4);

	D3D12_ROOT_PARAMETER rootParameter[4] = {};
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[0].Descriptor.ShaderRegister = 0;
	rootParameter[0].Descriptor.RegisterSpace = 0;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE u[3] = {};
	for (int i = 0; i < 3; i++) {
		u[i].BaseShaderRegister = i;
		u[i].NumDescriptors = 1;
		u[i].RegisterSpace = 0;
		u[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		u[i].OffsetInDescriptorsFromTableStart = 0;
		rootParameter[i + 1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameter[i + 1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameter[i + 1].DescriptorTable.pDescriptorRanges = &u[i];
		rootParameter[i + 1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	mRootSignatureCom = BasicPolygon::CreateRsCompute(4, rootParameter);
	if (mRootSignatureCom == nullptr)return false;

	descHandleCPU = mDescHeapCom->GetCPUDescriptorHandleForHeapStart();
	mSinInputHandleGPU = mDescHeapCom->GetGPUDescriptorHandleForHeapStart();
	mInputHandleGPU = mSinInputHandleGPU;
	mOutputHandleGPU = mSinInputHandleGPU;
	mPrevInputHandleGPU = mSinInputHandleGPU;
	if (!ComCreateSin(comIndex))return false;
	if (!ComCreateRipples(comIndex))return false;

	return true;
}

void Wave::SetCol(CoordTf::VECTOR3 dif, CoordTf::VECTOR3 spe, CoordTf::VECTOR3 am) {
	sg.vDiffuse.x = dif.x;
	sg.vDiffuse.y = dif.y;
	sg.vDiffuse.z = dif.z;

	sg.vSpeculer.x = spe.x;
	sg.vSpeculer.y = spe.y;
	sg.vSpeculer.z = spe.z;

	sg.vAmbient.x = am.x;
	sg.vAmbient.y = am.y;
	sg.vAmbient.z = am.z;
}

bool Wave::setDescHeap(const int numSrvTex, const int numSrvTex2, const int numCbv) {

	Dx_Device* device = Dx_Device::GetInstance();

	int numUav = 0;
	if (hs)numUav = 1;
	dpara.numDesc = numSrvTex + numSrvTex2 + (numCbv + dpara.NumMaxInstance) + numUav;
	int numHeap = dpara.NumMaterial * dpara.numDesc;
	dpara.descHeap = device->CreateDescHeap(numHeap);

	if (dpara.descHeap == nullptr)return false;
	UINT cbSize[1] = {};
	cbSize[0] = mObjectCB->getSizeInBytes();
	D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor(dpara.descHeap->GetCPUDescriptorHandleForHeapStart());

	//ConstantBuffer<WVPCB> wvp[] : register(b0, space1)
	for (UINT i = 0; i < dpara.NumMaxInstance; i++) {
		D3D12_GPU_VIRTUAL_ADDRESS ad2[1] = { wvp->getGPUVirtualAddress(i) };
		UINT size2[1] = { wvp->getSizeInBytes() };
		device->CreateCbv(hDescriptor, ad2, size2, 1);
	}

	for (int i = 0; i < dpara.NumMaterial; i++) {
		Dx_Device* d = device;
		d->CreateSrvTexture(hDescriptor, &texture[numSrvTex * i], numSrvTex);
		d->CreateSrvTexture(hDescriptor, mOutputBuffer.GetAddressOf(), numSrvTex2);
		D3D12_GPU_VIRTUAL_ADDRESS ad[1] = {};
		ad[0] = mObjectCB->Resource()->GetGPUVirtualAddress();
		d->CreateCbv(hDescriptor, ad, cbSize, numCbv);
	}
	return true;
}

bool Wave::DrawCreate(int comIndex, int texNo, int nortNo, bool blend, bool alpha, bool smooth, float divideBufferMagnification) {

	Dx_TextureHolder* dx = Dx_TextureHolder::GetInstance();
	BasicPolygon::dpara.material[0].diftex_no = texNo;
	BasicPolygon::dpara.material[0].nortex_no = nortNo;
	BasicPolygon::dpara.material[0].spetex_no = dx->GetTexNumber("dummyDifSpe.");
	BasicPolygon::mObjectCB2->CopyData(0, sg);
	const int numSrvTex = 3;
	const int numSrvTex2 = 1;
	const int numCbv = 1;
	BasicPolygon::setDivideArr(divArr, numDiv);

	UINT* indexCntArr = new UINT[BasicPolygon::dpara.NumMaterial];
	for (int m = 0; m < BasicPolygon::dpara.NumMaterial; m++) {
		indexCntArr[m] = BasicPolygon::dpara.Iview[m].IndexCount;
	}
	Dx_Util::createTangent(BasicPolygon::dpara.NumMaterial, indexCntArr,
		ver, &index, sizeof(VertexM), 0, 3 * 4, 12 * 4, 6 * 4);
	ARR_DELETE(indexCntArr);

	BasicPolygon::createDefaultBuffer(comIndex, ver, &index);
	ARR_DELETE(ver);
	ARR_DELETE(index);
	int numUav = 0;

	if (!BasicPolygon::createTexResource(comIndex))return false;
	if (!BasicPolygon::createPSO(pVertexLayout_MESH, numSrvTex + numSrvTex2, numCbv, numUav, blend, alpha))return false;

	if (Dx_Device::GetInstance()->getDxrCreateResourceState()) {
		BasicPolygon::setParameterDXR(alpha);
		if (!BasicPolygon::createStreamOutputResource(comIndex, divideBufferMagnification))return false;
		BasicPolygon::setColorDXR(0, sg);
		if (!BasicPolygon::createPSO_DXR(pVertexLayout_MESH, numSrvTex + numSrvTex2, numCbv, numUav, smooth))return false;
		setTextureDXR();
	}

	if (!setDescHeap(numSrvTex, numSrvTex2, numCbv))return false;
	return true;
}

void Wave::setMaterialType(MaterialType type) {
	BasicPolygon::dxrPara.mType[0] = type;
}

void Wave::setPointLight(int InstanceIndex, bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	Dx_Device* dev = Dx_Device::GetInstance();
	dxrPara.setPointLight(dev->dxrBuffSwapIndex(), 0, 0, InstanceIndex, on_off, range, atten);
}

void Wave::setPointLightAll(bool on_off,
	float range, CoordTf::VECTOR3 atten) {

	Dx_Device* dev = Dx_Device::GetInstance();
	dxrPara.setPointLightAll(dev->dxrBuffSwapIndex(), on_off, range, atten);
}

bool Wave::Create(int comIndex, int texNo, bool blend, bool alpha, float waveHeight, int divide, bool smooth, float TimeStep) {
	return Create(comIndex, texNo, -1, blend, alpha, waveHeight, divide, smooth, TimeStep);
}

bool Wave::Create(int comIndex, int texNo, int nortNo, bool blend, bool alpha, float waveHeight, int divide, bool smooth, float timeStep,
	float divideBufferMagnification) {

	TimeStep = timeStep;
	width = div = divide;
	cbw[0].wHei_mk012.as(waveHeight, 0.0f, 0.0f, 0.0f);
	cbw[1].wHei_mk012.as(waveHeight, 0.0f, 0.0f, 0.0f);
	GetShaderByteCode(smooth);
	if (!ComCreate(comIndex))return false;
	return DrawCreate(comIndex, texNo, nortNo, blend, alpha, smooth, divideBufferMagnification);
}

void Wave::InstancingUpdate(int waveNo, float speed, float disp, float SmoothRange,
	int DisturbX, int DisturbY,
	float gDisturbMag, float disturbStep, float updateStep, float damping,
	float SmoothRatio, float shininess) {

	static float SpatialStep = 0.25f;
	float d = damping * TimeStep + 2.0f;
	float e = (speed * speed) * (TimeStep * TimeStep) / (SpatialStep * SpatialStep);
	mk[0] = (damping * TimeStep - 2.0f) / d;
	mk[1] = (4.0f - 8.0f * e) / d;
	mk[2] = (2.0f * e) / d;

	Dx_Device* dev = Dx_Device::GetInstance();
	Step& st = step[dev->cBuffSwapUpdateIndex()];
	st.disturbStep = disturbStep;
	st.updateStep = updateStep;
	st.waveNo = waveNo;
	CONSTANT_BUFFER_WAVE& cb = cbw[dev->cBuffSwapUpdateIndex()];
	cb.speed = speed;
	cb.wHei_mk012.y = mk[0];
	cb.wHei_mk012.z = mk[1];
	cb.wHei_mk012.w = mk[2];
	cb.gDisturbMag = gDisturbMag;
	cb.gDisturbIndex.as((float)DisturbX, (float)DisturbY);

	BasicPolygon::InstancingUpdate(disp, SmoothRange, SmoothRatio, shininess);
}

void Wave::Update(int waveNo, float speed,
	CoordTf::VECTOR3 pos, CoordTf::VECTOR3 angle, CoordTf::VECTOR3 size, CoordTf::VECTOR4 Color,
	float disp, float SmoothRange,
	int DisturbX, int DisturbY,
	float gDisturbMag, float disturbStep, float updateStep, float damping,
	float SmoothRatio, float shininess,
	float px, float py, float mx, float my) {

	Instancing(pos, angle, size, Color, px, py, mx, my);
	InstancingUpdate(waveNo, speed, disp, SmoothRange,
		DisturbX, DisturbY,
		gDisturbMag, disturbStep, updateStep, damping,
		SmoothRatio, shininess);
}

void Wave::Compute(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();
	mObjectCB_WAVE->CopyData(0, cbw[dev->cBuffSwapDrawOrStreamoutputIndex()]);
	int waveNo = step[dev->cBuffSwapDrawOrStreamoutputIndex()].waveNo;

	ID3D12GraphicsCommandList* mCList = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex)->getCommandList();

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescHeapCom.Get() };
	mCList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCList->SetComputeRootSignature(mRootSignatureCom.Get());
	mCList->SetComputeRootConstantBufferView(0, mObjectCB_WAVE->Resource()->GetGPUVirtualAddress());
	D3D12_RESOURCE_BARRIER ba = {};
	ba.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

	switch (waveNo) {
	case 0:
		mCList->SetPipelineState(mPSOCom[0].Get());
		mCList->SetComputeRootDescriptorTable(1, mSinInputHandleGPU);
		mCList->SetComputeRootDescriptorTable(2, mOutputHandleGPU);
		mCList->Dispatch(div, div, 1);

		ba.UAV.pResource = mOutputBuffer.Get();
		mCList->ResourceBarrier(1, &ba);
		break;

	case 1:
		time0 += step[dev->cBuffSwapDrawOrStreamoutputIndex()].disturbStep;
		if (time0 > TimeStep) {
			mCList->SetPipelineState(mPSOCom[1].Get());
			mCList->SetComputeRootDescriptorTable(2, mInputHandleGPU);

			mCList->Dispatch(1, 1, 1);

			ba.UAV.pResource = mInputBuffer.Get();
			mCList->ResourceBarrier(1, &ba);
			time0 = 0.0f;
		}

		time1 += step[dev->cBuffSwapDrawOrStreamoutputIndex()].updateStep;
		if (time1 > TimeStep) {
			mCList->SetPipelineState(mPSOCom[2].Get());
			mCList->SetComputeRootDescriptorTable(1, mInputHandleGPU);
			mCList->SetComputeRootDescriptorTable(2, mOutputHandleGPU);
			mCList->SetComputeRootDescriptorTable(3, mPrevInputHandleGPU);

			mCList->Dispatch(div, div, 1);

			ba.UAV.pResource = mOutputBuffer.Get();
			mCList->ResourceBarrier(1, &ba);

			auto resTemp = mPrevInputHandleGPU;
			mPrevInputHandleGPU = mInputHandleGPU;
			mInputHandleGPU = mOutputHandleGPU;
			mOutputHandleGPU = resTemp;
			time1 = 0.0f;
		}
		break;
	}
}

void Wave::Draw(int comIndex) {

	Dx_Device* dev = Dx_Device::GetInstance();
	if (!BasicPolygon::firstCbSet[dev->cBuffSwapDrawOrStreamoutputIndex()] || !BasicPolygon::DrawOn)return;
	Compute(comIndex);
	BasicPolygon::Draw(comIndex);
}

void Wave::StreamOutput(int comIndex) {
	Compute(comIndex);
	BasicPolygon::StreamOutput(comIndex);
}

void Wave::CopyResource(int comIndex, ID3D12Resource* texture, D3D12_RESOURCE_STATES res, int index) {
	BasicPolygon::CopyResource(comIndex, texture, res, index);
}

void Wave::TextureInit(int width, int height, int index) {
	BasicPolygon::TextureInit(width, height, index);
}

HRESULT Wave::SetTextureMPixel(int comIndex, BYTE* frame, int index) {
	return BasicPolygon::SetTextureMPixel(comIndex, frame, index);
}