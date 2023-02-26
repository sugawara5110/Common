//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      Dx_ShaderHolder                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_ShaderHolder.h"
#include "./ShaderCG/ShaderCommonParameters.h"
#include "./ShaderCG/ShaderNormalTangent.h"
#include "./ShaderCG/Shader2D.h"
#include "./ShaderCG/Shader3D.h"
#include "./ShaderCG/ShaderMesh.h"
#include "./ShaderCG/ShaderMesh_D.h"
#include "./ShaderCG/ShaderSkinMesh.h"
#include "./ShaderCG/ShaderSkinMesh_D.h"
#include "./ShaderCG/ShaderCommonPS.h"
#include "./ShaderCG/ShaderCommonTriangleGS.h"
#include "./ShaderCG/ShaderCommonTriangleHSDS.h"
#include "./ShaderCG/ShaderCalculateLighting.h"
#include "./ShaderCG/ShaderGsOutput.h"
#include "./ShaderCG/ShaderSkinMeshCom.h"

void addChar::addStr(char* str1, char* str2) {
	size_t size1 = strlen(str1);
	size_t size2 = strlen(str2);
	size = size1 + size2 + 1;
	str = new char[size];
	memcpy(str, str1, size1 + 1);
	strncat(str, str2, size2 + 1);
}

static bool CreateShaderByteCodeBool = true;
static bool CreateFin = false;

ComPtr<ID3DBlob> Dx_ShaderHolder::CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName) {

	HRESULT hr;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors = nullptr;
	hr = D3DCompile(szFileName, size, nullptr, nullptr, nullptr, szFuncName, szProfileName,
		D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION, 0, &byteCode, &errors);
	if (FAILED(hr)) {
		CreateShaderByteCodeBool = false;
	}

	if (errors != nullptr)
		Dx_Util::ErrorMessage((char*)errors->GetBufferPointer());

	return byteCode;
}

void Dx_ShaderHolder::setNorTestPS() {
	addChar ComPS, Lighting, ParaNor;
	char* com = ShaderCommonParameters;
	ParaNor.addStr(com, ShaderNormalTangent);
	Lighting.addStr(ParaNor.str, ShaderCalculateLighting);
	ComPS.addStr(Lighting.str, ShaderCommonPS);

	pPixelShader_3D = CompileShader(ComPS.str, ComPS.size, "PS_L_NorTest", "ps_5_1");
	pPixelShader_3D_NoNormalMap = CompileShader(ComPS.str, ComPS.size, "PS_L_NoNormalMap_NorTest", "ps_5_1");
}

bool Dx_ShaderHolder::CreateShaderByteCode() {

	if (CreateFin)return true;

	size_t norS_size = strlen(ShaderNormalTangent) + 1;
	size_t norL_size = strlen(ShaderCalculateLighting) + 1;
	size_t com_size = strlen(ShaderCommonParameters) + 1;
	ShaderNormalTangentCopy = std::make_unique<char[]>(norS_size);
	ShaderCalculateLightingCopy = std::make_unique<char[]>(norL_size);
	ShaderCommonParametersCopy = std::make_unique<char[]>(com_size);
	memcpy(ShaderNormalTangentCopy.get(), ShaderNormalTangent, norS_size);
	memcpy(ShaderCalculateLightingCopy.get(), ShaderCalculateLighting, norL_size);
	memcpy(ShaderCommonParametersCopy.get(), ShaderCommonParameters, com_size);

	//各Shader結合
	addChar D3, Mesh, MeshD, Skin, SkinD, ComPS, ComHSDS, ComGS, ParaNor, Lighting, GsOut;
	char* com = ShaderCommonParameters;
	ParaNor.addStr(com, ShaderNormalTangent);
	Lighting.addStr(ParaNor.str, ShaderCalculateLighting);
	D3.addStr(com, Shader3D);
	Mesh.addStr(com, ShaderMesh);
	MeshD.addStr(com, ShaderMesh_D);
	Skin.addStr(com, ShaderSkinMesh);
	SkinD.addStr(com, ShaderSkinMesh_D);
	ComPS.addStr(Lighting.str, ShaderCommonPS);
	ComHSDS.addStr(ParaNor.str, ShaderCommonTriangleHSDS);
	ComGS.addStr(ParaNor.str, ShaderCommonTriangleGS);
	GsOut.addStr(ParaNor.str, ShaderGsOutput);

	//CommonPS
	pPixelShader_3D = CompileShader(ComPS.str, ComPS.size, "PS_L", "ps_5_1");
	pPixelShader_3D_NoNormalMap = CompileShader(ComPS.str, ComPS.size, "PS_L_NoNormalMap", "ps_5_1");
	pPixelShader_Emissive = CompileShader(ComPS.str, ComPS.size, "PS", "ps_5_1");
	//CommonHSDS(Triangle)
	pHullShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "HS", "hs_5_1");
	pDomainShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "DS", "ds_5_1");
	//CommonGS
	pGeometryShader_Before_ds_Smooth = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_Smooth", "gs_5_1");
	pGeometryShader_Before_ds_Edge = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_Edge", "gs_5_1");
	pGeometryShader_Before_vs = CompileShader(ComGS.str, ComGS.size, "GS_Before_vs", "gs_5_1");
	pGeometryShader_Before_ds_NoNormalMap_Smooth = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_NoNormalMap_Smooth", "gs_5_1");
	pGeometryShader_Before_ds_NoNormalMap_Edge = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_NoNormalMap_Edge", "gs_5_1");
	pGeometryShader_Before_vs_NoNormalMap = CompileShader(ComGS.str, ComGS.size, "GS_Before_vs_NoNormalMap", "gs_5_1");

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
	pVertexShader_SKIN = CompileShader(Skin.str, Skin.size, "VSSkin", "vs_5_1");
	//テセレーター有
	pVertexShader_SKIN_D = CompileShader(SkinD.str, SkinD.size, "VS", "vs_5_1");

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
	pVertexShader_MESH = CompileShader(Mesh.str, Mesh.size, "VSMesh", "vs_5_1");
	//テセレーター有メッシュ
	pVertexShader_MESH_D = CompileShader(MeshD.str, MeshD.size, "VSMesh", "vs_5_1");

	//3Dレイアウト基本色
	pVertexLayout_3DBC =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//基本色3D
	pVertexShader_BC = CompileShader(D3.str, D3.size, "VSBaseColor", "vs_5_1");
	pPixelShader_BC = CompileShader(D3.str, D3.size, "PSBaseColor", "ps_5_1");

	//2Dレイアウト
	pVertexLayout_2D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//テクスチャ2D
	pVertexShader_2DTC = CompileShader(Shader2D, strlen(Shader2D), "VSTextureColor", "vs_5_1");
	pPixelShader_2DTC = CompileShader(Shader2D, strlen(Shader2D), "PSTextureColor", "ps_5_1");
	//2D
	pVertexShader_2D = CompileShader(Shader2D, strlen(Shader2D), "VSBaseColor", "vs_5_1");
	pPixelShader_2D = CompileShader(Shader2D, strlen(Shader2D), "PSBaseColor", "ps_5_1");

	//DXRへのOutput
	pGeometryShader_Before_vs_Output = CompileShader(GsOut.str, GsOut.size, "GS_Before_vs", "gs_5_1");
	pGeometryShader_Before_ds_Output_Smooth = CompileShader(GsOut.str, GsOut.size, "GS_Before_ds_Smooth", "gs_5_1");
	pGeometryShader_Before_ds_Output_Edge = CompileShader(GsOut.str, GsOut.size, "GS_Before_ds_Edge", "gs_5_1");
	pDeclaration_Output =
	{
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL",   0, 0, 3, 0 },
		{ 0, "TANGENT",  0, 0, 3, 0 },
		{ 0, "TEXCOORD", 0, 0, 2, 0 },
		{ 0, "TEXCOORD", 1, 0, 2, 0 }
	};

	pVertexShader_SKIN_Com = CompileShader(ShaderSkinMeshCom, strlen(ShaderSkinMeshCom), "VSSkinCS", "cs_5_1");

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