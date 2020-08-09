//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DXR_Basic.cpp                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DXR_Basic.h"
#include <fstream>
#include <sstream>
#include "./ShaderDXR/ShaderBasicDXR.h"

static dxc::DxcDllSupport gDxcDllHelper;

AccelerationStructureBuffers DXR_Basic::createBottomLevelAS(int comNo, UINT InstanceID) {

	Dx12Process* dx = Dx12Process::GetInstance();

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexBuffer.StartAddress = pVertexBuffer[InstanceID].VertexBufferGPU.Get()->GetGPUVirtualAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = pVertexBuffer[InstanceID].VertexByteStride;
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	UINT numVertices = pVertexBuffer[InstanceID].VertexBufferByteSize / pVertexBuffer[InstanceID].VertexByteStride;
	geomDesc.Triangles.VertexCount = numVertices;
	geomDesc.Triangles.IndexBuffer = pIndexBuffer[InstanceID].IndexBufferGPU.Get()->GetGPUVirtualAddress();
	geomDesc.Triangles.IndexCount = pIndexBuffer[InstanceID].IndexCount;
	geomDesc.Triangles.IndexFormat = pIndexBuffer[InstanceID].IndexFormat;
	geomDesc.Triangles.Transform3x4 = 0;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	//AccelerationStructureを構築する為のリソース要件を取得(infoに入る)
	dx->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	//リソース要件を元にUAV作成
	AccelerationStructureBuffers buffers;
	dx->createDefaultResourceBuffer_UNORDERED_ACCESS(buffers.pScratch.GetAddressOf(), info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dx->createDefaultResourceBuffer_UNORDERED_ACCESS(buffers.pResult.GetAddressOf(), info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	//bottom-level AS作成, 第三引数は作成後の情報, 不要の場合nullptr
	dx->dx_sub[comNo].mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	//バリアしたリソースへのUAVアクセスに於いて次のUAVアクセス開始前に現在のUAVアクセスが終了する必要がある事を示す
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();

	dx->dx_sub[comNo].mCommandList->ResourceBarrier(1, &uavBarrier);

	return buffers;
}

void DXR_Basic::createTopLevelAS(int comNo, uint64_t& tlasSize, bool update) {

	Dx12Process* dx = Dx12Process::GetInstance();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = numInstance;
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
		dx->createUploadResource(topLevelBuffers.pInstanceDesc.GetAddressOf(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstance);
		tlasSize = info.ResultDataMaxSizeInBytes;
	}

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	topLevelBuffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
	ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstance);

	for (uint32_t i = 0; i < numInstance; i++) {
		//InstanceDesc初期化
		pInstanceDesc[i].InstanceID = i;//この値は、InstanceID()を介してシェーダーに公開されます
		pInstanceDesc[i].InstanceContributionToHitGroupIndex = 0;//シェーダーテーブル内のオフセット。ジオメトリが一つの場合,オフセット0
		pInstanceDesc[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		MatrixTranspose(&Transform[i]);
		memcpy(pInstanceDesc[i].Transform, &Transform[i], sizeof(pInstanceDesc[i].Transform));
		pInstanceDesc[i].AccelerationStructure = bottomLevelBuffers[i].pResult.Get()->GetGPUVirtualAddress();
		pInstanceDesc[i].InstanceMask = 0xFF;
	}

	topLevelBuffers.pInstanceDesc->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = topLevelBuffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = topLevelBuffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = topLevelBuffers.pScratch->GetGPUVirtualAddress();

	if (update) {
		asDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		asDesc.SourceAccelerationStructureData = topLevelBuffers.pResult->GetGPUVirtualAddress();
	}

	dx->dx_sub[comNo].mCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = topLevelBuffers.pResult.Get();
	dx->dx_sub[comNo].mCommandList->ResourceBarrier(1, &uavBarrier);
}

void DXR_Basic::createAccelerationStructures(int comNo) {

	bottomLevelBuffers = std::make_unique<AccelerationStructureBuffers[]>(numInstance);
	mpBottomLevelAS = std::make_unique<ComPtr<ID3D12Resource>[]>(numInstance);
	Transform = std::make_unique<MATRIX[]>(numInstance);

	for (UINT i = 0; i < numInstance; i++) {
		bottomLevelBuffers[i] = createBottomLevelAS(comNo, i);
		MatrixIdentity(&Transform[i]);
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
	addChar dxrShader;
	dxrShader.addStr(add_shader, ShaderBasicDXR);
	//シェーダーのコンパイル
	ComPtr<ID3DBlob> pDxilLib = CompileLibrary(dxrShader.str, L"ShaderBasicDXR", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kCamMissShader, kCamClosestHitShader, kMetallicMiss, kMetallicHitShader };
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

static RootSignatureDesc createRayGenRootDesc(UINT numInstance) {

	RootSignatureDesc desc = {};
	int numDescriptorRanges = 6;
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
	desc.range[2].NumDescriptors = numInstance;
	desc.range[2].RegisterSpace = 1;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numInstance;

	//Vertex(t2)
	desc.range[3].BaseShaderRegister = 2;
	desc.range[3].NumDescriptors = numInstance;
	desc.range[3].RegisterSpace = 2;
	desc.range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[3].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numInstance;

	//cbuffer global(b0)
	desc.range[4].BaseShaderRegister = 0;
	desc.range[4].NumDescriptors = 1;
	desc.range[4].RegisterSpace = 0;
	desc.range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[4].OffsetInDescriptorsFromTableStart = descCnt++;

	//world(b1)
	desc.range[5].BaseShaderRegister = 1;
	desc.range[5].NumDescriptors = numInstance;
	desc.range[5].RegisterSpace = 3;
	desc.range[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	desc.range[5].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numInstance;

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

static RootSignatureDesc createGlobalRootDesc(UINT numInstance) {

	RootSignatureDesc desc = {};
	int numDescriptorRanges = 3;
	desc.range.resize(numDescriptorRanges);
	UINT descCnt = 0;
	//g_texDiffuse(t0)
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = numInstance;
	desc.range[0].RegisterSpace = 10;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numInstance;

	//g_texNormal(t1)
	desc.range[1].BaseShaderRegister = 1;
	desc.range[1].NumDescriptors = numInstance;
	desc.range[1].RegisterSpace = 11;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = descCnt;
	descCnt += numInstance;

	//g_samLinear(s0)
	desc.range[2].BaseShaderRegister = 0;
	desc.range[2].NumDescriptors = 1;
	desc.range[2].RegisterSpace = 12;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	desc.range[2].OffsetInDescriptorsFromTableStart = 0;

	desc.rootParams.resize(2);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();
	desc.rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[1].DescriptorTable.pDescriptorRanges = &desc.range[2];
	desc.rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	desc.desc.NumParameters = 2;
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
	DxilLibrary dxilLib = createDxilLibrary(dx->ShaderNormalTangentCopy.get());//Dx12Processに保持してるshaderを入力
	subobjects[index++] = dxilLib.stateSubobject;

	//closesthitShader SUBOBJECT作成
	HitProgram hitProgram(nullptr, kCamClosestHitShader, kCamHitGroup);
	subobjects[index++] = hitProgram.subObject;

	//shadowhitShader SUBOBJECT作成
	HitProgram shadowHitProgram(nullptr, kMetallicHitShader, kMetallicHitGroup);
	subobjects[index++] = shadowHitProgram.subObject;

	//raygenerationShaderルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature rgsRootSignature(createRootSignature(createRayGenRootDesc(numInstance).desc));
	subobjects[index] = rgsRootSignature.subobject;

	//raygenerationShaderと↑のルートシグネチャ関連付け, SUBOBJECT作成
	uint32_t rgsRootIndex = index++;
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject;

	//closesthitShader, missShader ルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature hitMissRootSignature(createRootSignature(createRayGenRootDesc(numInstance).desc));
	subobjects[index] = hitMissRootSignature.subobject;

	//↑のclosesthitShader, missShader 共有ルートシグネチャとclosesthitShader, missShader関連付け, SUBOBJECT作成
	uint32_t hitMissRootIndex = index++;
	const WCHAR* missHitExportName[] = { kCamMissShader, kCamClosestHitShader };
	ExportAssociation missHitRootAssociation(missHitExportName, ARRAY_SIZE(missHitExportName), &(subobjects[hitMissRootIndex]));
	subobjects[index++] = missHitRootAssociation.subobject;

	//kShadowChs, kShadowMiss ルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature emptyRootSignature(createRootSignature(createRayGenRootDesc(numInstance).desc));
	subobjects[index] = emptyRootSignature.subobject;

	uint32_t shadowRootIndex = index++;
	const WCHAR* shadowRootExport[] = { kMetallicHitShader, kMetallicMiss };
	ExportAssociation shadowRootAssociation(shadowRootExport, ARRAY_SIZE(shadowRootExport), &(subobjects[shadowRootIndex]));
	subobjects[index++] = shadowRootAssociation.subobject;

	//ペイロードサイズをプログラムにバインドする SUBOBJECT作成
	uint32_t MaxAttributeSizeInBytes = sizeof(float) * 2;
	uint32_t maxPayloadSizeInBytes = sizeof(float) * 4;
	ShaderConfig shaderConfig(MaxAttributeSizeInBytes, maxPayloadSizeInBytes);
	subobjects[index] = shaderConfig.subobject;

	//ShaderConfigと全てのシェーダー関連付け SUBOBJECT作成
	uint32_t shaderConfigIndex = index++;
	const WCHAR* shaderExports[] = { kCamMissShader, kCamClosestHitShader, kRayGenShader, kMetallicHitShader, kMetallicMiss };
	ExportAssociation configAssociation(shaderExports, ARRAY_SIZE(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject;

	//パイプラインコンフィグ作成 SUBOBJECT作成
	PipelineConfig config(2);
	subobjects[index++] = config.subobject;

	//グローバルルートシグネチャ作成 SUBOBJECT作成
	GlobalRootSignature root(createRootSignature(createGlobalRootDesc(numInstance).desc));
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

void DXR_Basic::createShaderResources(ID3D12Resource** difTexture, ID3D12Resource** norTexture) {

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
	int num_t1 = numInstance;
	int num_t2 = numInstance;
	int num_b0 = 1;
	int num_b1 = numInstance;
	numSkipLocalHeap = num_u0 + num_t0 + num_t1 + num_t2 + num_b0 + num_b1;
	//Global
	int num_t3 = numInstance;
	int num_t4 = numInstance;
	int numHeap = numSkipLocalHeap + num_t3 + num_t4;

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
	for (UINT i = 0; i < numInstance; i++) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1 = {};
		srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc1.Buffer.NumElements = pIndexBuffer[i].IndexCount;
		srvDesc1.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc1.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc1.Buffer.StructureByteStride = sizeof(uint32_t);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateShaderResourceView(pIndexBuffer[i].IndexBufferGPU.Get(), &srvDesc1, srvHandle);
	}

	//SRVを作成 Vertex(t2)
	for (UINT i = 0; i < numInstance; i++) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
		srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc2.Buffer.NumElements = pVertexBuffer[i].VertexBufferByteSize / pVertexBuffer[i].VertexByteStride;
		srvDesc2.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc2.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc2.Buffer.StructureByteStride = pVertexBuffer[i].VertexByteStride;
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateShaderResourceView(pVertexBuffer[i].VertexBufferGPU.Get(), &srvDesc2, srvHandle);
	}

	//CBVを作成 cbuffer global(b0)
	D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
	bufferDesc.SizeInBytes = sCB->getSizeInBytes();
	bufferDesc.BufferLocation = sCB->Resource()->GetGPUVirtualAddress();
	srvHandle.ptr += offsetSize;
	dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);

	//CBVを作成 world(b1)
	for (UINT i = 0; i < numInstance; i++) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
		bufferDesc.SizeInBytes = world->getElementByteSize();
		bufferDesc.BufferLocation = world->getGPUVirtualAddress(i);
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateConstantBufferView(&bufferDesc, srvHandle);
	}

	//Global
	//SRVを作成 g_texDiffuse(t3)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3 = {};
	for (UINT i = 0; i < numInstance; i++) {
		srvDesc3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc3.Format = difTexture[i]->GetDesc().Format;
		srvDesc3.Texture2D.MostDetailedMip = 0;
		srvDesc3.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc3.Texture2D.MipLevels = difTexture[i]->GetDesc().MipLevels;
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateShaderResourceView(difTexture[i], &srvDesc3, srvHandle);
	}

	//SRVを作成 g_texNormal(t4)
	for (UINT i = 0; i < numInstance; i++) {
		srvDesc3.Format = norTexture[i]->GetDesc().Format;
		srvDesc3.Texture2D.MipLevels = norTexture[i]->GetDesc().MipLevels;
		srvHandle.ptr += offsetSize;
		dx->getDevice()->CreateShaderResourceView(norTexture[i], &srvDesc3, srvHandle);
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

	//Entry 1 - miss program
	memcpy(pData + mShaderTableEntrySize, pRtsoProps.Get()->GetShaderIdentifier(kCamMissShader),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - shadow miss program
	memcpy(pData + mShaderTableEntrySize * 2, pRtsoProps->GetShaderIdentifier(kMetallicMiss),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 3 - hit program
	uint8_t* pHitEntry = pData + mShaderTableEntrySize * 3;
	memcpy(pHitEntry, pRtsoProps.Get()->GetShaderIdentifier(kCamHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	//Entry 4 - shadow hit program
	uint8_t* pHitEntry1 = pData + mShaderTableEntrySize * 4;
	memcpy(pHitEntry1, pRtsoProps.Get()->GetShaderIdentifier(kMetallicHitGroup),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	mpShaderTable.Get()->Unmap(0, nullptr);
}

void DXR_Basic::updateTransform(UINT InstanceNo, VECTOR3 pos, VECTOR3 theta, VECTOR3 size) {
	MATRIX mov;
	MATRIX rotZ, rotY, rotX, rotZY, rotZYX;
	MATRIX scale;
	MATRIX scro;

	//拡大縮小
	MatrixScaling(&scale, size.x, size.y, size.z);
	//表示位置
	MatrixRotationZ(&rotZ, theta.z);
	MatrixRotationY(&rotY, theta.y);
	MatrixRotationX(&rotX, theta.x);
	MatrixMultiply(&rotZY, &rotZ, &rotY);
	MatrixMultiply(&rotZYX, &rotZY, &rotX);
	MatrixTranslation(&mov, pos.x, pos.y, pos.z);
	MatrixMultiply(&scro, &rotZYX, &scale);
	MatrixMultiply(&Transform[InstanceNo], &scro, &mov);
}

void DXR_Basic::setCB() {
	Dx12Process* dx = Dx12Process::GetInstance();
	MatrixInverse(&cb.projectionInv, &dx->mProj);
	MatrixInverse(&cb.viewInv, &dx->mView);
	cb.cameraUp.as(dx->upX, dx->upY, dx->upZ, 1.0f);
	cb.cameraPosition.as(dx->posX, dx->posY, dx->posZ, 1.0f);
	memcpy(cb.lightPosition, dx->plight.LightPos, sizeof(VECTOR4) * LIGHT_PCS);
	cb.lightAmbientColor.as(0.2f, 0.2f, 0.2f, 0.0f);//後で変える
	cb.lightDiffuseColor.as(0.5f, 0.5f, 0.5f, 1.0f);//後で変える
	sCB->CopyData(0, cb);
	for (UINT i = 0; i < numInstance; i++) {
		DxrTransformCB tcb = {};
		memcpy(&tcb, &Transform[i], sizeof(DxrTransformCB));
		world->CopyData(i, tcb);
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

	for (UINT i = 0; i < numInstance; i++)
		createTopLevelAS(comNo, mTlasSize, true);

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