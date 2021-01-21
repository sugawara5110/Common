//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DXR_Basic.cpp                                     **//
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
			cnt += PD[i]->NumMaterial * PD[i]->NumMaxInstance;
		}
		numMaterial = cnt;
		maxNumInstancing = INSTANCE_PCS_3D * numParameter;

		wvp = new ConstantBuffer<WVP_CB>(maxNumInstancing);
		cbObj[0].wvpCb = std::make_unique<WVP_CB[]>(maxNumInstancing);
		cbObj[1].wvpCb = std::make_unique<WVP_CB[]>(maxNumInstancing);
		material = new ConstantBuffer<DxrMaterialCB>(numMaterial);
		cbObj[0].matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
		cbObj[1].matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
		sCB = new ConstantBuffer<DxrConstantBuffer>(1);

		createTriangleVB(numMaterial);
		createAccelerationStructures();

		int numMat = 0;
		int numIns = 0;
		for (UINT i = 0; i < numParameter; i++) {
			for (int j = 0; j < PD[i]->NumMaterial; j++) {
				for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
					switch (type[numMat]) {
					case METALLIC:
						cbObj[0].matCb[numIns].materialNo = 0;
						cbObj[1].matCb[numIns].materialNo = 0;
						break;

					case NONREFLECTION:
						cbObj[0].matCb[numIns].materialNo = 1;
						cbObj[1].matCb[numIns].materialNo = 1;
						break;

					case EMISSIVE:
						cbObj[0].matCb[numIns].materialNo = 2;
						cbObj[1].matCb[numIns].materialNo = 2;
						break;

					case DIRECTIONLIGHT_METALLIC:
						cbObj[0].matCb[numIns].materialNo = 3;
						cbObj[1].matCb[numIns].materialNo = 3;
						break;

					case DIRECTIONLIGHT_NONREFLECTION:
						cbObj[0].matCb[numIns].materialNo = 4;
						cbObj[1].matCb[numIns].materialNo = 4;
						break;

					case TRANSLUCENCE:
						cbObj[0].matCb[numIns].materialNo = 5;
						cbObj[1].matCb[numIns].materialNo = 5;
						break;
					}
					numIns++;
				}
				numMat++;
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
			PD[i]->updateDXR[j].InstanceID =
			std::make_unique<UINT[]>(INSTANCE_PCS_3D * PD[i]->NumMaterial);
	}
	VertexBufferGPU = std::make_unique<ComPtr<ID3D12Resource>[]>(numMaterial);
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
				VertexView& vv = PD[i]->updateDXR[dx->dxrBuffSwap[1]].VviewDXR[j][t];
				dx->createDefaultResourceBuffer(VertexBufferGPU[MaterialCnt].GetAddressOf(),
					vv.VertexBufferByteSize, D3D12_RESOURCE_STATE_GENERIC_READ);
				MaterialCnt++;
			}
		}
	}
}

void DXR_Basic::createBottomLevelAS1(Dx12Process_sub* com, VertexView* vv,
	IndexView* iv, UINT currentIndexCount, UINT MaterialNo, bool update, bool alphaTest) {

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
	if (alphaTest)
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;//anyhit起動可
	else
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;//anyhit起動不可

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = buildFlag;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	//AccelerationStructureを構築する為のリソース要件を取得(infoに入る)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (!bLB.firstSet) {
		//リソース要件を元にUAV作成
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
		asDesc.SourceAccelerationStructureData = 0; //ALLOW_UPDATEの場合0にする
	}
	else {
		bLB.firstSet = true;
	}

	//bottom-level AS作成, 第三引数は作成後の情報, 不要の場合nullptr
	com->mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	com->delayUavResourceBarrier(bLB.pResult.Get());
}

void DXR_Basic::createBottomLevelAS(Dx12Process_sub* com) {
	Dx12Process* dx = Dx12Process::GetInstance();
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		bool createAS = ud.createAS;
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
				VertexView* vv = &ud.VviewDXR[j][t];
				IndexView* iv = &PD[i]->IviewDXR[j];
				UINT indexCnt = ud.currentIndexCount[j][t];
				if (ud.firstSet && (PD[i]->updateF || !ud.createAS)) {
					createBottomLevelAS1(com, vv, iv, indexCnt, MaterialCnt, PD[i]->updateF, PD[i]->alphaTest);
					createAS = true;
				}
				MaterialCnt++;
			}
		}
		ud.createAS = createAS;
	}
	com->RunDelayUavResourceBarrier();
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
	//AccelerationStructureを構築する為のリソース要件を取得(infoに入る)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (!tLB.firstSet) {
		//リソース要件を元にUAV作成
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

				if (ud.firstSet) {
					UINT matAddInd = 0;
					if (PD[i]->hs)matAddInd = k;
					D3D12_RAYTRACING_INSTANCE_DESC& pID = pInstanceDesc[RayInstanceCnt];
					UINT materialInstanceID = materialCnt + matAddInd;//max65535
					UINT InstancingID = i * INSTANCE_PCS_3D + k;//max65535
					UINT InstanceID = ((materialInstanceID << 16) & 0xffff0000) | (InstancingID & 0x0000ffff);
					ud.InstanceID[INSTANCE_PCS_3D * j + k] = InstanceID;
					pID.InstanceID = InstanceID;//この値は、InstanceID()を介してシェーダーに公開されます
					pID.InstanceContributionToHitGroupIndex = 0;//使用するhitShaderインデックス
					pID.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
					memcpy(pID.Transform, &ud.Transform[k], sizeof(pID.Transform));
					pID.AccelerationStructure = asObj[buffSwap[0]].bottomLevelBuffers[materialCnt + matAddInd].pResult.Get()->GetGPUVirtualAddress();
					pID.InstanceMask = ud.InstanceMask;
				}
				RayInstanceCnt++;
			}
			materialCnt += PD[i]->NumMaxInstance;
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
		asDesc.SourceAccelerationStructureData = 0; //ALLOW_UPDATEの場合0にする
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
static const WCHAR* kAnyBasicClosestHitShader = L"anyBasicHit";
static const WCHAR* kBasicMissShader = L"basicMiss";
static const WCHAR* kBasicClosestHitShader = L"basicHit";
static const WCHAR* kBasicHitGroup = L"basicHitGroup";
static const WCHAR* kEmissiveMiss = L"emissiveMiss";
static const WCHAR* kEmissiveHitShader = L"emissiveHit";
static const WCHAR* kEmissiveHitGroup = L"emissiveHitGroup";

static ComPtr<ID3DBlob> CompileLibrary(char* shaderByte, const WCHAR* filename, const WCHAR* targetString) {

	//コンパイルライブラリ初期化
	HRESULT hr_Hel = gDxcDllHelper.Initialize();
	if (FAILED(hr_Hel)) {
		ErrorMessage("DXR CompileLibrary gDxcDllHelper.Initialize() Error");
	}
	ComPtr<IDxcCompiler> pCompiler;
	ComPtr<IDxcLibrary> pLibrary;
	gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, pCompiler.GetAddressOf());
	gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, pLibrary.GetAddressOf());

	uint32_t size = (uint32_t)strlen(shaderByte) + 1;
	//string → IDxcBlob変換
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

	//Compile結果
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
	//シェーダーのコンパイル
	ComPtr<ID3DBlob> pDxilLib = CompileLibrary(dxrShader[2].str, L"ShaderBasicDXR", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader,
		kAnyBasicClosestHitShader, kBasicMissShader, kBasicClosestHitShader,
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

static RootSignatureDesc createRayGenRootDesc(UINT numMaterial, UINT numInstancing) {

	RootSignatureDesc desc = {};
	int numDescriptorRanges = 7;
	desc.range.resize(numDescriptorRanges);
	UINT descCnt = 0;
	//gOutput(u0)
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[0].OffsetInDescriptorsFromTableStart = descCnt++;

	//gDepthOut(u1)
	desc.range[1].BaseShaderRegister = 1;
	desc.range[1].NumDescriptors = 1;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[1].OffsetInDescriptorsFromTableStart = descCnt++;

	//Indices(t0)
	desc.range[2].BaseShaderRegister = 0;
	desc.range[2].NumDescriptors = numMaterial;
	desc.range[2].RegisterSpace = 1;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//Vertex(t1)
	desc.range[3].BaseShaderRegister = 1;
	desc.range[3].NumDescriptors = numMaterial;
	desc.range[3].RegisterSpace = 2;
	desc.range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[3].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//cbuffer global(b0)
	desc.range[4].BaseShaderRegister = 0;
	desc.range[4].NumDescriptors = 1;
	desc.range[4].RegisterSpace = 0;
	desc.range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[4].OffsetInDescriptorsFromTableStart = descCnt++;

	//material(b1)
	desc.range[5].BaseShaderRegister = 1;
	desc.range[5].NumDescriptors = numMaterial;
	desc.range[5].RegisterSpace = 3;
	desc.range[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[5].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//wvp(b2)
	desc.range[6].BaseShaderRegister = 2;
	desc.range[6].NumDescriptors = numInstancing;
	desc.range[6].RegisterSpace = 4;
	desc.range[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[6].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numInstancing;

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

	//CreateStateObject作成に必要な各D3D12_STATE_SUBOBJECTを作成
	std::array<D3D12_STATE_SUBOBJECT, 13> subobjects;
	uint32_t index = 0;

	//DXIL library 初期化, SUBOBJECT作成
	addChar str;
	str.addStr(dx->ShaderNormalTangentCopy.get(), dx->ShaderCalculateLightingCopy.get());
	DxilLibrary dxilLib = createDxilLibrary(str.str);//Dx12Processに保持してるshaderを入力
	subobjects[index++] = dxilLib.stateSubobject;

	//BasicHitShader SUBOBJECT作成
	HitProgram basicHitProgram(kAnyBasicClosestHitShader, kBasicClosestHitShader, kBasicHitGroup);
	subobjects[index++] = basicHitProgram.subObject;

	//EmissiveHitShader SUBOBJECT作成
	HitProgram emissiveHitProgram(nullptr, kEmissiveHitShader, kEmissiveHitGroup);
	subobjects[index++] = emissiveHitProgram.subObject;

	//raygenerationShaderルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature rgsRootSignature(createRootSignature(createRayGenRootDesc(numMaterial, maxNumInstancing).desc));
	subobjects[index] = rgsRootSignature.subobject;

	//raygenerationShaderと↑のルートシグネチャ関連付け, SUBOBJECT作成
	uint32_t rgsRootIndex = index++;
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject;

	//basicHitShader, basicMissShader ルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature basicHitMissRootSignature(createRootSignature(createRayGenRootDesc(numMaterial, maxNumInstancing).desc));
	subobjects[index] = basicHitMissRootSignature.subobject;

	//↑のbasicHitShader, basicMissShader 共有ルートシグネチャとbasicHitShader, basicMissShader関連付け, SUBOBJECT作成
	uint32_t basicHitMissRootIndex = index++;
	const WCHAR* basicMissHitExportName[] = { kAnyBasicClosestHitShader, kBasicMissShader, kBasicClosestHitShader };
	ExportAssociation basicMissHitRootAssociation(basicMissHitExportName, ARRAY_SIZE(basicMissHitExportName),
		&(subobjects[basicHitMissRootIndex]));
	subobjects[index++] = basicMissHitRootAssociation.subobject;

	//kEmissiveHit, kEmissiveMiss ルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature emissiveRootSignature(createRootSignature(createRayGenRootDesc(numMaterial, maxNumInstancing).desc));
	subobjects[index] = emissiveRootSignature.subobject;

	uint32_t emissiveRootIndex = index++;
	const WCHAR* emissiveRootExport[] = { kEmissiveHitShader, kEmissiveMiss };
	ExportAssociation emissiveRootAssociation(emissiveRootExport, ARRAY_SIZE(emissiveRootExport),
		&(subobjects[emissiveRootIndex]));
	subobjects[index++] = emissiveRootAssociation.subobject;

	//ペイロードサイズをプログラムにバインドする SUBOBJECT作成
	uint32_t MaxAttributeSizeInBytes = sizeof(float) * 2;
	uint32_t maxPayloadSizeInBytes = sizeof(float) * 13;
	ShaderConfig shaderConfig(MaxAttributeSizeInBytes, maxPayloadSizeInBytes);
	subobjects[index] = shaderConfig.subobject;

	//ShaderConfigと全てのシェーダー関連付け SUBOBJECT作成
	uint32_t shaderConfigIndex = index++;
	const WCHAR* shaderExports[] = {
		kRayGenShader,
		kAnyBasicClosestHitShader, kBasicMissShader, kBasicClosestHitShader,
		kEmissiveHitShader, kEmissiveMiss
	};
	ExportAssociation configAssociation(shaderExports, ARRAY_SIZE(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject;

	//パイプラインコンフィグ作成 SUBOBJECT作成
	PipelineConfig config(maxRecursion);
	subobjects[index++] = config.subobject;

	//グローバルルートシグネチャ作成 SUBOBJECT作成
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

	dx->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mpDepthResource.GetAddressOf(),
		dx->mClientWidth,
		dx->mClientHeight,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		DXGI_FORMAT_R32_FLOAT);

	//Local
	int num_u0 = 1;
	int num_u1 = 1;
	int num_t0 = numMaterial;
	int num_t1 = numMaterial;
	int num_b0 = 1;
	int num_b1 = numMaterial;
	int num_b2 = maxNumInstancing;
	numSkipLocalHeap = num_u0 + num_u1 + num_t0 + num_t1 + num_b0 + num_b1 + num_b2;
	//Global
	int num_t00 = numMaterial;
	int num_t01 = numMaterial;
	int num_t02 = numMaterial;
	int numHeap = numSkipLocalHeap + num_t00 + num_t01 + num_t02;

	//Local
	mpSrvUavCbvHeap = dx->CreateDescHeap(numHeap);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavCbvHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	UINT offsetSize = dx->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//UAVを作成 gOutput(u0)
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	dx->getDevice()->CreateUnorderedAccessView(mpOutputResource.Get(), nullptr, &uavDesc, srvHandle);

	//UAVを作成 gDepthOut(u1)
	srvHandle.ptr += offsetSize;
	dx->getDevice()->CreateUnorderedAccessView(mpDepthResource.Get(), nullptr, &uavDesc, srvHandle);

	//SRVを作成 Indices(t0)
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
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
	}

	//SRVを作成 Vertex(t1)
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
				VertexView& vv = PD[i]->updateDXR[dx->dxrBuffSwap[1]].VviewDXR[j][t];
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
	}

	//CBVを作成 cbuffer global(b0)
	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	bufferDesc.SizeInBytes = sCB->getSizeInBytes();
	bufferDesc.BufferLocation = sCB->Resource()->GetGPUVirtualAddress();
	srvHandle.ptr += offsetSize;
	dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);

	//CBVを作成 material(b1)
	for (UINT i = 0; i < numMaterial; i++) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
		bufferDesc.SizeInBytes = material->getElementByteSize();
		bufferDesc.BufferLocation = material->getGPUVirtualAddress(i);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);
	}

	//CBVを作成 wvp(b2)
	for (UINT i = 0; i < maxNumInstancing; i++) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
		bufferDesc.SizeInBytes = wvp->getElementByteSize();
		bufferDesc.BufferLocation = wvp->getGPUVirtualAddress(i);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);
	}

	//Global
	//SRVを作成 g_texDiffuse(t0), g_texNormal(t1), g_texSpecular(t2)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3 = {};
	for (UINT t = 0; t < 3; t++) {
		for (UINT i = 0; i < numParameter; i++) {
			for (int j = 0; j < PD[i]->NumMaterial; j++) {
				for (UINT ins = 0; ins < PD[i]->NumMaxInstance; ins++) {
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
		シェーダーテーブルの各エントリは同じサイズにする必要あり
		一番サイズの大きいエントリに合わせる
		ray-genプログラムには最大のエントリが必要-ディスクリプタテーブルのsizeof（プログラム識別子）+ 8バイト
		エントリサイズはD3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENTにアライメントする
	*/

	//サイズ計算
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 8; //ray-genは + 8
	mShaderTableEntrySize = ALIGNMENT_TO(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
	uint32_t shaderTableSize = mShaderTableEntrySize * 5;//raygen + miss * 2 + hit * 2

	Dx12Process::GetInstance()->createUploadResource(mpShaderTable.GetAddressOf(), shaderTableSize);

	uint8_t* pData;
	mpShaderTable.Get()->Map(0, nullptr, (void**)&pData);

	ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	mpPipelineState.Get()->QueryInterface(IID_PPV_ARGS(pRtsoProps.GetAddressOf()));

	//Entry 0 - ray-gen shader 識別子を取得, 記録
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
			for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
				VECTOR4& dif = PD[i]->diffuse[j];
				VECTOR4& spe = PD[i]->specular[j];
				VECTOR4& amb = PD[i]->ambient[j];
				DxrMaterialCB& mcb = cbObj->matCb[MaterialCnt];
				mcb.vDiffuse.as(dif.x, dif.y, dif.z, 0.0f);
				mcb.vSpeculer.as(spe.x, spe.y, spe.z, 0.0f);
				mcb.vAmbient.as(amb.x, amb.y, amb.z, 0.0f);
				mcb.shininess = ud.shininess;
				mcb.RefractiveIndex = ud.RefractiveIndex;
				memcpy(&mcb.AddObjColor, &ud.AddObjColor, sizeof(VECTOR4));
				MaterialCnt++;
			}
		}
	}
}

void DXR_Basic::updateCB(CBobj* cbObj, UINT numRecursion) {
	Dx12Process* dx = Dx12Process::GetInstance();
	MATRIX VP;
	Dx12Process::Update& upd = dx->upd[dx->cBuffSwap[1]];
	MatrixMultiply(&VP, &upd.mView, &upd.mProj);
	MatrixTranspose(&VP);
	DxrConstantBuffer& cb = cbObj->cb;
	MatrixInverse(&cb.projectionToWorld, &VP);
	cb.cameraUp.as(upd.up.x, upd.up.y, upd.up.z, 1.0f);
	cb.cameraPosition.as(upd.pos.x, upd.pos.y, upd.pos.z, 1.0f);
	cb.maxRecursion = numRecursion;
	updateMaterial(cbObj);

	int cntEm = 0;
	bool breakF = false;
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < ud.NumInstance; k++) {
				UINT matAddInd = 0;
				if (PD[i]->hs)matAddInd = k;
				if (cbObj->matCb[MaterialCnt + matAddInd].materialNo == 2) {
					for (UINT v = 0; v < ud.numVertex; v++) {
						MATRIX Transpose;
						memcpy(&Transpose, &ud.Transform[k], sizeof(MATRIX));
						MatrixTranspose(&Transpose);
						VECTOR3 v3;
						if (ud.useVertex) {
							v3.as(ud.v[v].x, ud.v[v].y, ud.v[v].z);
							VectorMatrixMultiply(&v3, &Transpose);
						}
						else {
							v3.as(Transpose._41, Transpose._42, Transpose._43);
						}
						cb.emissivePosition[cntEm].x = v3.x;
						cb.emissivePosition[cntEm].y = v3.y;
						cb.emissivePosition[cntEm].z = v3.z;
						cb.emissivePosition[cntEm].w = upd.plight.LightPos[cntEm].w;
						cb.Lightst[cntEm].x = upd.plight.Lightst[cntEm].x;
						cb.Lightst[cntEm].y = upd.plight.Lightst[cntEm].y;
						cb.Lightst[cntEm].z = upd.plight.Lightst[cntEm].z;
						cb.Lightst[cntEm].w = upd.plight.Lightst[cntEm].w;
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
			MaterialCnt += PD[i]->NumMaxInstance;
			if (breakF)break;
		}
		if (breakF)break;
	}

	cb.numEmissive.x = (float)cntEm;
	memcpy(&cb.GlobalAmbientColor, &dx->GlobalAmbientLight, sizeof(VECTOR4));
	memcpy(&cb.dDirection, &upd.dlight.Direction, sizeof(VECTOR4));
	memcpy(&cb.dLightColor, &upd.dlight.LightColor, sizeof(VECTOR4));
	memcpy(&cb.dLightst, &upd.dlight.onoff, sizeof(VECTOR4));

	for (UINT i = 0; i < numParameter; i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dx->dxrBuffSwap[1]];
		for (UINT k = 0; k < ud.NumInstance; k++) {
			int index = INSTANCE_PCS_3D * i + k;
			memcpy(&cbObj->wvpCb[index].wvp, &ud.WVP[k], sizeof(MATRIX));
			memcpy(&cbObj->wvpCb[index].world, &ud.Transform[k], sizeof(MATRIX));
			wvp->CopyData(index, cbObj->wvpCb[index]);
		}
	}
}

void DXR_Basic::updateAS(Dx12Process_sub* com, UINT numRecursion) {
	createBottomLevelAS(com);
	createTopLevelAS(com);
	updateCB(&cbObj[buffSwap[0]], numRecursion);
}

void DXR_Basic::updateVertexBuffer(int comNo) {
	UINT MaterialCnt = 0;
	Dx12Process* dx = Dx12Process::GetInstance();
	Dx12Process_sub& d = dx->dx_sub[comNo];
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < PD[i]->NumMaxInstance; t++) {
				VertexView& vv = PD[i]->updateDXR[dx->dxrBuffSwap[1]].VviewDXR[j][t];

				d.delayResourceBarrierBefore(VertexBufferGPU[MaterialCnt].Get(),
					D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
				d.delayResourceBarrierBefore(vv.VertexBufferGPU.Get(),
					D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
				d.delayCopyResource(VertexBufferGPU[MaterialCnt].Get(), vv.VertexBufferGPU.Get());
				d.delayResourceBarrierAfter(VertexBufferGPU[MaterialCnt].Get(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
				d.delayResourceBarrierAfter(vv.VertexBufferGPU.Get(),
					D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);

				MaterialCnt++;
			}
		}
	}
	d.RunDelayResourceBarrierBefore();
	d.RunDelayCopyResource();
	d.RunDelayResourceBarrierAfter();
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
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;//missShader 2個

	//Hit
	size_t hitOffset = 3 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * 2;//hitShader 2個

	//Dispatch
	com->mCommandList->SetPipelineState1(mpPipelineState.Get());
	com->mCommandList->DispatchRays(&raytraceDesc);
}

void DXR_Basic::copyBackBuffer(int comNo) {
	//結果をバックバッファにコピー
	Dx12Process* dx = Dx12Process::GetInstance();
	Dx12Process_sub& d = dx->dx_sub[comNo];
	d.delayResourceBarrierBefore(mpOutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	d.delayResourceBarrierBefore(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	d.delayCopyResource(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		mpOutputResource.Get());

	d.delayResourceBarrierAfter(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	d.delayResourceBarrierAfter(mpOutputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void DXR_Basic::copyDepthBuffer(int comNo) {

	Dx12Process* dx = Dx12Process::GetInstance();
	Dx12Process_sub& d = dx->dx_sub[comNo];
	d.delayResourceBarrierBefore(mpDepthResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	d.delayResourceBarrierBefore(dx->mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);

	d.delayCopyResource(dx->mDepthStencilBuffer.Get(), mpDepthResource.Get());

	d.delayResourceBarrierAfter(dx->mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	d.delayResourceBarrierAfter(mpDepthResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
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

DXR_Basic::~DXR_Basic() {
	S_DELETE(sCB);
	S_DELETE(material);
	S_DELETE(wvp);
}