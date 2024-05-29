//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　          DxrRenderer.cpp                                   **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DxrRenderer.h"
#include <fstream>
#include <sstream>
#include "./ShaderDXR/ShaderBasicAnyHit.h"
#include "./ShaderDXR/ShaderBasicHit.h"
#include "./ShaderDXR/ShaderBasicMiss.h"
#include "./ShaderDXR/ShaderCommon.h"
#include "./ShaderDXR/ShaderEmissiveHit.h"
#include "./ShaderDXR/ShaderEmissiveMiss.h"
#include "./ShaderDXR/ShaderGlobalParameters.h"
#include "./ShaderDXR/ShaderLocalParameters.h"
#include "./ShaderDXR/ShaderRayGen.h"
#include "./ShaderDXR/ShaderTraceRay.h"
#include "../Core/Dx_Light.h"

namespace {
	dxc::DxcDllSupport gDxcDllHelper;

	const WCHAR* kRayGenShaderArr[] = {
		L"rayGen",
		L"rayGen_InstanceIdMapTest",
		L"rayGen"
	};
	WCHAR kRayGenShader[255] = {};

	const WCHAR* kBasicClosestHitShaderArr[] = {
		L"basicHit",
		L"basicHit",
		L"basicHit_normalMapTest"
	};
	WCHAR kBasicClosestHitShader[255] = {};

	const WCHAR* kBasicAnyHitShader = L"anyBasicHit";
	const WCHAR* kBasicMissShader = L"basicMiss";
	const WCHAR* kBasicHitGroup = L"basicHitGroup";

	const WCHAR* kEmissiveMiss = L"emissiveMiss";
	const WCHAR* kEmissiveHitShader = L"emissiveHit";
	const WCHAR* kEmissiveHitGroup = L"emissiveHitGroup";

	ComPtr<ID3DBlob> CompileLibrary(char* shaderByte, const WCHAR* filename, const WCHAR* targetString) {

		//コンパイルライブラリ初期化
		HRESULT hr_Hel = gDxcDllHelper.Initialize();
		if (FAILED(hr_Hel)) {
			Dx_Util::ErrorMessage("DXR CompileLibrary gDxcDllHelper.Initialize() Error");
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
			Dx_Util::ErrorMessage("DXR CompileLibrary CreateBlobWithEncodingFromPinned Error");
		}
		//Compile
		ComPtr<IDxcOperationResult> pResult;
		HRESULT hr_Com = pCompiler->Compile(pTextBlob.Get(), filename, L"", targetString,
			nullptr, 0, nullptr, 0, nullptr, pResult.GetAddressOf());
		if (FAILED(hr_Com)) {
			Dx_Util::ErrorMessage("DXR CompileLibrary Compile Error");
		}

		//Compile結果
		HRESULT resultCode;
		pResult.Get()->GetStatus(&resultCode);
		if (FAILED(resultCode))
		{
			ComPtr<IDxcBlobEncoding> pError;
			pResult.Get()->GetErrorBuffer(pError.GetAddressOf());
			Dx_Util::ErrorMessage("DXR compileLibrary Error");
			Dx_Util::ErrorMessage((char*)pError.Get()->GetBufferPointer());
			return nullptr;
		}

		ComPtr<IDxcBlob> pBlob;
		pResult->GetResult(pBlob.GetAddressOf());
		ComPtr<ID3DBlob> retBlob;
		pBlob.Get()->QueryInterface(IID_PPV_ARGS(retBlob.GetAddressOf()));
		return retBlob;
	}

	struct DxilLibrary {
		DxilLibrary(ComPtr<ID3DBlob> pBlob, const WCHAR* entryPoint) : pShaderBlob(pBlob)
		{
			stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			stateSubobject.pDesc = &dxilLibDesc;

			dxilLibDesc = {};
			if (pBlob)
			{
				dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
				dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
				dxilLibDesc.NumExports = 1;
				dxilLibDesc.pExports = &exportDesc;

				exportName = entryPoint;
				exportDesc.Name = exportName.c_str();
				exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc.ExportToRename = nullptr;
			}
		};

		D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
		D3D12_STATE_SUBOBJECT stateSubobject{};
		ComPtr<ID3DBlob> pShaderBlob;
		D3D12_EXPORT_DESC exportDesc;
		std::wstring exportName;
	};

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

	RootSignatureDesc createGlobalRootDesc() {
		RootSignatureDesc desc = {};
		int numDescriptorRanges = 2;
		desc.range.resize(numDescriptorRanges);

		//cbuffer global(b0)
		desc.range[0].BaseShaderRegister = 0;
		desc.range[0].NumDescriptors = 1;
		desc.range[0].RegisterSpace = 0;
		desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		desc.range[0].OffsetInDescriptorsFromTableStart = 0;

		//g_samLinear(s0)
		desc.range[1].BaseShaderRegister = 0;
		desc.range[1].NumDescriptors = 1;
		desc.range[1].RegisterSpace = 14;
		desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		desc.range[1].OffsetInDescriptorsFromTableStart = 0;

		desc.rootParams.resize(3);
		desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		desc.rootParams[0].DescriptorTable.pDescriptorRanges = &desc.range[0];
		desc.rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		desc.rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		desc.rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		desc.rootParams[1].DescriptorTable.pDescriptorRanges = &desc.range[1];
		desc.rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		//gRtScene(t4)
		desc.rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		desc.rootParams[2].Descriptor.ShaderRegister = 4;
		desc.rootParams[2].Descriptor.RegisterSpace = 15;
		desc.rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		desc.desc.NumParameters = (UINT)desc.rootParams.size();
		desc.desc.pParameters = desc.rootParams.data();
		desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		return desc;
	}

	RootSignatureDesc createLocalRootDescRayGen() {
		RootSignatureDesc desc = {};
		int numDescriptorRanges = 3;
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

		//gInstanceIdMap(u2)
		desc.range[2].BaseShaderRegister = 2;
		desc.range[2].NumDescriptors = 1;
		desc.range[2].RegisterSpace = 0;
		desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		desc.range[2].OffsetInDescriptorsFromTableStart = descCnt++;

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

	RootSignatureDesc createLocalRootDescHit(UINT numMaterial, UINT numInstancing) {
		RootSignatureDesc desc = {};
		int numDescriptorRanges = 7;
		desc.range.resize(numDescriptorRanges);
		UINT descCnt = 0;

		//Indices(t0)
		desc.range[0].BaseShaderRegister = 0;
		desc.range[0].NumDescriptors = numMaterial;
		desc.range[0].RegisterSpace = 1;
		desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		desc.range[0].OffsetInDescriptorsFromTableStart = descCnt;
		descCnt += numMaterial;

		//material(b1)
		desc.range[1].BaseShaderRegister = 1;
		desc.range[1].NumDescriptors = numMaterial;
		desc.range[1].RegisterSpace = 3;
		desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		desc.range[1].OffsetInDescriptorsFromTableStart = descCnt;
		descCnt += numMaterial;

		//wvp(b2)
		desc.range[2].BaseShaderRegister = 2;
		desc.range[2].NumDescriptors = numInstancing;
		desc.range[2].RegisterSpace = 4;
		desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		desc.range[2].OffsetInDescriptorsFromTableStart = descCnt;
		descCnt += numInstancing;

		//g_texDiffuse(t0)
		desc.range[3].BaseShaderRegister = 0;
		desc.range[3].NumDescriptors = numMaterial;
		desc.range[3].RegisterSpace = 10;
		desc.range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		desc.range[3].OffsetInDescriptorsFromTableStart = descCnt;
		descCnt += numMaterial;

		//g_texNormal(t1)
		desc.range[4].BaseShaderRegister = 1;
		desc.range[4].NumDescriptors = numMaterial;
		desc.range[4].RegisterSpace = 11;
		desc.range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		desc.range[4].OffsetInDescriptorsFromTableStart = descCnt;
		descCnt += numMaterial;

		//g_texSpecular(t2)
		desc.range[5].BaseShaderRegister = 2;
		desc.range[5].NumDescriptors = numMaterial;
		desc.range[5].RegisterSpace = 12;
		desc.range[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		desc.range[5].OffsetInDescriptorsFromTableStart = descCnt;
		descCnt += numMaterial;

		//Vertex(t3)
		desc.range[6].BaseShaderRegister = 3;
		desc.range[6].NumDescriptors = numMaterial;
		desc.range[6].RegisterSpace = 13;
		desc.range[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
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

	UINT numCreateMaterial(ParameterDXR* pd) {
		if (pd->hs) {
			return pd->NumMaterial * pd->NumMaxInstance;
		}
		return pd->NumMaterial;
	}

	UINT numMaterialMaxInstance(ParameterDXR* pd) {
		if (pd->hs) {
			return pd->NumMaxInstance;
		}
		return 1;
	}
}

void DxrRenderer::initDXR(std::vector<ParameterDXR*>& pd, UINT MaxRecursion, ShaderTestMode Mode) {

	PD = pd;
	TMin = 0.001f;
	TMax = 10000.0f;

	Dx_CommandManager* cMa = Dx_CommandManager::GetInstance();
	Dx_CommandListObj* cObj = cMa->getGraphicsComListObj(0);

	cObj->Bigin();
	maxRecursion = MaxRecursion;
	numMaterial = 0;
	maxNumInstancing = 0;
	for (auto i = 0; i < PD.size(); i++) {
		numMaterial += numCreateMaterial(PD[i]);
		maxNumInstancing += PD[i]->NumMaxInstance;
	}

	wvp = NEW ConstantBuffer<WVP_CB>(maxNumInstancing);
	cbObj[0].wvpCb = std::make_unique<WVP_CB[]>(maxNumInstancing);
	cbObj[1].wvpCb = std::make_unique<WVP_CB[]>(maxNumInstancing);
	material = NEW ConstantBuffer<DxrMaterialCB>(numMaterial);
	cbObj[0].matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
	cbObj[1].matCb = std::make_unique<DxrMaterialCB[]>(numMaterial);
	sCB = NEW ConstantBuffer<DxrConstantBuffer>(1);

	createAccelerationStructures();

	int numIns = 0;
	for (auto i = 0; i < PD.size(); i++) {
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < numMaterialMaxInstance(PD[i]); t++) {
				cbObj[0].matCb[numIns].materialNo = PD[i]->mType[j];
				cbObj[1].matCb[numIns].materialNo = PD[i]->mType[j];
				numIns++;
			}
		}
	}
	cbObj[0].cb.numEmissive.y = (float)maxNumInstancing;
	cbObj[1].cb.numEmissive.y = (float)maxNumInstancing;

	createRtPipelineState(Mode);
	createShaderResources();
	createShaderTable();
	cObj->End();
	cMa->RunGpu();
	cMa->WaitFence();
}

void DxrRenderer::setTMin_TMax(float tMin, float tMax) {
	TMin = tMin;
	TMax = tMax;
}

void DxrRenderer::createBottomLevelAS1(Dx_CommandListObj* com, VertexView* vv,
	IndexView* iv, UINT currentIndexCount, UINT MaterialNo,
	bool update, bool tessellation, bool alphaTest) {

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	if (!update)buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	AccelerationStructureBuffers& bLB = asObj[buffSwap[0]].bottomLevelBuffers[MaterialNo];

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexBuffer.StartAddress = vv->VertexBufferGPU.getResource()->GetGPUVirtualAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = vv->VertexByteStride;
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	UINT numVertices = vv->VertexBufferByteSize / vv->VertexByteStride;
	geomDesc.Triangles.VertexCount = numVertices;
	geomDesc.Triangles.IndexBuffer = iv->IndexBufferGPU.getResource()->GetGPUVirtualAddress();
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
	Dx_Device* device = Dx_Device::GetInstance();
	device->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (!bLB.firstSet) {
		//リソース要件を元にUAV作成
		device->createDefaultResourceBuffer_UNORDERED_ACCESS(bLB.pScratch.GetAddressOf(), info.ScratchDataSizeInBytes);
		device->createDefaultResourceBuffer_UNORDERED_ACCESS(bLB.pResult.GetAddressOf(), info.ResultDataMaxSizeInBytes,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		com->ResourceBarrier(bLB.pScratch.Get(), D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = bLB.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = bLB.pScratch->GetGPUVirtualAddress();
	asDesc.SourceAccelerationStructureData = 0;//PERFORM_UPDATE以外は0にする

	if (bLB.firstSet) {
		if (!tessellation && update) {
			asDesc.Inputs.Flags = buildFlag |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			asDesc.SourceAccelerationStructureData = bLB.pResult->GetGPUVirtualAddress();
		}
	}
	else {
		bLB.firstSet = true;
	}

	//bottom-level AS作成, 第三引数は作成後の情報, 不要の場合nullptr
	com->getCommandList()->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	com->delayUavResourceBarrier(bLB.pResult.Get());
}

void DxrRenderer::createBottomLevelAS(Dx_CommandListObj* com) {
	Dx_Device* dev = Dx_Device::GetInstance();
	for (UINT i = 0; i < numMaterial; i++) {
		AccelerationStructureBuffers& bLB = asObj[buffSwap[0]].bottomLevelBuffers[i];
		if (bLB.firstSet)com->delayUavResourceBarrier(bLB.pResult.Get());
	}
	com->RunDelayUavResourceBarrier();

	UINT MaterialCnt = 0;
	for (auto i = 0; i < PD.size(); i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dev->dxrBuffSwapRaytraceIndex()];
		bool createAS = ud.createAS;
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < numMaterialMaxInstance(PD[i]); t++) {
				VertexView* vv = &ud.VviewDXR[j][t];
				IndexView* iv = &PD[i]->IviewDXR[j];
				UINT indexCnt = ud.currentIndexCount[j][t];
				if (ud.firstSet && (PD[i]->updateF || !ud.createAS)) {
					createBottomLevelAS1(com, vv, iv, indexCnt, MaterialCnt,
						PD[i]->updateF, PD[i]->tessellationF, PD[i]->alphaTest);
					createAS = true;
				}
				MaterialCnt++;
			}
		}
		ud.createAS = createAS;
	}
	com->RunDelayUavResourceBarrier();
}

void DxrRenderer::createTopLevelAS(Dx_CommandListObj* com) {

	Dx_Device* dev = Dx_Device::GetInstance();
	AccelerationStructureBuffers& tLB = asObj[buffSwap[0]].topLevelBuffers;

	if (tLB.firstSet) {
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = tLB.pResult.Get();
		com->getCommandList()->ResourceBarrier(1, &uavBarrier);
	}

	UINT numRayInstance = 0;
	for (auto i = 0; i < PD.size(); i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dev->dxrBuffSwapRaytraceIndex()];
		if (tLB.firstSet)
			numRayInstance += ud.NumInstance * PD[i]->NumMaterial;
		else
			numRayInstance += PD[i]->NumMaxInstance * PD[i]->NumMaterial;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = numRayInstance;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	//AccelerationStructureを構築する為のリソース要件を取得(infoに入る)
	Dx_Device* device = Dx_Device::GetInstance();
	device->getDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (!tLB.firstSet) {
		//リソース要件を元にUAV作成
		device->createDefaultResourceBuffer_UNORDERED_ACCESS(tLB.pScratch.GetAddressOf(),
			info.ScratchDataSizeInBytes);
		device->createDefaultResourceBuffer_UNORDERED_ACCESS(tLB.pResult.GetAddressOf(),
			info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
		device->createUploadResource(tLB.pInstanceDesc.GetAddressOf(),
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numRayInstance);

		com->ResourceBarrier(tLB.pScratch.Get(), D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc = {};
	tLB.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
	ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numRayInstance);

	UINT RayInstanceCnt = 0;
	UINT materialCnt = 0;
	UINT InstancingCnt = 0;
	for (auto i = 0; i < PD.size(); i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dev->dxrBuffSwapRaytraceIndex()];
		UINT udInstanceIDCnt = 0;
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < ud.NumInstance; k++) {
				if (ud.NumInstance > PD[i]->NumMaxInstance) {
					Dx_Util::ErrorMessage("NumInstance is greater than NumMaxInstanc.");
				}
				if (ud.firstSet) {
					UINT matAddInd = 0;
					if (PD[i]->hs)matAddInd = k;
					D3D12_RAYTRACING_INSTANCE_DESC& pID = pInstanceDesc[RayInstanceCnt];
					UINT materialInstanceID = materialCnt + matAddInd;//max65535
					UINT InstancingID = InstancingCnt + k;//max65535
					UINT InstanceID = ((materialInstanceID << 16) & 0xffff0000) | (InstancingID & 0x0000ffff);
					ud.InstanceID[udInstanceIDCnt + k] = InstanceID;
					pID.InstanceID = InstanceID;//この値は、InstanceID()を介してシェーダーに公開されます
					pID.InstanceContributionToHitGroupIndex = 0;//使用するhitShaderインデックス
					pID.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
					memcpy(pID.Transform, &ud.Transform[k], sizeof(pID.Transform));
					pID.AccelerationStructure = asObj[buffSwap[0]].bottomLevelBuffers[materialCnt + matAddInd].pResult.Get()->GetGPUVirtualAddress();
					pID.InstanceMask = ud.InstanceMask;
				}
				RayInstanceCnt++;
			}
			udInstanceIDCnt += PD[i]->NumMaxInstance;
			materialCnt += numMaterialMaxInstance(PD[i]);
		}
		InstancingCnt += PD[i]->NumMaxInstance;
	}

	tLB.pInstanceDesc->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = tLB.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = tLB.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = tLB.pScratch->GetGPUVirtualAddress();
	asDesc.SourceAccelerationStructureData = 0;

	if (!tLB.firstSet) {
		tLB.firstSet = true;
		asObj[buffSwap[0]].mpTopLevelAS = tLB.pResult;
	}

	com->getCommandList()->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = tLB.pResult.Get();
	com->getCommandList()->ResourceBarrier(1, &uavBarrier);
}

void DxrRenderer::createAccelerationStructures() {
	asObj[0].bottomLevelBuffers = std::make_unique<AccelerationStructureBuffers[]>(numMaterial);
	asObj[1].bottomLevelBuffers = std::make_unique<AccelerationStructureBuffers[]>(numMaterial);
}

ComPtr<ID3D12RootSignature> DxrRenderer::createRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc) {
	return Dx_Device::GetInstance()->CreateRsCommon(&desc);
}

void DxrRenderer::createRtPipelineState(ShaderTestMode Mode) {

	wcscpy(kRayGenShader, kRayGenShaderArr[Mode]);
	wcscpy(kBasicClosestHitShader, kBasicClosestHitShaderArr[Mode]);

	//CreateStateObject作成に必要な各D3D12_STATE_SUBOBJECTを作成
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	//↓配列内に同じ配列内のアドレスを保持してる要素有りの場合事前にreserveをしておく(arrayの方が良かったかも・・)
	subobjects.reserve(30);//後々増やした場合の為に多めに

	//Shader文字列組み合わせ
	addChar calcLight = {};
	calcLight.addStr(Dx_ShaderHolder::ShaderNormalTangentCopy.get(), Dx_ShaderHolder::ShaderCalculateLightingCopy.get());

	addChar para = {};
	para.addStr(ShaderGlobalParameters, ShaderLocalParameters);

	addChar com_t = {};
	com_t.addStr(calcLight.str, ShaderCommon);

	addChar com = {};
	com.addStr(para.str, com_t.str);

	addChar tRay = {};
	tRay.addStr(com.str, ShaderTraceRay);

	addChar rayGen = {};
	rayGen.addStr(com.str, ShaderRayGen);

	addChar any = {};
	any.addStr(com.str, ShaderBasicAnyHit);

	addChar bHit = {};
	bHit.addStr(tRay.str, ShaderBasicHit);

	addChar bMis = {};
	bMis.addStr(ShaderGlobalParameters, ShaderBasicMiss);

	addChar eHit = {};
	eHit.addStr(com.str, ShaderEmissiveHit);

	addChar eMis = {};
	eMis.addStr(ShaderGlobalParameters, ShaderEmissiveMiss);

	//DXIL library 初期化, SUBOBJECT作成
	DxilLibrary Ray(CompileLibrary(rayGen.str, L"rayGen", L"lib_6_3"), kRayGenShader);
	DxilLibrary Any(CompileLibrary(any.str, L"any", L"lib_6_3"), kBasicAnyHitShader);
	DxilLibrary Bhit(CompileLibrary(bHit.str, L"bHit", L"lib_6_3"), kBasicClosestHitShader);
	DxilLibrary Bmis(CompileLibrary(bMis.str, L"bMis", L"lib_6_3"), kBasicMissShader);
	DxilLibrary Ehit(CompileLibrary(eHit.str, L"eHit", L"lib_6_3"), kEmissiveHitShader);
	DxilLibrary Emis(CompileLibrary(eMis.str, L"eMis", L"lib_6_3"), kEmissiveMiss);

	subobjects.push_back(Ray.stateSubobject);
	subobjects.push_back(Any.stateSubobject);
	subobjects.push_back(Bhit.stateSubobject);
	subobjects.push_back(Bmis.stateSubobject);
	subobjects.push_back(Ehit.stateSubobject);
	subobjects.push_back(Emis.stateSubobject);

	//BasicHitShader SUBOBJECT作成
	HitProgram basicHitProgram(kBasicAnyHitShader, kBasicClosestHitShader, kBasicHitGroup);
	subobjects.push_back(basicHitProgram.subObject);

	//EmissiveHitShader SUBOBJECT作成
	HitProgram emissiveHitProgram(nullptr, kEmissiveHitShader, kEmissiveHitGroup);
	subobjects.push_back(emissiveHitProgram.subObject);

	//RayGen: ローカルルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature loRootSignatureRay(createRootSignature(createLocalRootDescRayGen().desc));
	subobjects.push_back(loRootSignatureRay.subobject);
	D3D12_STATE_SUBOBJECT* p_rsigRay = &subobjects[subobjects.size() - 1];

	//RayGen: シェーダーとローカルルートシグネチャ関連付け
	const WCHAR* ExportNameRay[] =
	{ kRayGenShader };
	ExportAssociation loRootAssociationRay(ExportNameRay, ARRAY_SIZE(ExportNameRay), p_rsigRay);
	subobjects.push_back(loRootAssociationRay.subobject);

	//Hit: ローカルルートシグネチャ作成, SUBOBJECT作成
	LocalRootSignature loRootSignatureHit(createRootSignature(createLocalRootDescHit(numMaterial, maxNumInstancing).desc));
	subobjects.push_back(loRootSignatureHit.subobject);
	D3D12_STATE_SUBOBJECT* p_rsigHit = &subobjects[subobjects.size() - 1];

	//Hit: シェーダーとローカルルートシグネチャ関連付け
	const WCHAR* ExportNameHit[] =
	{
	 kBasicAnyHitShader, kBasicClosestHitShader,
	 kEmissiveHitShader
	};
	ExportAssociation loRootAssociationHit(ExportNameHit, ARRAY_SIZE(ExportNameHit), p_rsigHit);
	subobjects.push_back(loRootAssociationHit.subobject);

	//ペイロードサイズをプログラムにバインドする SUBOBJECT作成
	uint32_t MaxAttributeSizeInBytes = sizeof(float) * 2;
	uint32_t maxPayloadSizeInBytes = sizeof(float) * 14;
	ShaderConfig shaderConfig(MaxAttributeSizeInBytes, maxPayloadSizeInBytes);
	subobjects.push_back(shaderConfig.subobject);
	D3D12_STATE_SUBOBJECT* p_conf = &subobjects[subobjects.size() - 1];

	//ShaderConfigと全てのシェーダー関連付け SUBOBJECT作成
	const WCHAR* shaderExports[] = {
		kRayGenShader,
		kBasicAnyHitShader, kBasicMissShader, kBasicClosestHitShader,
		kEmissiveHitShader, kEmissiveMiss
	};
	ExportAssociation configAssociation(shaderExports, ARRAY_SIZE(shaderExports), p_conf);
	subobjects.push_back(configAssociation.subobject);

	//パイプラインコンフィグ作成 SUBOBJECT作成
	PipelineConfig config(maxRecursion);
	subobjects.push_back(config.subobject);

	//グローバルルートシグネチャ作成 SUBOBJECT作成
	GlobalRootSignature root(createRootSignature(createGlobalRootDesc().desc));
	mpGlobalRootSig = root.pRootSig;
	subobjects.push_back(root.subobject);

	D3D12_STATE_OBJECT_DESC desc = {};
	desc.NumSubobjects = (UINT)subobjects.size();
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	HRESULT hr = Dx_Device::GetInstance()->getDevice()->CreateStateObject(&desc, IID_PPV_ARGS(mpPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		Dx_Util::ErrorMessage("DxrRenderer::createRtPipelineState() Error");
	}
}

void DxrRenderer::createShaderResources() {

	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	Dx_Device* device = Dx_Device::GetInstance();
	mpOutputResource.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(sw->getClientWidth(), sw->getClientHeight());

	mpDepthResource.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(sw->getClientWidth(), sw->getClientHeight(),
		D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT_R32_FLOAT);

	mpInstanceIdMapResource.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(sw->getClientWidth(), sw->getClientHeight(),
		D3D12_RESOURCE_STATE_COMMON,
		DXGI_FORMAT_R32_FLOAT);

	mpOutputResource.ResourceBarrier(0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	mpDepthResource.ResourceBarrier(0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	mpInstanceIdMapResource.ResourceBarrier(0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//Local
	int num_u0 = 1;
	int num_u1 = 1;
	int num_u2 = 1;
	int numLocalHeapRay = num_u0 + num_u1 + num_u2;
	int num_t0 = numMaterial;
	int num_b1 = numMaterial;
	int num_b2 = maxNumInstancing;
	int num_t00 = numMaterial;
	int num_t01 = numMaterial;
	int num_t02 = numMaterial;
	int num_t03 = numMaterial;
	int numLocalHeapHit = num_t0 + num_b1 + num_b2 + num_t00 + num_t01 + num_t02 + num_t03;
	numLocalHeap = numLocalHeapRay + numLocalHeapHit;
	//Global
	int num_b0 = 1;
	numGlobalHeap = num_b0;

	for (int heapInd = 0; heapInd < numSwapIndex; heapInd++) {

		mpSrvUavCbvHeap[heapInd] = device->CreateDescHeap(numLocalHeap + numGlobalHeap);
		LocalHandleRay[heapInd] = mpSrvUavCbvHeap[heapInd].Get()->GetGPUDescriptorHandleForHeapStart();
		LocalHandleHit[heapInd].ptr = LocalHandleRay[heapInd].ptr + numLocalHeapRay * device->getCbvSrvUavDescriptorSize();
		GlobalHandle[heapInd].ptr = LocalHandleHit[heapInd].ptr + numLocalHeapHit * device->getCbvSrvUavDescriptorSize();

		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavCbvHeap[heapInd].Get()->GetCPUDescriptorHandleForHeapStart();

		//Local
		//UAVを作成 gOutput(u0)
		mpOutputResource.CreateUavTexture(srvHandle);

		//UAVを作成 gDepthOut(u1)
		mpDepthResource.CreateUavTexture(srvHandle);

		//UAVを作成 gInstanceIdMap(u2)
		mpInstanceIdMapResource.CreateUavTexture(srvHandle);

		//SRVを作成 Indices(t0)
		for (auto i = 0; i < PD.size(); i++) {
			for (int j = 0; j < PD[i]->NumMaterial; j++) {
				for (UINT t = 0; t < numMaterialMaxInstance(PD[i]); t++) {
					IndexView& iv = PD[i]->IviewDXR[j];
					UINT size = sizeof(uint32_t);
					iv.IndexBufferGPU.CreateSrvBuffer(srvHandle, size);
				}
			}
		}

		//CBVを作成 material(b1)
		for (UINT i = 0; i < numMaterial; i++) {
			D3D12_GPU_VIRTUAL_ADDRESS ad1[1] = { material->getGPUVirtualAddress(i) };
			UINT size1[1] = { material->getSizeInBytes() };
			device->CreateCbv(srvHandle, ad1, size1, 1);
		}

		//CBVを作成 wvp(b2)
		for (UINT i = 0; i < maxNumInstancing; i++) {
			D3D12_GPU_VIRTUAL_ADDRESS ad2[1] = { wvp->getGPUVirtualAddress(i) };
			UINT size2[1] = { wvp->getSizeInBytes() };
			device->CreateCbv(srvHandle, ad2, size2, 1);
		}

		//SRVを作成 g_texDiffuse(t0), g_texNormal(t1), g_texSpecular(t2)
		for (UINT t = 0; t < 3; t++) {
			for (auto i = 0; i < PD.size(); i++) {
				for (int j = 0; j < PD[i]->NumMaterial; j++) {
					for (UINT ins = 0; ins < numMaterialMaxInstance(PD[i]); ins++) {
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
						device->CreateSrvTexture(srvHandle, &res, 1);
					}
				}
			}
		}

		//SRVを作成 Vertex(t3)
		for (auto i = 0; i < PD.size(); i++) {
			for (int j = 0; j < PD[i]->NumMaterial; j++) {
				for (UINT t = 0; t < numMaterialMaxInstance(PD[i]); t++) {
					VertexView& vv = PD[i]->updateDXR[heapInd].VviewDXR[j][t];
					UINT size = vv.VertexByteStride;
					vv.VertexBufferGPU.CreateSrvBuffer(srvHandle, size);
				}
			}
		}

		//Global
		//CBVを作成 cbuffer global(b0)
		D3D12_GPU_VIRTUAL_ADDRESS ad0[1] = { sCB->Resource()->GetGPUVirtualAddress() };
		UINT size0[1] = { sCB->getSizeInBytes() };
		device->CreateCbv(srvHandle, ad0, size0, 1);
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

	mpSamplerHeap = device->CreateSamplerDescHeap(samLinear);
	samplerHandle = mpSamplerHeap.Get()->GetGPUDescriptorHandleForHeapStart();
}

void DxrRenderer::createShaderTable() {

	//レコードサイズ計算
	auto RecordSize = [](UINT addRecordSize) {
		const auto shaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
		UINT RecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		RecordSize += addRecordSize;
		return ALIGNMENT_TO(shaderRecordAlignment, RecordSize);
	};

	UINT raygenRecordSize = RecordSize(sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
	UINT missRecordSize = RecordSize(sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
	UINT hitgroupRecordSize = RecordSize(sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));

	//シェーダーテーブルサイズ
	UINT raygenSize = raygenRecordSize;
	UINT missSize = missRecordSize * 2;
	UINT hitGroupSize = hitgroupRecordSize * 2;

	auto tableAlign = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	auto raygenRegion = ALIGNMENT_TO(tableAlign, raygenSize);
	auto missRegion = ALIGNMENT_TO(tableAlign, missSize);
	auto hitgroupRegion = ALIGNMENT_TO(tableAlign, hitGroupSize);

	uint32_t shaderTableSize = raygenRegion + missRegion + hitgroupRegion;

	ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	mpPipelineState.Get()->QueryInterface(IID_PPV_ARGS(pRtsoProps.GetAddressOf()));

	auto writeTable = [pRtsoProps](uint8_t* dst, LPCWSTR Shader, uint64_t* Handle = nullptr) {
		memcpy(dst, pRtsoProps.Get()->GetShaderIdentifier(Shader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		if (Handle) {
			*(uint64_t*)(dst + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = *Handle;
		}
	};

	for (int i = 0; i < numSwapIndex; i++) {

		Dx_Device::GetInstance()->createUploadResource(mpShaderTable[i].GetAddressOf(), shaderTableSize);

		uint8_t* pData = nullptr;
		mpShaderTable[i].Get()->Map(0, nullptr, (void**)&pData);

		//rayGen
		uint8_t* ray = pData;
		writeTable(ray, kRayGenShader, &LocalHandleRay[i].ptr);
		pData += raygenRegion;

		//basic miss
		uint8_t* miss = pData;
		writeTable(miss, kBasicMissShader);
		miss += missRecordSize;

		//emissive miss
		writeTable(miss, kEmissiveMiss);
		pData += missRegion;

		//basic hit
		uint8_t* hit = pData;
		writeTable(hit, kBasicHitGroup, &LocalHandleHit[i].ptr);
		hit += hitgroupRecordSize;

		//emissive hit
		writeTable(hit, kEmissiveHitGroup, &LocalHandleHit[i].ptr);

		mpShaderTable[i].Get()->Unmap(0, nullptr);

		Dx_SwapChain* sw = Dx_SwapChain::GetInstance();

		D3D12_DISPATCH_RAYS_DESC& rDesc = raytraceDesc[i];

		rDesc.Width = sw->getClientWidth();
		rDesc.Height = sw->getClientHeight();
		rDesc.Depth = 1;

		D3D12_GPU_VIRTUAL_ADDRESS startAddress = mpShaderTable[i].Get()->GetGPUVirtualAddress();

		rDesc.RayGenerationShaderRecord.StartAddress = startAddress;
		rDesc.RayGenerationShaderRecord.SizeInBytes = raygenSize;
		startAddress += raygenRegion;

		rDesc.MissShaderTable.StartAddress = startAddress;
		rDesc.MissShaderTable.StrideInBytes = missRecordSize;
		rDesc.MissShaderTable.SizeInBytes = missSize;
		startAddress += missRegion;

		rDesc.HitGroupTable.StartAddress = startAddress;
		rDesc.HitGroupTable.StrideInBytes = hitgroupRecordSize;
		rDesc.HitGroupTable.SizeInBytes = hitGroupSize;
	}
}

void DxrRenderer::updateMaterial(CBobj* cbObj) {

	using namespace CoordTf;
	Dx_Device* dev = Dx_Device::GetInstance();
	UINT MaterialCnt = 0;
	for (auto i = 0; i < PD.size(); i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dev->dxrBuffSwapRaytraceIndex()];

		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT t = 0; t < numMaterialMaxInstance(PD[i]); t++) {
				VECTOR4& dif = PD[i]->diffuse[j];
				VECTOR4& spe = PD[i]->specular[j];
				VECTOR4& amb = PD[i]->ambient[j];
				DxrMaterialCB& mcb = cbObj->matCb[MaterialCnt];
				mcb.vDiffuse.as(dif.x, dif.y, dif.z, 0.0f);
				mcb.vSpeculer.as(spe.x, spe.y, spe.z, 0.0f);
				mcb.vAmbient.as(amb.x, amb.y, amb.z, 0.0f);
				mcb.shininess = ud.shininess;
				mcb.RefractiveIndex = ud.RefractiveIndex;
				MaterialCnt++;
			}
		}
	}
}

void DxrRenderer::updateCB(CBobj* cbObj, UINT numRecursion) {
	using namespace CoordTf;
	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	Dx_Device* dev = Dx_Device::GetInstance();
	MATRIX VP;
	Dx_SwapChain::Update& upd = sw->getUpdate(dev->cBuffSwapDrawOrStreamoutputIndex());
	MatrixMultiply(&VP, &upd.mView, &upd.mProj);
	MatrixTranspose(&VP);
	DxrConstantBuffer& cb = cbObj->cb;
	MatrixInverse(&cb.projectionToWorld, &VP);
	cb.cameraPosition.as(upd.pos.x, upd.pos.y, upd.pos.z, 1.0f);
	cb.maxRecursion = numRecursion;
	cb.TMin_TMax.x = TMin;
	cb.TMin_TMax.y = TMax;
	updateMaterial(cbObj);

	int cntEm = 0;
	bool breakF = false;
	UINT MaterialCnt = 0;
	for (auto i = 0; i < PD.size(); i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dev->dxrBuffSwapRaytraceIndex()];
		UINT udInstanceIDCnt = 0;
		for (int j = 0; j < PD[i]->NumMaterial; j++) {
			for (UINT k = 0; k < ud.NumInstance; k++) {
				UINT matAddInd = 0;
				if (PD[i]->hs)matAddInd = k;
				if ((cbObj->matCb[MaterialCnt + matAddInd].materialNo & EMISSIVE) == EMISSIVE) {
					for (UINT v = 0; v < ud.numVertex; v++) {
						MATRIX Transpose;
						memcpy(&Transpose, &ud.Transform[k], sizeof(MATRIX));
						MatrixTranspose(&Transpose);
						VECTOR3 v3 = {};
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
						float plightOn = PD[i]->getplightOn(dev->dxrBuffSwapRaytraceIndex(), v, j, k);
						CoordTf::VECTOR4 Lightst = PD[i]->getLightst(dev->dxrBuffSwapRaytraceIndex(), v, j, k);
						cb.emissivePosition[cntEm].w = plightOn;
						cb.Lightst[cntEm].x = Lightst.x;
						cb.Lightst[cntEm].y = Lightst.y;
						cb.Lightst[cntEm].z = Lightst.z;
						cb.Lightst[cntEm].w = Lightst.w;
						cb.emissiveNo[cntEm].x = (float)ud.InstanceID[udInstanceIDCnt + k];
						cntEm++;
						if (cntEm >= LIGHT_PCS) {
							breakF = true;
							break;
						}
					}
				}
				if (breakF)break;
			}
			udInstanceIDCnt += PD[i]->NumMaxInstance;
			MaterialCnt += numMaterialMaxInstance(PD[i]);
			if (breakF)break;
		}
		if (breakF)break;
	}

	Dx_Light::Update& ul = Dx_Light::getUpdate(dev->cBuffSwapDrawOrStreamoutputIndex());
	DirectionLight& dl = ul.dlight;

	cb.numEmissive.x = (float)cntEm;
	memcpy(&cb.GlobalAmbientColor, &Dx_Light::getGlobalAmbientLight(), sizeof(VECTOR4));
	memcpy(&cb.dDirection, &dl.Direction, sizeof(VECTOR4));
	memcpy(&cb.dLightColor, &dl.LightColor, sizeof(VECTOR4));
	cb.dLightst.as(dl.onoff, 0.0f, 0.0f, 0.0f);

	UINT InstancingCnt = 0;
	for (auto i = 0; i < PD.size(); i++) {
		UpdateDXR& ud = PD[i]->updateDXR[dev->dxrBuffSwapRaytraceIndex()];
		for (UINT k = 0; k < ud.NumInstance; k++) {
			int index = InstancingCnt + k;
			memcpy(&cbObj->wvpCb[index].wvp, &ud.WVP[k], sizeof(MATRIX));
			memcpy(&cbObj->wvpCb[index].world, &ud.Transform[k], sizeof(MATRIX));
			memcpy(&cbObj->wvpCb[index].AddObjColor, &ud.AddObjColor[k], sizeof(VECTOR4));
			wvp->CopyData(index, cbObj->wvpCb[index]);
		}
		InstancingCnt += PD[i]->NumMaxInstance;
	}
}

void DxrRenderer::updateAS(Dx_CommandListObj* com, UINT numRecursion) {
	createBottomLevelAS(com);
	createTopLevelAS(com);
	updateCB(&cbObj[buffSwap[0]], numRecursion);
}

void DxrRenderer::setCB() {
	sCB->CopyData(0, cbObj[buffSwap[1]].cb);
	for (UINT i = 0; i < numMaterial; i++)material->CopyData(i, cbObj[buffSwap[1]].matCb[i]);
}

void DxrRenderer::raytrace(Dx_CommandListObj* com) {

	if (!asObj[buffSwap[1]].topLevelBuffers.firstSet)return;

	Dx_Device* dev = Dx_Device::GetInstance();
	setCB();

	com->getCommandList()->SetComputeRootSignature(mpGlobalRootSig.Get());

	ID3D12DescriptorHeap* heaps[] = { mpSrvUavCbvHeap[dev->dxrBuffSwapRaytraceIndex()].Get(), mpSamplerHeap.Get() };
	com->getCommandList()->SetDescriptorHeaps(ARRAY_SIZE(heaps), heaps);

	com->getCommandList()->SetComputeRootDescriptorTable(0, GlobalHandle[dev->dxrBuffSwapRaytraceIndex()]);
	com->getCommandList()->SetComputeRootDescriptorTable(1, samplerHandle);
	com->getCommandList()->SetComputeRootShaderResourceView(2, asObj[buffSwap[1]].mpTopLevelAS.Get()->GetGPUVirtualAddress());

	//Dispatch
	com->getCommandList()->SetPipelineState1(mpPipelineState.Get());
	com->getCommandList()->DispatchRays(&raytraceDesc[dev->dxrBuffSwapRaytraceIndex()]);
}

void DxrRenderer::copyBackBuffer(uint32_t comNo) {
	//結果をバックバッファにコピー
	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	sw->GetRtBuffer()->delayCopyResource(comNo, &mpOutputResource);
}

void DxrRenderer::copyDepthBuffer(uint32_t comNo) {

	Dx_SwapChain* sw = Dx_SwapChain::GetInstance();
	sw->GetDepthBuffer()->delayCopyResource(comNo, &mpDepthResource);
}

void DxrRenderer::update_g(int comNo, UINT numRecursion) {
	Dx_CommandListObj* d = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comNo);
	updateAS(d, numRecursion);
}

void DxrRenderer::update_c(int comNo, UINT numRecursion) {
	Dx_CommandListObj* d = Dx_CommandManager::GetInstance()->getComputeComListObj(comNo);
	updateAS(d, numRecursion);
}

void DxrRenderer::raytrace_g(int comNo) {
	Dx_CommandListObj* d = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comNo);
	raytrace(d);
}

void DxrRenderer::raytrace_c(int comNo) {
	Dx_CommandListObj* d = Dx_CommandManager::GetInstance()->getComputeComListObj(comNo);
	raytrace(d);
}

DxrRenderer::~DxrRenderer() {
	S_DELETE(sCB);
	S_DELETE(material);
	S_DELETE(wvp);
}

Dx_Resource* DxrRenderer::getInstanceIdMap() {
	return &mpInstanceIdMapResource;
}