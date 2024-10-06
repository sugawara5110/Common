//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      Dx_ShaderHolder                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_ShaderHolder.h"

void addChar::addStr(char* str1, char* str2) {
	size_t size1 = strlen(str1);
	size_t size2 = strlen(str2);
	size = size1 + size2 + 1;
	str = NEW char[size];
	memcpy(str, str1, size1 + 1);
	strncat(str, str2, size2 + 1);
}

bool Dx_ShaderHolder::CreateShaderByteCodeBool = true;
bool Dx_ShaderHolder::CreateFin = false;
bool Dx_ShaderHolder::setCommonPass_fin = false;
char* Dx_ShaderHolder::middle_pass = "Common/Direct3DWrapper/DX_3DCG/Core/ShaderCG/";

ComPtr<ID3DBlob> Dx_ShaderHolder::CompileShader(LPCVOID pSrcData, size_t size, LPCSTR pSourceName, LPSTR szFuncName, LPSTR szProfileName, ID3DInclude* pInclude) {

	HRESULT hr;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors = nullptr;
	hr = D3DCompile(pSrcData, size, pSourceName, nullptr, pInclude, szFuncName, szProfileName,
		D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION, 0, &byteCode, &errors);
	if (FAILED(hr)) {
		CreateShaderByteCodeBool = false;
	}

	if (errors != nullptr)
		Dx_Util::ErrorMessage((char*)errors->GetBufferPointer());

	return byteCode;
}

ComPtr<ID3DBlob> Dx_ShaderHolder::CompileShader(LPCVOID pSrcData, size_t size, LPSTR szFuncName, LPSTR szProfileName) {
	return CompileShader(pSrcData, size, nullptr, szFuncName, szProfileName, nullptr);
}

std::unique_ptr<char[]> Dx_ShaderHolder::getShaderPass(char* file_name, char* middle_pass) {

	addChar p1, p2;
	p1.addStr(CommonPass.get(), middle_pass);
	p2.addStr(p1.str, file_name);

	size_t size = strlen(p2.str) + 1;
	std::unique_ptr<char[]> ret = std::make_unique<char[]>(size);
	memcpy(ret.get(), p2.str, size);
	return ret;
}

ComPtr<ID3DBlob> Dx_ShaderHolder::CompileShader(LPCSTR pSourceName, LPSTR szFuncName, LPSTR szProfileName) {
	auto pass = getShaderPass((char*)pSourceName, middle_pass);
	auto pSrcData = Dx_Util::ConvertFileToChar(pass.get());
	return CompileShader(pSrcData.get(), strlen(pSrcData.get()) + 1, pass.get(),
		szFuncName, szProfileName, D3D_COMPILE_STANDARD_FILE_INCLUDE);
}

std::unique_ptr<char[]> Dx_ShaderHolder::getShaderRead(char* file_name, char* middle_pass) {
	auto pass = getShaderPass(file_name, middle_pass);
	return Dx_Util::ConvertFileToChar(pass.get());
}

std::unique_ptr<char[]> Dx_ShaderHolder::getShaderRead_ShaderCG(char* file_name) {
	return getShaderRead(file_name, middle_pass);
}

void Dx_ShaderHolder::setNorTestPS() {
	pPixelShader_3D = CompileShader("ShaderCommonPS.hlsl", "PS_L_NorTest", "ps_5_1");
	pPixelShader_3D_NoNormalMap = CompileShader("ShaderCommonPS.hlsl", "PS_L_NoNormalMap_NorTest", "ps_5_1");
}

bool Dx_ShaderHolder::CreateShaderByteCode() {

	if (CreateFin)return true;

	if (!setCommonPass_fin) {
		Dx_Util::ErrorMessage("Run CreateShaderByteCode after running setCommonPass.");
	}

	ShaderCommonParametersCopy = getShaderRead_ShaderCG("ShaderCommonParameters.hlsl");
	ShaderNormalTangentCopy = getShaderRead_ShaderCG("ShaderNormalTangent.hlsl");
	ShaderCalculateLightingCopy = getShaderRead_ShaderCG("ShaderCalculateLighting.hlsl");

	//CommonPS
	pPixelShader_3D = CompileShader("ShaderCommonPS.hlsl", "PS_L", "ps_5_1");
	pPixelShader_3D_NoNormalMap = CompileShader("ShaderCommonPS.hlsl", "PS_L_NoNormalMap", "ps_5_1");
	pPixelShader_Emissive = CompileShader("ShaderCommonPS.hlsl", "PS", "ps_5_1");
	//CommonHSDS(Triangle)
	pHullShaderTriangle = CompileShader("ShaderCommonTriangleHSDS.hlsl", "HS", "hs_5_1");
	pDomainShaderTriangle = CompileShader("ShaderCommonTriangleHSDS.hlsl", "DS", "ds_5_1");
	//CommonGS
	pGeometryShader_Before_ds_Smooth = CompileShader("ShaderCommonTriangleGS.hlsl", "GS_Before_ds_Smooth", "gs_5_1");
	pGeometryShader_Before_ds_Edge = CompileShader("ShaderCommonTriangleGS.hlsl", "GS_Before_ds_Edge", "gs_5_1");
	pGeometryShader_Before_vs = CompileShader("ShaderCommonTriangleGS.hlsl", "GS_Before_vs", "gs_5_1");
	pGeometryShader_Before_ds_NoNormalMap_Smooth = CompileShader("ShaderCommonTriangleGS.hlsl", "GS_Before_ds_NoNormalMap_Smooth", "gs_5_1");
	pGeometryShader_Before_ds_NoNormalMap_Edge = CompileShader("ShaderCommonTriangleGS.hlsl", "GS_Before_ds_NoNormalMap_Edge", "gs_5_1");
	pGeometryShader_Before_vs_NoNormalMap = CompileShader("ShaderCommonTriangleGS.hlsl", "GS_Before_vs_NoNormalMap", "gs_5_1");

	//スキンメッシュ
	pVertexLayout_SKIN =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "GEO_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_INDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 80, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pVertexShader_SKIN = CompileShader("ShaderSkinMesh.hlsl", "VSSkin", "vs_5_1");
	//テセレーター有
	pVertexShader_SKIN_D = CompileShader("ShaderSkinMesh_D.hlsl", "VS", "vs_5_1");

	//メッシュレイアウト
	pVertexLayout_MESH =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "GEO_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//メッシュ
	pVertexShader_MESH = CompileShader("ShaderMesh.hlsl", "VSMesh", "vs_5_1");
	//テセレーター有メッシュ
	pVertexShader_MESH_D = CompileShader("ShaderMesh_D.hlsl", "VSMesh", "vs_5_1");

	//3Dレイアウト基本色
	pVertexLayout_3DBC =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//基本色3D
	pVertexShader_BC = CompileShader("Shader3D.hlsl", "VSBaseColor", "vs_5_1");
	pPixelShader_BC = CompileShader("Shader3D.hlsl", "PSBaseColor", "ps_5_1");

	//2Dレイアウト
	pVertexLayout_2D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//テクスチャ2D
	pVertexShader_2DTC = CompileShader("Shader2D.hlsl", "VSTextureColor", "vs_5_1");
	pPixelShader_2DTC = CompileShader("Shader2D.hlsl", "PSTextureColor", "ps_5_1");
	//2D
	pVertexShader_2D = CompileShader("Shader2D.hlsl", "VSBaseColor", "vs_5_1");
	pPixelShader_2D = CompileShader("Shader2D.hlsl", "PSBaseColor", "ps_5_1");

	//DXRへのOutput
	pGeometryShader_Before_vs_Output = CompileShader("ShaderGsOutput.hlsl", "GS_Before_vs", "gs_5_1");
	pGeometryShader_Before_ds_Output_Smooth = CompileShader("ShaderGsOutput.hlsl", "GS_Before_ds_Smooth", "gs_5_1");
	pGeometryShader_Before_ds_Output_Edge = CompileShader("ShaderGsOutput.hlsl", "GS_Before_ds_Edge", "gs_5_1");
	pDeclaration_Output =
	{
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL",   0, 0, 3, 0 },
		{ 0, "TANGENT",  0, 0, 3, 0 },
		{ 0, "TEXCOORD", 0, 0, 2, 0 },
		{ 0, "TEXCOORD", 1, 0, 2, 0 }
	};

	pVertexShader_SKIN_Com = CompileShader("ShaderSkinMeshCom.hlsl", "VSSkinCS", "cs_5_1");

	CreateFin = true;

	return CreateShaderByteCodeBool;
}

ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_ds_Smooth = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_ds_Edge = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_vs = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_ds_NoNormalMap_Smooth = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_ds_NoNormalMap_Edge = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_vs_NoNormalMap = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_vs_Output = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_ds_Output_Smooth = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pGeometryShader_Before_ds_Output_Edge = nullptr;

ComPtr<ID3DBlob> Dx_ShaderHolder::pHullShaderTriangle = nullptr;

ComPtr<ID3DBlob> Dx_ShaderHolder::pDomainShaderTriangle = nullptr;

std::vector<D3D12_INPUT_ELEMENT_DESC> Dx_ShaderHolder::pVertexLayout_SKIN;
std::vector<D3D12_SO_DECLARATION_ENTRY> Dx_ShaderHolder::pDeclaration_Output;
std::vector<D3D12_INPUT_ELEMENT_DESC> Dx_ShaderHolder::pVertexLayout_MESH;
std::vector<D3D12_INPUT_ELEMENT_DESC> Dx_ShaderHolder::pVertexLayout_3DBC;
std::vector<D3D12_INPUT_ELEMENT_DESC> Dx_ShaderHolder::pVertexLayout_2D;

ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_SKIN = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_SKIN_D = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_MESH_D = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_MESH = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_BC = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_2D = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_2DTC = nullptr;

ComPtr<ID3DBlob> Dx_ShaderHolder::pPixelShader_3D = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pPixelShader_3D_NoNormalMap = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pPixelShader_Emissive = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pPixelShader_BC = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pPixelShader_2D = nullptr;
ComPtr<ID3DBlob> Dx_ShaderHolder::pPixelShader_2DTC = nullptr;

ComPtr<ID3DBlob> Dx_ShaderHolder::pVertexShader_SKIN_Com = nullptr;

std::unique_ptr<char[]> Dx_ShaderHolder::ShaderNormalTangentCopy = nullptr;
std::unique_ptr<char[]> Dx_ShaderHolder::ShaderCalculateLightingCopy = nullptr;
std::unique_ptr<char[]> Dx_ShaderHolder::ShaderCommonParametersCopy = nullptr;

std::unique_ptr<char[]> Dx_ShaderHolder::CommonPass = nullptr;

void Dx_ShaderHolder::setCommonPass(char* pass) {
	size_t size = strlen(pass) + 1;
	CommonPass = std::make_unique<char[]>(size);
	memcpy(CommonPass.get(), pass, size);
	setCommonPass_fin = true;
}
