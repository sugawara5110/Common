//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      Dx_ShaderHolder                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#include "Dx_ShaderHolder.h"
#include "./ShaderCG/ShaderCommonParameters.h"
#include "./ShaderCG/ShaderNormalTangent.h"
#include "./ShaderCG/Shader2D.h"
#include "./ShaderCG/Shader3D.h"
#include "./ShaderCG/ShaderMesh.h"
#include "./ShaderCG/ShaderMesh_D.h"
#include "./ShaderCG/ShaderParticle.h"
#include "./ShaderCG/ShaderSkinMesh.h"
#include "./ShaderCG/ShaderSkinMesh_D.h"
#include "./ShaderCG/ShaderWaveCom.h"
#include "./ShaderCG/ShaderWaveDraw.h"
#include "./ShaderCG/ShaderCommonPS.h"
#include "./ShaderCG/ShaderCommonTriangleGS.h"
#include "./ShaderCG/ShaderCommonTriangleHSDS.h"
#include "./ShaderCG/ShaderPostEffect.h"
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
		ErrorMessage((char*)errors->GetBufferPointer());

	return byteCode;
}

bool Dx_ShaderHolder::CreateShaderByteCode() {

	size_t norS_size = strlen(ShaderNormalTangent) + 1;
	size_t norL_size = strlen(ShaderCalculateLighting) + 1;
	ShaderNormalTangentCopy = std::make_unique<char[]>(norS_size);
	ShaderCalculateLightingCopy = std::make_unique<char[]>(norL_size);
	memcpy(ShaderNormalTangentCopy.get(), ShaderNormalTangent, norS_size);
	memcpy(ShaderCalculateLightingCopy.get(), ShaderCalculateLighting, norL_size);

	//各Shader結合
	addChar D3, Mesh, MeshD, Skin, SkinD, Wave, ComPS, ComHSDS, ComGS, ParaNor, Lighting, GsOut;
	char* com = ShaderCommonParameters;
	ParaNor.addStr(com, ShaderNormalTangent);
	Lighting.addStr(ParaNor.str, ShaderCalculateLighting);
	D3.addStr(com, Shader3D);
	Mesh.addStr(com, ShaderMesh);
	MeshD.addStr(com, ShaderMesh_D);
	Skin.addStr(com, ShaderSkinMesh);
	SkinD.addStr(com, ShaderSkinMesh_D);
	Wave.addStr(com, ShaderWaveDraw);
	ComPS.addStr(Lighting.str, ShaderCommonPS);
	ComHSDS.addStr(com, ShaderCommonTriangleHSDS);
	ComGS.addStr(ParaNor.str, ShaderCommonTriangleGS);
	GsOut.addStr(ParaNor.str, ShaderGsOutput);

	//CommonPS
	pPixelShader_3D = CompileShader(ComPS.str, ComPS.size, "PS_L", "ps_5_0");
	pPixelShader_3D_NoNormalMap = CompileShader(ComPS.str, ComPS.size, "PS_L_NoNormalMap", "ps_5_0");
	pPixelShader_Emissive = CompileShader(ComPS.str, ComPS.size, "PS", "ps_5_0");
	//CommonHSDS(Triangle)
	pHullShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "HS", "hs_5_0");
	pDomainShaderTriangle = CompileShader(ComHSDS.str, ComHSDS.size, "DS", "ds_5_0");
	//CommonGS
	pGeometryShader_Before_ds_Smooth = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_Smooth", "gs_5_0");
	pGeometryShader_Before_ds_Edge = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_Edge", "gs_5_0");
	pGeometryShader_Before_vs = CompileShader(ComGS.str, ComGS.size, "GS_Before_vs", "gs_5_0");
	pGeometryShader_Before_ds_NoNormalMap_Smooth = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_NoNormalMap_Smooth", "gs_5_0");
	pGeometryShader_Before_ds_NoNormalMap_Edge = CompileShader(ComGS.str, ComGS.size, "GS_Before_ds_NoNormalMap_Edge", "gs_5_0");
	pGeometryShader_Before_vs_NoNormalMap = CompileShader(ComGS.str, ComGS.size, "GS_Before_vs_NoNormalMap", "gs_5_0");

	//ポストエフェクト
	pComputeShader_Post[0] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "MosaicCS", "cs_5_0");
	pComputeShader_Post[1] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "BlurCS", "cs_5_0");
	pComputeShader_Post[2] = CompileShader(ShaderPostEffect, strlen(ShaderPostEffect), "DepthOfFieldCS", "cs_5_0");

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
	pVertexShader_SKIN = CompileShader(Skin.str, Skin.size, "VSSkin", "vs_5_0");
	//テセレーター有
	pVertexShader_SKIN_D = CompileShader(SkinD.str, SkinD.size, "VS", "vs_5_0");

	//ストリーム出力データ定義(パーティクル用)
	pDeclaration_PSO =
	{
		{ 0, "POSITION", 0, 0, 3, 0 }, //「x,y,z」をスロット「0」の「POSITION」に出力
		{ 0, "POSITION", 1, 0, 3, 0 },
		{ 0, "POSITION", 2, 0, 3, 0 },
	};
	//ストリーム出力
	pVertexShader_PSO = CompileShader(ShaderParticle, strlen(ShaderParticle), "VS_SO", "vs_5_0");
	pGeometryShader_PSO = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_Point_SO", "gs_5_0");

	//パーティクル頂点インプットレイアウトを定義
	pVertexLayout_P =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3 * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//パーティクル
	pVertexShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "VS", "vs_5_0");
	pGeometryShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_Point", "gs_5_0");
	pPixelShader_P = CompileShader(ShaderParticle, strlen(ShaderParticle), "PS", "ps_5_0");

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
	pVertexShader_MESH = CompileShader(Mesh.str, Mesh.size, "VSMesh", "vs_5_0");
	//テセレーター有メッシュ
	pVertexShader_MESH_D = CompileShader(MeshD.str, MeshD.size, "VSMesh", "vs_5_0");

	//3Dレイアウト基本色
	pVertexLayout_3DBC =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	//テクスチャ3D
	pVertexShader_TC = CompileShader(D3.str, D3.size, "VSTextureColor", "vs_5_0");
	//基本色3D
	pVertexShader_BC = CompileShader(D3.str, D3.size, "VSBaseColor", "vs_5_0");
	pPixelShader_BC = CompileShader(D3.str, D3.size, "PSBaseColor", "ps_5_0");
	//Wave
	pComputeShader_Wave = CompileShader(ShaderWaveCom, strlen(ShaderWaveCom), "CS", "cs_5_0");
	pDomainShader_Wave = CompileShader(Wave.str, Wave.size, "DSWave", "ds_5_0");

	//2Dレイアウト
	pVertexLayout_2D =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	//テクスチャ2D
	pVertexShader_2DTC = CompileShader(Shader2D, strlen(Shader2D), "VSTextureColor", "vs_5_0");
	pPixelShader_2DTC = CompileShader(Shader2D, strlen(Shader2D), "PSTextureColor", "ps_5_0");
	//2D
	pVertexShader_2D = CompileShader(Shader2D, strlen(Shader2D), "VSBaseColor", "vs_5_0");
	pPixelShader_2D = CompileShader(Shader2D, strlen(Shader2D), "PSBaseColor", "ps_5_0");

	//DXRへのOutput
	pGeometryShader_Before_vs_Output = CompileShader(GsOut.str, GsOut.size, "GS_Before_vs", "gs_5_0");
	pGeometryShader_Before_ds_Output_Smooth = CompileShader(GsOut.str, GsOut.size, "GS_Before_ds_Smooth", "gs_5_0");
	pGeometryShader_Before_ds_Output_Edge = CompileShader(GsOut.str, GsOut.size, "GS_Before_ds_Edge", "gs_5_0");
	pDeclaration_Output =
	{
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL",   0, 0, 3, 0 },
		{ 0, "TANGENT",  0, 0, 3, 0 },
		{ 0, "TEXCOORD", 0, 0, 2, 0 },
		{ 0, "TEXCOORD", 1, 0, 2, 0 }
	};

	pGeometryShader_P_Output = CompileShader(ShaderParticle, strlen(ShaderParticle), "GS_PointDxr", "gs_5_0");

	pVertexShader_SKIN_Com = CompileShader(ShaderSkinMeshCom, strlen(ShaderSkinMeshCom), "VSSkinCS", "cs_5_0");

	return CreateShaderByteCodeBool;
}
