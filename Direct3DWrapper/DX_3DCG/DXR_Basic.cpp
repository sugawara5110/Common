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

void DXR_Basic::initDXR(int comNo, UINT numparameter, ParameterDXR** pd, MaterialType* type) {

	PD = pd;
	Dx12Process* dx = Dx12Process::GetInstance();

	if (dx->DXR_CreateResource) {
		numParameter = numparameter;
		UINT cnt = 0;
		for (UINT i = 0; i < numParameter; i++) {
			cnt += PD[i]->NumMaterial;
		}
		numMaterial = cnt;
		maxNumInstancing = INSTANCE_PCS_3D * numParameter;

		material = new ConstantBuffer<DxrMaterialCB>(numMaterial);
		matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
		instance = new ConstantBuffer<DxrInstanceCB>(maxNumInstancing);
		insCb = std::make_unique<DxrInstanceCB[]>(maxNumInstancing);
		sCB = new ConstantBuffer<DxrConstantBuffer>(1);

		dx->dx_sub[comNo].Bigin();
		createTriangleVB(comNo, numMaterial);
		createAccelerationStructures(comNo);
		dx->dx_sub[comNo].End();
		dx->WaitFence();
		for (UINT i = 0; i < numMaterial; i++) {
			mpBottomLevelAS[i] = bottomLevelBuffers[i].pResult;

			switch (type[i]) {
			case METALLIC:
				matCb[i].materialNo = 0;
				break;
			case EMISSIVE:
				matCb[i].materialNo = 1;
				break;
			case NONREFLECTION:
				matCb[i].materialNo = 2;
				break;
			}
		}
		mpTopLevelAS = topLevelBuffers.pResult;

		createRtPipelineState();
		createShaderResources();
		createShaderTable();
	}
}

void DXR_Basic::createTriangleVB(int comNo, UINT numMaterial) {

	Dx12Process* dx = Dx12Process::GetInstance();
	pVertexBuffer = std::make_unique<VertexObj[]>(numMaterial);
	pIndexBuffer = std::make_unique<IndexObj[]>(numMaterial);

	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			VertexObj& vb = pVertexBuffer[MaterialCnt];
			VertexView& vv = PD[i]->VviewDXR[j];
			vb.VertexByteStride = vv.VertexByteStride;
			vb.VertexBufferByteSize = vv.VertexBufferByteSize;
			vb.VertexBufferGPU = vv.VertexBufferGPU.Get();

			IndexObj& ib = pIndexBuffer[MaterialCnt];
			IndexView& iv = PD[i]->IviewDXR[j];
			ib.IndexBufferByteSize = iv.IndexBufferByteSize;
			ib.IndexCount = iv.IndexCount;
			ib.IndexBufferGPU = iv.IndexBufferGPU.Get();
			MaterialCnt++;
		}
	}
}

void DXR_Basic::createBottomLevelAS(int comNo, UINT MaterialNo, bool update) {

	Dx12Process* dx = Dx12Process::GetInstance();

	VertexObj& pVB = pVertexBuffer[MaterialNo];
	IndexObj& pIB = pIndexBuffer[MaterialNo];

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexBuffer.StartAddress = pVB.VertexBufferGPU->GetGPUVirtualAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = pVB.VertexByteStride;
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	UINT numVertices = pVB.VertexBufferByteSize / pVB.VertexByteStride;
	geomDesc.Triangles.VertexCount = numVertices;
	geomDesc.Triangles.IndexBuffer = pIB.IndexBufferGPU->GetGPUVirtualAddress();
	geomDesc.Triangles.IndexCount = pIB.IndexCount;
	geomDesc.Triangles.IndexFormat = pIB.IndexFormat;
	geomDesc.Triangles.Transform3x4 = 0;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	//AccelerationStructureを構築する為のリソース要件を取得(infoに入る)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	AccelerationStructureBuffers& bLB = bottomLevelBuffers[MaterialNo];

	if (update) {
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = bLB.pResult.Get();
		dx->dx_sub[comNo].mCommandList->ResourceBarrier(1, &uavBarrier);
	}
	else {
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

	if (update) {
		asDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		asDesc.SourceAccelerationStructureData = 0; //ALLOW_UPDATEの場合0にする
	}

	//bottom-level AS作成, 第三引数は作成後の情報, 不要の場合nullptr
	dx->dx_sub[comNo].mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	//バリアしたリソースへのUAVアクセスに於いて次のUAVアクセス開始前に現在のUAVアクセスが終了する必要がある事を示す
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = bLB.pResult.Get();

	dx->dx_sub[comNo].mCommandList->ResourceBarrier(1, &uavBarrier);
}

void DXR_Basic::createTopLevelAS(int comNo, uint64_t& tlasSize, bool update) {

	Dx12Process* dx = Dx12Process::GetInstance();

	UINT numRayInstance = 0;
	for (UINT i = 0; i < numParameter; i++) {
		if (update)
			numRayInstance += PD[i]->NumInstance * PD[i]->NumMaterial;
		else
			numRayInstance += INSTANCE_PCS_3D * PD[i]->NumMaterial;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = numRayInstance;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	//AccelerationStructureを構築する為のリソース要件を取得(infoに入る)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (update) {
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = topLevelBuffers.pResult.Get();
		dx->dx_sub[comNo].mCommandList->ResourceBarrier(1, &uavBarrier);
	}
	else {
		//リソース要件を元にUAV作成
		dx->createDefaultResourceBuffer_UNORDERED_ACCESS(topLevelBuffers.pScratch.GetAddressOf(), info.ScratchDataSizeInBytes,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		dx->createDefaultResourceBuffer_UNORDERED_ACCESS(topLevelBuffers.pResult.GetAddressOf(), info.ResultDataMaxSizeInBytes,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
		dx->createUploadResource(topLevelBuffers.pInstanceDesc.GetAddressOf(),
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numRayInstance);
		tlasSize = info.ResultDataMaxSizeInBytes;
	}

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	topLevelBuffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
	ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numRayInstance);

	UINT RayInstanceCnt = 0;
	UINT materialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < PD[i]->NumInstance; k++) {
				//InstanceDesc初期化
				D3D12_RAYTRACING_INSTANCE_DESC& pID = pInstanceDesc[RayInstanceCnt];
				UINT materialInstanceID = materialCnt;
				UINT InstancingID = i * INSTANCE_PCS_3D + k;
				UINT InstanceID = ((materialInstanceID << 16) & 0xffff0000) | (InstancingID & 0x0000ffff);
				pID.InstanceID = InstanceID;//この値は、InstanceID()を介してシェーダーに公開されます
				pID.InstanceContributionToHitGroupIndex = 0;//シェーダーテーブル内のオフセット。ジオメトリが一つの場合,オフセット0
				pID.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				memcpy(pID.Transform, &PD[i]->Transform[k], sizeof(pID.Transform));
				pID.AccelerationStructure = bottomLevelBuffers[materialCnt].pResult.Get()->GetGPUVirtualAddress();
				pID.InstanceMask = 0xFF;
				RayInstanceCnt++;
			}
			materialCnt++;
		}
	}

	topLevelBuffers.pInstanceDesc->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = topLevelBuffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = topLevelBuffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = topLevelBuffers.pScratch->GetGPUVirtualAddress();

	if (update) {
		//更新範囲広い場合ALLOW_UPDATEだとうまくいく
		asDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		asDesc.SourceAccelerationStructureData = 0; //ALLOW_UPDATEの場合0にする
	}

	dx->dx_sub[comNo].mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = topLevelBuffers.pResult.Get();
	dx->dx_sub[comNo].mCommandList->ResourceBarrier(1, &uavBarrier);
}

void DXR_Basic::createAccelerationStructures(int comNo) {

	bottomLevelBuffers = std::make_unique<AccelerationStructureBuffers[]>(numMaterial);
	mpBottomLevelAS = std::make_unique<ComPtr<ID3D12Resource>[]>(numMaterial);

	for (UINT i = 0; i < numMaterial; i++) {
		createBottomLevelAS(comNo, i, false);
	}
	createTopLevelAS(comNo, mTlasSize, false);
}

static const WCHAR* kRayGenShader = L"rayGen";
static const WCHAR* kCamMissShader = L"camMiss";
static const WCHAR* kCamClosestHitShader = L"camHit";
static const WCHAR* kCamHitGroup = L"camHitGroup";
static const WCHAR* kMetallicMiss = L"metallicMiss";
static const WCHAR* kMetallicHitShader = L"metallicHit";
static const WCHAR* kMetallicHitGroup = L"metallicHitGroup";
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
		kCamMissShader, kCamClosestHitShader,
		kMetallicMiss, kMetallicHitShader,
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

static RootSignatureDesc createRayGenRootDesc(UINT numMaterial, UINT maxNumInstancing) {

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

	//gRtScene(t0)
	desc.range[1].BaseShaderRegister = 0;
	desc.range[1].NumDescriptors = 1;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = descCnt++;

	//Indices(t1)
	desc.range[2].BaseShaderRegister = 1;
	desc.range[2].NumDescriptors = numMaterial;
	desc.range[2].RegisterSpace = 1;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numMaterial;

	//Vertex(t2)
	desc.range[3].BaseShaderRegister = 2;
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

	//instance(b1)
	desc.range[5].BaseShaderRegister = 1;
	desc.range[5].NumDescriptors = maxNumInstancing;
	desc.range[5].RegisterSpace = 3;
	desc.range[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[5].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += maxNumInstancing;

	//material(b2)
	desc.range[6].BaseShaderRegister = 2;
	desc.range[6].NumDescriptors = numMaterial;
	desc.range[6].RegisterSpace = 4;
	desc.range[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[6].OffsetInDescriptorsFromTableStart = descCnt;
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

	desc.rootParams.resize(2);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 3;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();
	desc.rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[1].DescriptorTable.pDescriptorRanges = &desc.range[3];
	desc.rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.desc.NumParameters = 2;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return desc;
}

void DXR_Basic::createRtPipelineState() {

	Dx12Process* dx = Dx12Process::GetInstance();

	//CreateStateObject作成に必要な各D3D12_STATE_SUBOBJECTを作成
	std::array<D3D12_STATE_SUBOBJECT, 16> subobjects;
	uint32_t index = 0;

	//DXIL library 初期化, SUBOBJECT作成
	addChar str;
	str.addStr(dx->ShaderNormalTangentCopy.get(), dx->ShaderCalculateLightingCopy.get());
	DxilLibrary dxilLib = createDxilLibrary(str.str);//Dx12Processに保持してるshaderを入力
	subobjects[index++] = dxilLib.stateSubobject;

	//CamhitShader SUBOBJECT作成
	HitProgram camHitProgram(nullptr, kCamClosestHitShader, kCamHitGroup);
	subobjects[index++] = camHitProgram.subObject;

	//MetallicHitShader SUBOBJECT作成
	HitProgram metallicHitProgram(nullptr, kMetallicHitShader, kMetallicHitGroup);
	subobjects[index++] = metallicHitProgram.subObject;

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

	//camHitShader, camMissShader ルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature camHitMissRootSignature(createRootSignature(createRayGenRootDesc(numMaterial, maxNumInstancing).desc));
	subobjects[index] = camHitMissRootSignature.subobject;

	//↑のcamHitShader, camMissShader 共有ルートシグネチャとcamHitShader, camMissShader関連付け, SUBOBJECT作成
	uint32_t camHitMissRootIndex = index++;
	const WCHAR* camMissHitExportName[] = { kCamMissShader, kCamClosestHitShader };
	ExportAssociation camMissHitRootAssociation(camMissHitExportName, ARRAY_SIZE(camMissHitExportName),
		&(subobjects[camHitMissRootIndex]));
	subobjects[index++] = camMissHitRootAssociation.subobject;

	//kMetallicHit, kMetallicMiss ルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature metallicRootSignature(createRootSignature(createRayGenRootDesc(numMaterial, maxNumInstancing).desc));
	subobjects[index] = metallicRootSignature.subobject;

	uint32_t metallicRootIndex = index++;
	const WCHAR* metallicRootExport[] = { kMetallicHitShader, kMetallicMiss };
	ExportAssociation metallicRootAssociation(metallicRootExport, ARRAY_SIZE(metallicRootExport),
		&(subobjects[metallicRootIndex]));
	subobjects[index++] = metallicRootAssociation.subobject;

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
	uint32_t maxPayloadSizeInBytes = sizeof(float) * 5;
	ShaderConfig shaderConfig(MaxAttributeSizeInBytes, maxPayloadSizeInBytes);
	subobjects[index] = shaderConfig.subobject;

	//ShaderConfigと全てのシェーダー関連付け SUBOBJECT作成
	uint32_t shaderConfigIndex = index++;
	const WCHAR* shaderExports[] = {
		kRayGenShader,
		kCamClosestHitShader,kCamMissShader,
		kMetallicHitShader, kMetallicMiss,
		kEmissiveHitShader, kEmissiveMiss
	};
	ExportAssociation configAssociation(shaderExports, ARRAY_SIZE(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject;

	//パイプラインコンフィグ作成 SUBOBJECT作成
	PipelineConfig config(3);
	subobjects[index++] = config.subobject;

	//グローバルルートシグネチャ作成 SUBOBJECT作成
	GlobalRootSignature root(createRootSignature(createGlobalRootDesc(numMaterial).desc));
	mpGlobalRootSig = root.pRootSig;
	subobjects[index++] = root.subobject;

	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index;
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	HRESULT hr = dx->getDevice()->CreateStateObject(&desc, IID_PPV_ARGS(mpPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		ErrorMessage("DXR_Basic::createRtPipelineState() Error");
	}
}

void DXR_Basic::createShaderResources() {

	Dx12Process* dx = Dx12Process::GetInstance();
	//出力リソースを作成します。寸法と形式はスワップチェーンと一致している必要があります
	//バックバッファは実際にはDXGI_FORMAT_R8G8B8A8_UNORM_SRGBですが、
	//sRGB形式はUAVで使用できません。シェーダーで自分自身をsRGBに変換します
	dx->createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mpOutputResource.GetAddressOf(),
		dx->mClientWidth,
		dx->mClientHeight,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	//Local
	int num_u0 = 1;
	int num_t0 = 1;
	int num_t1 = numMaterial;
	int num_t2 = numMaterial;
	int num_b0 = 1;
	int num_b1 = maxNumInstancing;
	int num_b2 = numMaterial;
	numSkipLocalHeap = num_u0 + num_t0 + num_t1 + num_t2 + num_b0 + num_b1 + num_b2;
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

	//SRVを作成 gRtScene(t0)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc0 = {};
	srvDesc0.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc0.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc0.RaytracingAccelerationStructure.Location = mpTopLevelAS.Get()->GetGPUVirtualAddress();
	srvHandle.ptr += offsetSize;
	dx->getDevice()->CreateShaderResourceView(nullptr, &srvDesc0, srvHandle);

	//SRVを作成 Indices(t1)
	for (UINT i = 0; i < numMaterial; i++) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1 = {};
		srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc1.Buffer.NumElements = pIndexBuffer[i].IndexCount;
		srvDesc1.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc1.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc1.Buffer.StructureByteStride = sizeof(uint32_t);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateShaderResourceView(pIndexBuffer[i].IndexBufferGPU, &srvDesc1, srvHandle);
	}

	//SRVを作成 Vertex(t2)
	for (UINT i = 0; i < numMaterial; i++) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
		srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc2.Buffer.NumElements = pVertexBuffer[i].VertexBufferByteSize / pVertexBuffer[i].VertexByteStride;
		srvDesc2.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc2.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc2.Buffer.StructureByteStride = pVertexBuffer[i].VertexByteStride;
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateShaderResourceView(pVertexBuffer[i].VertexBufferGPU, &srvDesc2, srvHandle);
	}

	//CBVを作成 cbuffer global(b0)
	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	bufferDesc.SizeInBytes = sCB->getSizeInBytes();
	bufferDesc.BufferLocation = sCB->Resource()->GetGPUVirtualAddress();
	srvHandle.ptr += offsetSize;
	dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);

	//CBVを作成 instance(b1)
	for (UINT i = 0; i < maxNumInstancing; i++) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
		bufferDesc.SizeInBytes = instance->getElementByteSize();
		bufferDesc.BufferLocation = instance->getGPUVirtualAddress(i);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);
	}

	//CBVを作成 material(b2)
	for (UINT i = 0; i < numMaterial; i++) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
		bufferDesc.SizeInBytes = material->getElementByteSize();
		bufferDesc.BufferLocation = material->getGPUVirtualAddress(i);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);
	}

	//Global
	//SRVを作成 g_texDiffuse(t0), g_texNormal(t1), g_texSpecular(t2)
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
		シェーダーテーブルの各エントリは同じサイズにする必要あり
		一番サイズの大きいエントリに合わせる
		ray-genプログラムには最大のエントリが必要-ディスクリプタテーブルのsizeof（プログラム識別子）+ 8バイト
		エントリサイズはD3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENTにアライメントする
	*/

	//サイズ計算
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 8; //ray-genは + 8
	mShaderTableEntrySize = ALIGNMENT_TO(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
	uint32_t shaderTableSize = mShaderTableEntrySize * 7;//raygen + miss * 3 + hit * 3

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

	//Entry 1 - cam miss program
	memcpy(pData + mShaderTableEntrySize, pRtsoProps.Get()->GetShaderIdentifier(kCamMissShader),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - metallic miss program
	memcpy(pData + mShaderTableEntrySize * 2, pRtsoProps->GetShaderIdentifier(kMetallicMiss),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 3 - emissive miss program
	memcpy(pData + mShaderTableEntrySize * 3, pRtsoProps->GetShaderIdentifier(kEmissiveMiss),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 4 - cam hit program
	uint8_t* pHitEntry = pData + mShaderTableEntrySize * 4;
	memcpy(pHitEntry, pRtsoProps.Get()->GetShaderIdentifier(kCamHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 5 - metallic hit program
	uint8_t* pHitEntry1 = pData + mShaderTableEntrySize * 5;
	memcpy(pHitEntry1, pRtsoProps.Get()->GetShaderIdentifier(kMetallicHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 6 - emissive hit program
	uint8_t* pHitEntry2 = pData + mShaderTableEntrySize * 6;
	memcpy(pHitEntry2, pRtsoProps.Get()->GetShaderIdentifier(kEmissiveHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	mpShaderTable.Get()->Unmap(0, nullptr);
}

void DXR_Basic::updateMaterial() {

	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {

			VECTOR4& dif = PD[i]->diffuse[j];
			VECTOR4& spe = PD[i]->specular[j];
			VECTOR4& amb = PD[i]->ambient[j];
			matCb[MaterialCnt].vDiffuse.as(dif.x, dif.y, dif.z, 0.0f);
			matCb[MaterialCnt].vSpeculer.as(spe.x, spe.y, spe.z, 0.0f);
			matCb[MaterialCnt].vAmbient.as(amb.x, amb.y, amb.z, 0.0f);
			matCb[MaterialCnt].shininess = PD[i]->shininess;
			if (PD[i]->alphaTest) {
				matCb[MaterialCnt].alphaTest = 1.0f;
			}
			else {
				matCb[MaterialCnt].alphaTest = 0.0f;
			}
			MaterialCnt++;
		}
	}
}

void DXR_Basic::setCB() {
	Dx12Process* dx = Dx12Process::GetInstance();
	MATRIX VP;
	MatrixMultiply(&VP, &dx->mView, &dx->mProj);
	MatrixTranspose(&VP);
	MatrixInverse(&cb.projectionToWorld, &VP);
	cb.cameraUp.as(dx->upX, dx->upY, dx->upZ, 1.0f);
	cb.cameraPosition.as(dx->posX, dx->posY, dx->posZ, 1.0f);
	updateMaterial();

	int cntEm = 0;
	bool breakF = false;
	UINT MaterialCnt = 0;
	for (UINT i = 0; i < numParameter; i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < PD[i]->NumInstance; k++) {
				if (matCb[MaterialCnt].materialNo == 1) {
					MATRIX Transpose;
					memcpy(&Transpose, &PD[i]->Transform[k], sizeof(MATRIX));
					MatrixTranspose(&Transpose);
					cb.emissivePosition[cntEm].x = Transpose._41;
					cb.emissivePosition[cntEm].y = Transpose._42;
					cb.emissivePosition[cntEm].z = Transpose._43;
					cb.emissivePosition[cntEm].w = 1.0f;//wはライトonoffに使う？
					cb.Lightst[cntEm].x = dx->plight.Lightst[cntEm].x;
					cb.Lightst[cntEm].y = dx->plight.Lightst[cntEm].y;
					cb.Lightst[cntEm].z = dx->plight.Lightst[cntEm].z;
					cb.Lightst[cntEm].w = dx->plight.Lightst[cntEm].w;
					cntEm++;
					if (cntEm >= LIGHT_PCS) {
						breakF = true;
						break;
					}
				}
			}
			MaterialCnt++;
			if (breakF)break;
		}
		if (breakF)break;
	}

	cb.numEmissive.x = (float)cntEm;
	memcpy(&cb.GlobalAmbientColor, &dx->GlobalAmbientLight, sizeof(VECTOR4));
	sCB->CopyData(0, cb);

	for (UINT i = 0; i < numMaterial; i++)material->CopyData(i, matCb[i]);

	for (UINT i = 0; i < numParameter; i++) {
		for (UINT k = 0; k < PD[i]->NumInstance; k++) {
			memcpy(&insCb[INSTANCE_PCS_3D * i + k].world, &PD[i]->Transform[k], sizeof(MATRIX));
			instance->CopyData(INSTANCE_PCS_3D * i + k, insCb[INSTANCE_PCS_3D * i + k]);
		}
	}
}

void DXR_Basic::raytrace(int comNo) {

	Dx12Process* dx = Dx12Process::GetInstance();
	setCB();

	dx->dx_sub[comNo].ResourceBarrier(mpOutputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	dx->dx_sub[comNo].mCommandList->SetComputeRootSignature(mpGlobalRootSig.Get());

	ID3D12DescriptorHeap* heaps[] = { mpSrvUavCbvHeap.Get(), mpSamplerHeap.Get() };
	dx->dx_sub[comNo].mCommandList->SetDescriptorHeaps(ARRAY_SIZE(heaps), heaps);

	UINT offsetSize = dx->mCbvSrvUavDescriptorSize;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavCbvHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	srvHandle.ptr += offsetSize * numSkipLocalHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = mpSamplerHeap.Get()->GetGPUDescriptorHandleForHeapStart();

	dx->dx_sub[comNo].mCommandList->SetComputeRootDescriptorTable(0, srvHandle);
	dx->dx_sub[comNo].mCommandList->SetComputeRootDescriptorTable(1, samplerHandle);

	for (UINT i = 0; i < numMaterial; i++)createBottomLevelAS(comNo, i, true);
	createTopLevelAS(comNo, mTlasSize, true);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = dx->mClientWidth;
	raytraceDesc.Height = dx->mClientHeight;
	raytraceDesc.Depth = 3;

	//RayGen
	raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress();
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

	//Miss
	size_t missOffset = 1 * mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 3;//missShader 3個

	//Hit
	size_t hitOffset = 4 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable.Get()->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * 3;//hitShader 3個

	//Dispatch
	dx->dx_sub[comNo].mCommandList->SetPipelineState1(mpPipelineState.Get());
	dx->dx_sub[comNo].mCommandList->DispatchRays(&raytraceDesc);

	//結果をバックバッファにコピー
	dx->dx_sub[comNo].ResourceBarrier(mpOutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	dx->dx_sub[comNo].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	dx->dx_sub[comNo].mCommandList->CopyResource(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		mpOutputResource.Get());

	dx->dx_sub[comNo].ResourceBarrier(dx->mSwapChainBuffer[dx->mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
}