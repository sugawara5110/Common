//*****************************************************************************************//
//**                                                                                     **//
//**                   �@�@�@          DXR_Basic.cpp                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DXR_Basic.h"
#include <fstream>
#include <sstream>
#include "./ShaderDXR/ShaderParametersDXR.h"
#include "./ShaderDXR/ShaderCommonDXR.h"
#include "./ShaderDXR/ShaderBasicDXR.h"

static dxc::DxcDllSupport gDxcDllHelper;

void DXR_Basic::initDXR(UINT numparameter, ParameterDXR** pd, MaterialType* type, UINT MaxRecursion) {

	PD = pd;
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource) {
		maxRecursion = MaxRecursion;
		numParameter = numparameter;
		UINT cnt = 0;
		for (UINT i = 0; i < numParameter; i++) {
			cnt += PD[i]->NumMaterial;
		}
		numMaterial = cnt;

		material = new ConstantBuffer<DxrMaterialCB>(numMaterial);
		cbObj[0].matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
		cbObj[1].matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
		sCB = new ConstantBuffer<DxrConstantBuffer>(1);

		createTriangleVB(numMaterial);
		createAccelerationStructures();

		for (UINT i = 0; i < numMaterial; i++) {

			switch (type[i]) {
			case METALLIC:
				cbObj[0].matCb[i].materialNo = 0;
				cbObj[1].matCb[i].materialNo = 0;
				break;

			case NONREFLECTION:
				cbObj[0].matCb[i].materialNo = 1;
				cbObj[1].matCb[i].materialNo = 1;
				break;

			case EMISSIVE:
				cbObj[0].matCb[i].materialNo = 2;
				cbObj[1].matCb[i].materialNo = 2;
				break;

			case DIRECTIONLIGHT_METALLIC:
				cbObj[0].matCb[i].materialNo = 3;
				cbObj[1].matCb[i].materialNo = 3;
				break;

			case DIRECTIONLIGHT_NONREFLECTION:
				cbObj[0].matCb[i].materialNo = 4;
				cbObj[1].matCb[i].materialNo = 4;
				break;
			}
		}

		createRtPipelineState();
		createShaderResources();
		createShaderTable();
	}
}

void DXR_Basic::createTriangleVB(UINT numMaterial) {
	Dx12Process* dx = Dx12Process::GetInstance();
	for (UINT i = 0; i < numParameter; i++) {
		for (UINT j = 0; j < 2; j++)
			PD[i]->updateDXR[j].InstanceID = std::make_unique<UINT[]>(INSTANCE_PCS_3D * PD[i]->NumMaterial);
	}
	VertexBufferGPU = std::make_unique<ComPtr<ID3D12Resource>[]>(numMaterial);
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			VertexView& vv = PD[i]->updateDXR[dx->dxrBuffSwap[1]].VviewDXR[j];
			dx->createDefaultResourceBuffer(VertexBufferGPU[MaterialCnt].GetAddressOf(),
				vv.VertexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ);
			MaterialCnt++;
		}
	}
}

void DXR_Basic::createBottomLevelAS1(Dx12Process_sub* com, VertexView* vv,
	IndexView* iv, UINT currentIndexCount, UINT MaterialNo, bool update) {

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	if (!update)buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	Dx12Process* dx = Dx12Process::GetInstance();
	AccelerationStructureBuffers& bLB = asObj[buffSwap[0]].bottomLevelBuffers[MaterialNo];

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexBuffer.StartAddress = vv->VertexBufferGPU->GetGPUVirtualAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = vv->VertexByteStride;
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	UINT numVertices = vv->VertexBufferByteSize / vv->VertexByteStride;
	geomDesc.Triangles.VertexCount = numVertices;
	geomDesc.Triangles.IndexBuffer = iv->IndexBufferGPU->GetGPUVirtualAddress();
	geomDesc.Triangles.IndexCount = iv->IndexCount;
	if (bLB.firstSet)geomDesc.Triangles.IndexCount = currentIndexCount;
	geomDesc.Triangles.IndexFormat = iv->IndexFormat;
	geomDesc.Triangles.Transform3x4 = 0;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = buildFlag;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	//AccelerationStructure���\�z����ׂ̃��\�[�X�v�����擾(info�ɓ���)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (bLB.firstSet) {
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = bLB.pResult.Get();
		com->mCommandList->ResourceBarrier(1, &uavBarrier);
	}
	else {
		//���\�[�X�v��������UAV�쐬
		dx->createDefaultResourceBuffer_UNORDERED_ACCESS(bLB.pScratch.GetAddressOf(), info.ScratchDataSizeInBytes,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		dx->createDefaultResourceBuffer_UNORDERED_ACCESS(bLB.pResult.GetAddressOf(), info.ResultDataMaxSizeInBytes,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = bLB.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = bLB.pScratch->GetGPUVirtualAddress();

	if (bLB.firstSet) {
		asDesc.Inputs.Flags = buildFlag;
		asDesc.SourceAccelerationStructureData = 0; //ALLOW_UPDATE�̏ꍇ0�ɂ���
	}
	else {
		bLB.firstSet = true;
	}

	//bottom-level AS�쐬, ��O�����͍쐬��̏��, �s�v�̏ꍇnullptr
	com->mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	//�o���A�������\�[�X�ւ�UAV�A�N�Z�X�ɉ����Ď���UAV�A�N�Z�X�J�n�O�Ɍ��݂�UAV�A�N�Z�X���I������K�v�����鎖������
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = bLB.pResult.Get();

	com->mCommandList->ResourceBarrier(1, &uavBarrier);
}

void DXR_Basic::createBottomLevelAS(Dx12Process_sub* com) {
	Dx12Process* dx = Dx12Process::GetInstance();
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		bool createAS = ud.createAS;
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			VertexView* vv = &ud.VviewDXR[j];
			IndexView* iv = &PD[i]->IviewDXR[j];
			UINT indexCnt = ud.currentIndexCount[j];
			if (ud.firstSet && (PD[i]->updateF || !ud.createAS)) {
				createBottomLevelAS1(com, vv, iv, indexCnt, MaterialCnt, PD[i]->updateF);
				createAS = true;
			}
			MaterialCnt++;
		}
		ud.createAS = createAS;
	}
}

void DXR_Basic::createTopLevelAS(Dx12Process_sub* com) {

	Dx12Process* dx = Dx12Process::GetInstance();
	AccelerationStructureBuffers& tLB = asObj[buffSwap[0]].topLevelBuffers;

	UINT numRayInstance = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		if (tLB.firstSet)
			numRayInstance += ud.NumInstance * PD[i]->NumMaterial;
		else
			numRayInstance += INSTANCE_PCS_3D * PD[i]->NumMaterial;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	inputs.NumDescs = numRayInstance;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	//AccelerationStructure���\�z����ׂ̃��\�[�X�v�����擾(info�ɓ���)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (tLB.firstSet) {
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = tLB.pResult.Get();
		com->mCommandList->ResourceBarrier(1, &uavBarrier);
	}
	else {
		//���\�[�X�v��������UAV�쐬
		dx->createDefaultResourceBuffer_UNORDERED_ACCESS(tLB.pScratch.GetAddressOf(),
			info.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		dx->createDefaultResourceBuffer_UNORDERED_ACCESS(tLB.pResult.GetAddressOf(),
			info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
		dx->createUploadResource(tLB.pInstanceDesc.GetAddressOf(),
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numRayInstance);
	}

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	tLB.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
	ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numRayInstance);

	UINT RayInstanceCnt = 0;
	UINT materialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < ud.NumInstance; k++) {
				//InstanceDesc������
				if (ud.firstSet) {
					D3D12_RAYTRACING_INSTANCE_DESC& pID = pInstanceDesc[RayInstanceCnt];
					UINT materialInstanceID = materialCnt;//max65535
					UINT InstancingID = i * INSTANCE_PCS_3D + k;//max65535
					UINT InstanceID = ((materialInstanceID << 16) & 0xffff0000) | (InstancingID & 0x0000ffff);
					ud.InstanceID[INSTANCE_PCS_3D * j + k] = InstanceID;
					pID.InstanceID = InstanceID;//���̒l�́AInstanceID()����ăV�F�[�_�[�Ɍ��J����܂�
					pID.InstanceContributionToHitGroupIndex = 0;//�V�F�[�_�[�e�[�u�����̃I�t�Z�b�g�B�W�I���g������̏ꍇ,�I�t�Z�b�g0
					pID.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
					memcpy(pID.Transform, &ud.Transform[k], sizeof(pID.Transform));
					pID.AccelerationStructure = asObj[buffSwap[0]].bottomLevelBuffers[materialCnt].pResult.Get()->GetGPUVirtualAddress();
					pID.InstanceMask = ud.InstanceMask;
				}
				RayInstanceCnt++;
			}
			materialCnt++;
		}
	}

	tLB.pInstanceDesc->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = tLB.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = tLB.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = tLB.pScratch->GetGPUVirtualAddress();

	if (tLB.firstSet) {
		asDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		asDesc.SourceAccelerationStructureData = 0; //ALLOW_UPDATE�̏ꍇ0�ɂ���
	}
	else {
		tLB.firstSet = true;
		asObj[buffSwap[0]].mpTopLevelAS = tLB.pResult;
	}

	com->mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = tLB.pResult.Get();
	com->mCommandList->ResourceBarrier(1, &uavBarrier);
}

void DXR_Basic::createAccelerationStructures() {
	asObj[0].bottomLevelBuffers = std::make_unique<AccelerationStructureBuffers[]>(numMaterial);
	asObj[1].bottomLevelBuffers = std::make_unique<AccelerationStructureBuffers[]>(numMaterial);
}

static const WCHAR* kRayGenShader = L"rayGen";
static const WCHAR* kBasicMissShader = L"basicMiss";
static const WCHAR* kBasicClosestHitShader = L"basicHit";
static const WCHAR* kBasicHitGroup = L"basicHitGroup";
static const WCHAR* kEmissiveMiss = L"emissiveMiss";
static const WCHAR* kEmissiveHitShader = L"emissiveHit";
static const WCHAR* kEmissiveHitGroup = L"emissiveHitGroup";

static ComPtr<ID3DBlob> CompileLibrary(char* shaderByte, const WCHAR* filename, const WCHAR* targetString) {

	//�R���p�C�����C�u����������
	HRESULT hr_Hel = gDxcDllHelper.Initialize();
	if (FAILED(hr_Hel)) {
		ErrorMessage("DXR CompileLibrary gDxcDllHelper.Initialize() Error");
	}
	ComPtr<IDxcCompiler> pCompiler;
	ComPtr<IDxcLibrary> pLibrary;
	gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, pCompiler.GetAddressOf());
	gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, pLibrary.GetAddressOf());

	uint32_t size = (uint32_t)strlen(shaderByte) + 1;
	//string �� IDxcBlob�ϊ�
	ComPtr<IDxcBlobEncoding> pTextBlob;
	HRESULT hr_Li = pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)shaderByte,
		size, 0, pTextBlob.GetAddressOf());
	if (FAILED(hr_Li)) {
		ErrorMessage("DXR CompileLibrary CreateBlobWithEncodingFromPinned Error");
	}
	//Compile
	ComPtr<IDxcOperationResult> pResult;
	HRESULT hr_Com = pCompiler->Compile(pTextBlob.Get(), filename, L"", targetString,
		nullptr, 0, nullptr, 0, nullptr, pResult.GetAddressOf());
	if (FAILED(hr_Com)) {
		ErrorMessage("DXR CompileLibrary Compile Error");
	}

	//Compile����
	HRESULT resultCode;
	pResult.Get()->GetStatus(&resultCode);
	if (FAILED(resultCode))
	{
		ComPtr<IDxcBlobEncoding> pError;
		pResult.Get()->GetErrorBuffer(pError.GetAddressOf());
		ErrorMessage("DXR compileLibrary Error");
		ErrorMessage((char*)pError.Get()->GetBufferPointer());
		return nullptr;
	}

	ComPtr<IDxcBlob> pBlob;
	pResult->GetResult(pBlob.GetAddressOf());
	ComPtr<ID3DBlob> retBlob;
	pBlob.Get()->QueryInterface(IID_PPV_ARGS(retBlob.GetAddressOf()));
	return retBlob;
}

struct DxilLibrary {

	DxilLibrary(ComPtr<ID3DBlob> pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
	{
		stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stateSubobject.pDesc = &dxilLibDesc;

		dxilLibDesc = {};
		exportDesc.resize(entryPointCount);
		exportName.resize(entryPointCount);
		if (pBlob)
		{
			dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
			dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
			dxilLibDesc.NumExports = entryPointCount;
			dxilLibDesc.pExports = exportDesc.data();

			for (uint32_t i = 0; i < entryPointCount; i++)
			{
				exportName[i] = entryPoint[i];
				exportDesc[i].Name = exportName[i].c_str();
				exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[i].ExportToRename = nullptr;
			}
		}
	};

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	D3D12_STATE_SUBOBJECT stateSubobject{};
	ComPtr<ID3DBlob> pShaderBlob;
	std::vector<D3D12_EXPORT_DESC> exportDesc;
	std::vector<std::wstring> exportName;
};

static DxilLibrary createDxilLibrary(char* add_shader) {
	addChar dxrShader[3];
	dxrShader[0].addStr(add_shader, ShaderParametersDXR);
	dxrShader[1].addStr(dxrShader[0].str, ShaderCommonDXR);
	dxrShader[2].addStr(dxrShader[1].str, ShaderBasicDXR);
	//�V�F�[�_�[�̃R���p�C��
	ComPtr<ID3DBlob> pDxilLib = CompileLibrary(dxrShader[2].str, L"ShaderBasicDXR", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader,
		kBasicMissShader, kBasicClosestHitShader,
		kEmissiveMiss, kEmissiveHitShader
	};
	return DxilLibrary(pDxilLib, entryPoints, ARRAY_SIZE(entryPoints));
}

struct HitProgram {

	HitProgram(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name) : exportName(name)
	{
		desc = {};
		desc.AnyHitShaderImport = ahsExport;
		desc.ClosestHitShaderImport = chsExport;
		desc.HitGroupExport = exportName.c_str();

		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObject.pDesc = &desc;
	}

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc;
	D3D12_STATE_SUBOBJECT subObject;
};

struct ExportAssociation {

	ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
	{
		association.NumExports = exportCount;
		association.pExports = exportNames;
		association.pSubobjectToAssociate = pSubobjectToAssociate;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobject.pDesc = &association;
	}

	D3D12_STATE_SUBOBJECT subobject = {};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
};

ComPtr<ID3D12RootSignature> DXR_Basic::createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc) {
	return Dx12Process::GetInstance()->CreateRsCommon(&desc);
}

struct LocalRootSignature {

	LocalRootSignature(ComPtr<ID3D12RootSignature> rootSig)
	{
		pRootSig = rootSig;
		pInterface = pRootSig.Get();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct GlobalRootSignature {

	GlobalRootSignature(ComPtr<ID3D12RootSignature> rootSig)
	{
		pRootSig = rootSig;
		pInterface = pRootSig.Get();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct ShaderConfig {

	ShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
	{
		shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
		shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		subobject.pDesc = &shaderConfig;
	}

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct PipelineConfig {

	PipelineConfig(uint32_t maxTraceRecursionDepth)
	{
		config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		subobject.pDesc = &config;
	}

	D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct RootSignatureDesc {

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};

static RootSignatureDesc createRayGenRootDesc(UINT numMaterial) {

	RootSignatureDesc desc = {};
	int numDescriptorRanges = 5;
	desc.range.resize(numDescriptorRanges);
	UINT descCnt = 0;
	//gOutput(u0)
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[0].OffsetInDescriptorsFromTableStart = descCnt++;

	//Indices(t0)
	desc.range[1].BaseShaderRegister = 0;
	desc.range[1].NumDescriptors = numMaterial;
	desc.range[1].RegisterSpace = 1;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//Vertex(t1)
	desc.range[2].BaseShaderRegister = 1;
	desc.range[2].NumDescriptors = numMaterial;
	desc.range[2].RegisterSpace = 2;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//cbuffer global(b0)
	desc.range[3].BaseShaderRegister = 0;
	desc.range[3].NumDescriptors = 1;
	desc.range[3].RegisterSpace = 0;
	desc.range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[3].OffsetInDescriptorsFromTableStart = descCnt++;

	//material(b1)
	desc.range[4].BaseShaderRegister = 1;
	desc.range[4].NumDescriptors = numMaterial;
	desc.range[4].RegisterSpace = 3;
	desc.range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[4].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();
	desc.rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

static RootSignatureDesc createGlobalRootDesc(UINT numMaterial) {

	RootSignatureDesc desc = {};
	int numDescriptorRanges = 4;
	desc.range.resize(numDescriptorRanges);
	UINT descCnt = 0;
	//g_texDiffuse(t0)
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = numMaterial;
	desc.range[0].RegisterSpace = 10;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//g_texNormal(t1)
	desc.range[1].BaseShaderRegister = 1;
	desc.range[1].NumDescriptors = numMaterial;
	desc.range[1].RegisterSpace = 11;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//g_texSpecular(t2)
	desc.range[2].BaseShaderRegister = 2;
	desc.range[2].NumDescriptors = numMaterial;
	desc.range[2].RegisterSpace = 12;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//g_samLinear(s0)
	desc.range[3].BaseShaderRegister = 0;
	desc.range[3].NumDescriptors = 1;
	desc.range[3].RegisterSpace = 13;
	desc.range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	desc.range[3].OffsetInDescriptorsFromTableStart = 0;

	desc.rootParams.resize(3);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 3;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();
	desc.rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[1].DescriptorTable.pDescriptorRanges = &desc.range[3];
	desc.rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//gRtScene(t3)
	desc.rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	desc.rootParams[2].Descriptor.ShaderRegister = 3;
	desc.rootParams[2].Descriptor.RegisterSpace = 14;
	desc.rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.desc.NumParameters = 3;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return desc;
}

void DXR_Basic::createRtPipelineState() {

	Dx12Process* dx = Dx12Process::GetInstance();

	//CreateStateObject�쐬�ɕK�v�ȊeD3D12_STATE_SUBOBJECT���쐬
	std::array<D3D12_STATE_SUBOBJECT, 13> subobjects;
	uint32_t index = 0;

	//DXIL library ������, SUBOBJECT�쐬
	addChar str;
	str.addStr(dx->ShaderNormalTangentCopy.get(), dx->ShaderCalculateLightingCopy.get());
	DxilLibrary dxilLib = createDxilLibrary(str.str);//Dx12Process�ɕێ����Ă�shader�����
	subobjects[index++] = dxilLib.stateSubobject;

	//BasicHitShader SUBOBJECT�쐬
	HitProgram basicHitProgram(nullptr, kBasicClosestHitShader, kBasicHitGroup);
	subobjects[index++] = basicHitProgram.subObject;

	//EmissiveHitShader SUBOBJECT�쐬
	HitProgram emissiveHitProgram(nullptr, kEmissiveHitShader, kEmissiveHitGroup);
	subobjects[index++] = emissiveHitProgram.subObject;

	//raygenerationShader���[�g�V�O�l�`���쐬, SUBOBJECT�쐬
	LocalRootSignature rgsRootSignature(createRootSignature(createRayGenRootDesc(numMaterial).desc));
	subobjects[index] = rgsRootSignature.subobject;

	//raygenerationShader�Ɓ��̃��[�g�V�O�l�`���֘A�t��, SUBOBJECT�쐬
	uint32_t rgsRootIndex = index++;
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject;

	//basicHitShader, basicMissShader ���[�g�V�O�l�`���쐬, SUBOBJECT�쐬
	LocalRootSignature basicHitMissRootSignature(createRootSignature(createRayGenRootDesc(numMaterial).desc));
	subobjects[index] = basicHitMissRootSignature.subobject;

	//����basicHitShader, basicMissShader ���L���[�g�V�O�l�`����basicHitShader, basicMissShader�֘A�t��, SUBOBJECT�쐬
	uint32_t basicHitMissRootIndex = index++;
	const WCHAR* basicMissHitExportName[] = { kBasicMissShader, kBasicClosestHitShader };
	ExportAssociation basicMissHitRootAssociation(basicMissHitExportName, ARRAY_SIZE(basicMissHitExportName),
		&(subobjects[basicHitMissRootIndex]));
	subobjects[index++] = basicMissHitRootAssociation.subobject;

	//kEmissiveHit, kEmissiveMiss ���[�g�V�O�l�`���쐬, SUBOBJECT�쐬
	LocalRootSignature emissiveRootSignature(createRootSignature(createRayGenRootDesc(numMaterial).desc));
	subobjects[index] = emissiveRootSignature.subobject;

	uint32_t emissiveRootIndex = index++;
	const WCHAR* emissiveRootExport[] = { kEmissiveHitShader, kEmissiveMiss };
	ExportAssociation emissiveRootAssociation(emissiveRootExport, ARRAY_SIZE(emissiveRootExport),
		&(subobjects[emissiveRootIndex]));
	subobjects[index++] = emissiveRootAssociation.subobject;

	//�y�C���[�h�T�C�Y���v���O�����Ƀo�C���h���� SUBOBJECT�쐬
	uint32_t MaxAttributeSizeInBytes = sizeof(float) * 2;
	uint32_t maxPayloadSizeInBytes = sizeof(float) * 12;
	ShaderConfig shaderConfig(MaxAttributeSizeInBytes, maxPayloadSizeInBytes);
	subobjects[index] = shaderConfig.subobject;

	//ShaderConfig�ƑS�ẴV�F�[�_�[�֘A�t�� SUBOBJECT�쐬
	uint32_t shaderConfigIndex = index++;
	const WCHAR* shaderExports[] = {
		kRayGenShader,
		kBasicClosestHitShader,kBasicMissShader,
		kEmissiveHitShader, kEmissiveMiss
	};
	ExportAssociation configAssociation(shaderExports, ARRAY_SIZE(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject;

	//�p�C�v���C���R���t�B�O�쐬 SUBOBJECT�쐬
	PipelineConfig config(maxRecursion);
	subobjects[index++] = config.subobject;

	//�O���[�o�����[�g�V�O�l�`���쐬 SUBOBJECT�쐬
	GlobalRootSignature root(createRootSignature(createGlobalRootDesc(numMaterial).desc));
	mpGlobalRootSig = root.pRootSig;
	subobjects[index++] = root.subobject;

	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index;
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	HRESULT hr = dx->md3dDevice.Get()->CreateStateObject(&desc, IID_PPV_ARGS(mpPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		ErrorMessage("DXR_Basic::createRtPipelineState() Error");
	}
}

void DXR_Basic::createShaderResources() {

	Dx12Process* dx = Dx12Process::GetInstance();
	dx->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mpOutputResource.GetAddressOf(),
		dx->mClientWidth,
		dx->mClientHeight,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//Local
	int num_u0 = 1;
	int num_t0 = numMaterial;
	int num_t1 = numMaterial;
	int num_b0 = 1;
	int num_b1 = numMaterial;
	numSkipLocalHeap = num_u0 + num_t0 + num_t1 + num_b0 + num_b1;
	//Global
	int num_t00 = numMaterial;
	int num_t01 = numMaterial;
	int num_t02 = numMaterial;
	int numHeap = numSkipLocalHeap + num_t00 + num_t01 + num_t02;

	//Local
	mpSrvUavCbvHeap = dx->CreateDescHeap(numHeap);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavCbvHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	UINT offsetSize = dx->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//UAV���쐬 gOutput(u0)
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	dx->getDevice()->CreateUnorderedAccessView(mpOutputResource.Get(), nullptr, &uavDesc, srvHandle);

	//SRV���쐬 Indices(t0)
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			IndexView& iv = PD[i]->IviewDXR[j];
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1 = {};
			srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc1.Buffer.NumElements = iv.IndexCount;
			srvDesc1.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc1.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc1.Buffer.StructureByteStride = sizeof(uint32_t);
			srvHandle.ptr += offsetSize;
			dx->getDevice()->CreateShaderResourceView(iv.IndexBufferGPU.Get(), &srvDesc1, srvHandle);
		}
	}

	//SRV���쐬 Vertex(t1)
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			VertexView& vv = PD[i]->updateDXR[dx->dxrBuffSwap[1]].VviewDXR[j];
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
			srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc2.Buffer.NumElements = vv.VertexBufferByteSize / vv.VertexByteStride;
			srvDesc2.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc2.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc2.Buffer.StructureByteStride = vv.VertexByteStride;
			srvHandle.ptr += offsetSize;
			dx->getDevice()->CreateShaderResourceView(VertexBufferGPU[MaterialCnt].Get(), &srvDesc2, srvHandle);
			MaterialCnt++;
		}
	}

	//CBV���쐬 cbuffer global(b0)
	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	bufferDesc.SizeInBytes = sCB->getSizeInBytes();
	bufferDesc.BufferLocation = sCB->Resource()->GetGPUVirtualAddress();
	srvHandle.ptr += offsetSize;
	dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);

	//CBV���쐬 material(b1)
	for (UINT i = 0; i < numMaterial; i++) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
		bufferDesc.SizeInBytes = material->getElementByteSize();
		bufferDesc.BufferLocation = material->getGPUVirtualAddress(i);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);
	}

	//Global
	//SRV���쐬 g_texDiffuse(t0), g_texNormal(t1), g_texSpecular(t2)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3 = {};
	for (UINT t = 0; t < 3; t++) {
		UINT InstanceCnt = 0;
		for (UINT i = 0; i < numParameter; i++) {
			for (int j = 0; j < PD[i]->NumMaterial; j++) {
				ID3D12Resource* res = nullptr;
				switch (t) {
				case 0:
					res = PD[i]->difTex[j];
					break;
				case 1:
					res = PD[i]->norTex[j];
					break;
				case 2:
					res = PD[i]->speTex[j];
					break;
				}
				srvDesc3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc3.Format = res->GetDesc().Format;
				srvDesc3.Texture2D.MostDetailedMip = 0;
				srvDesc3.Texture2D.ResourceMinLODClamp = 0.0f;
				srvDesc3.Texture2D.MipLevels = res->GetDesc().MipLevels;
				srvHandle.ptr += offsetSize;
				dx->getDevice()->CreateShaderResourceView(res, &srvDesc3, srvHandle);
			}
		}
	}

	//g_samLinear(s0)
	D3D12_SAMPLER_DESC samLinear = {};
	samLinear.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samLinear.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samLinear.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samLinear.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samLinear.MipLODBias = 0;
	samLinear.MaxAnisotropy = 16;
	samLinear.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samLinear.MinLOD = 0.0f;
	samLinear.MaxLOD = D3D12_FLOAT32_MAX;

	mpSamplerHeap = dx->CreateSamplerDescHeap(samLinear);
}

void DXR_Basic::createShaderTable() {
	/*
		�V�F�[�_�[�e�[�u���̊e�G���g���͓����T�C�Y�ɂ���K�v����
		��ԃT�C�Y�̑傫���G���g���ɍ��킹��
		ray-gen�v���O�����ɂ͍ő�̃G���g�����K�v-�f�B�X�N���v�^�e�[�u����sizeof�i�v���O�������ʎq�j+ 8�o�C�g
		�G���g���T�C�Y��D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT�ɃA���C�����g����
	*/

	//�T�C�Y�v�Z
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 8; //ray-gen�� + 8
	mShaderTableEntrySize = ALIGNMENT_TO(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
	uint32_t shaderTableSize = mShaderTableEntrySize * 5;//raygen + miss * 2 + hit * 2

	Dx12Process::GetInstance()->createUploadResource(mpShaderTable.GetAddressOf(), shaderTableSize);

	uint8_t* pData;
	mpShaderTable.Get()->Map(0, nullptr, (void**)&pData);

	ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	mpPipelineState.Get()->QueryInterface(IID_PPV_ARGS(pRtsoProps.GetAddressOf()));

	//Entry 0 - ray-gen shader ���ʎq���擾, �L�^
	memcpy(pData, pRtsoProps.Get()->GetShaderIdentifier(kRayGenShader),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	uint64_t heapStart = mpSrvUavCbvHeap.Get()->GetGPUDescriptorHandleForHeapStart().ptr;
	*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

	//Entry 1 - basic miss program
	memcpy(pData + mShaderTableEntrySize, pRtsoProps.Get()->GetShaderIdentifier(kBasicMissShader),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - emissive miss program
	memcpy(pData + mShaderTableEntrySize * 2, pRtsoProps->GetShaderIdentifier(kEmissiveMiss),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 3 - basic hit program
	uint8_t* pHitEntry = pData + mShaderTableEntrySize * 3;
	memcpy(pHitEntry, pRtsoProps.Get()->GetShaderIdentifier(kBasicHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 4 - emissive hit program
	uint8_t* pHitEntry2 = pData + mShaderTableEntrySize * 4;
	memcpy(pHitEntry2, pRtsoProps.Get()->GetShaderIdentifier(kEmissiveHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	mpShaderTable.Get()->Unmap(0, nullptr);
}

void DXR_Basic::updateMaterial(CBobj* cbObj) {

	Dx12Process* dx = Dx12Process::GetInstance();
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];

		for (int j = 0; j < PD[i]->NumMaterial; j++) {

			VECTOR4& dif = PD[i]->diffuse[j];
			VECTOR4& spe = PD[i]->specular[j];
			VECTOR4& amb = PD[i]->ambient[j];
			DxrMaterialCB& mcb = cbObj->matCb[MaterialCnt];
			mcb.vDiffuse.as(dif.x, dif.y, dif.z, 0.0f);
			mcb.vSpeculer.as(spe.x, spe.y, spe.z, 0.0f);
			mcb.vAmbient.as(amb.x, amb.y, amb.z, 0.0f);
			mcb.shininess = ud.shininess;
			memcpy(&mcb.AddObjColor, &ud.AddObjColor, sizeof(VECTOR4));
			if (PD[i]->alphaTest) {
				mcb.alphaTest = 1.0f;
			}
			else {
				mcb.alphaTest = 0.0f;
			}
			MaterialCnt++;
		}
	}
}

void DXR_Basic::updateCB(CBobj* cbObj, UINT numRecursion) {
	Dx12Process* dx = Dx12Process::GetInstance();
	MATRIX VP;
	MatrixMultiply(&VP, &dx->mView, &dx->mProj);
	MatrixTranspose(&VP);
	DxrConstantBuffer& cb = cbObj->cb;
	MatrixInverse(&cb.projectionToWorld, &VP);
	cb.cameraUp.as(dx->upX, dx->upY, dx->upZ, 1.0f);
	cb.cameraPosition.as(dx->posX, dx->posY, dx->posZ, 1.0f);
	cb.maxRecursion = numRecursion;
	updateMaterial(cbObj);

	int cntEm = 0;
	bool breakF = false;
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < ud.NumInstance; k++) {
				if (cbObj->matCb[MaterialCnt].materialNo == 2) {
					for (UINT v = 0; v < PD[i]->numVertex; v++) {
						MATRIX Transpose;
						memcpy(&Transpose, &ud.Transform[k], sizeof(MATRIX));
						MatrixTranspose(&Transpose);
						VECTOR3 v3;
						if (PD[i]->useVertex) {
							v3.as(PD[i]->v[v].x, PD[i]->v[v].y, PD[i]->v[v].z);
							VectorMatrixMultiply(&v3, &Transpose);
						}
						else {
							v3.as(Transpose._41, Transpose._42, Transpose._43);
						}
						cb.emissivePosition[cntEm].x = v3.x;
						cb.emissivePosition[cntEm].y = v3.y;
						cb.emissivePosition[cntEm].z = v3.z;
						cb.emissivePosition[cntEm].w = dx->plight.LightPos[cntEm].w;
						cb.Lightst[cntEm].x = dx->plight.Lightst[cntEm].x;
						cb.Lightst[cntEm].y = dx->plight.Lightst[cntEm].y;
						cb.Lightst[cntEm].z = dx->plight.Lightst[cntEm].z;
						cb.Lightst[cntEm].w = dx->plight.Lightst[cntEm].w;
						cb.emissiveNo[cntEm].x = (float)ud.InstanceID[INSTANCE_PCS_3D * j + k];
						cntEm++;
						if (cntEm >= LIGHT_PCS) {
							breakF = true;
							break;
						}
					}
				}
				if (breakF)break;
			}
			MaterialCnt++;
			if (breakF)break;
		}
		if (breakF)break;
	}

	cb.numEmissive.x = (float)cntEm;
	memcpy(&cb.GlobalAmbientColor, &dx->GlobalAmbientLight, sizeof(VECTOR4));
	memcpy(&cb.dDirection, &dx->dlight.Direction, sizeof(VECTOR4));
	memcpy(&cb.dLightColor, &dx->dlight.LightColor, sizeof(VECTOR4));
	memcpy(&cb.dLightst, &dx->dlight.onoff, sizeof(VECTOR4));
}

void DXR_Basic::updateAS(Dx12Process_sub* com, UINT numRecursion) {
	createBottomLevelAS(com);
	createTopLevelAS(com);
	updateCB(&cbObj[buffSwap[0]], numRecursion);
}

void DXR_Basic::updateVertexBuffer(int comNo) {
	UINT MaterialCnt = 0;
	Dx12Process* dx = Dx12Process::GetInstance();
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			VertexView& vv = PD[i]->updateDXR[dx->dxrBuffSwap[1]].VviewDXR[j];

			dx->dx->dx_sub[comNo].CopyResourceGENERIC_READ(VertexBufferGPU[MaterialCnt].Get(),
				vv.VertexBufferGPU.Get());
			MaterialCnt++;
		}
	}
}

void DXR_Basic::setCB() {
	sCB->CopyData(0, cbObj[buffSwap[1]].cb);
	for (UINT i = 0; i < numMaterial; i++)material->CopyData(i, cbObj[buffSwap[1]].matCb[i]);
}

void DXR_Basic::raytrace(Dx12Process_sub* com) {

	if (!asObj[buffSwap[1]].topLevelBuffers.firstSet)return;

	Dx12Process* dx = Dx12Process::GetInstance();
	setCB();

	com->mCommandList->SetComputeRootSignature(mpGlobalRootSig.Get());

	ID3D12DescriptorHeap* heaps[] = { mpSrvUavCbvHeap.Get(), mpSamplerHeap.Get() };
	com->mCommandList->SetDescriptorHeaps(ARRAY_SIZE(heaps), heaps);

	UINT offsetSize = dx->mCbvSrvUavDescriptorSize;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavCbvHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	srvHandle.ptr += offsetSize * numSkipLocalHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = mpSamplerHeap.Get()->GetGPUDescriptorHandleForHeapStart();

	com->mCommandList->SetComputeRootDescriptorTable(0, srvHandle);
	com->mCommandList->SetComputeRootDescriptorTable(1, samplerHandle);
	com->mCommandList->SetComputeRootShaderResourceView(2, asObj[buffSwap[1]].mpTopLevelAS.Get()->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = dx->mClientWidth;
	raytraceDesc.Height = dx->mClientHeight;
	raytraceDesc.Depth = 1;

	//RayGen
	raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress();
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

	//Miss
	size_t missOffset = 1 * mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;//missShader 2��

	//Hit
	size_t hitOffset = 3 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * 2;//hitShader 2��

	//Dispatch
	com->mCommandList->SetPipelineState1(mpPipelineState.Get());
	com->mCommandList->DispatchRays(&raytraceDesc);
}

void DXR_Basic::copyBackBuffer(int comNo) {
	//���ʂ��o�b�N�o�b�t�@�ɃR�s�[
	Dx12Process* dx = Dx12Process::GetInstance();
	dx->dx_sub[comNo].ResourceBarrier(mpOutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	dx->dx_sub[comNo].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	dx->dx_sub[comNo].mCommandList->CopyResource(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		mpOutputResource.Get());

	dx->dx_sub[comNo].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	dx->dx_sub[comNo].ResourceBarrier(mpOutputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void DXR_Basic::update_g(int comNo, UINT numRecursion) {
	Dx12Process* dx = Dx12Process::GetInstance();
	updateAS(&dx->dx_sub[comNo], numRecursion);
}

void DXR_Basic::update_c(int comNo, UINT numRecursion) {
	Dx12Process* dx = Dx12Process::GetInstance();
	updateAS(&dx->dx_subCom[comNo], numRecursion);
}

void DXR_Basic::raytrace_g(int comNo) {
	Dx12Process* dx = Dx12Process::GetInstance();
	raytrace(&dx->dx_sub[comNo]);
}

void DXR_Basic::raytrace_c(int comNo) {
	Dx12Process* dx = Dx12Process::GetInstance();
	raytrace(&dx->dx_subCom[comNo]);
}