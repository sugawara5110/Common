//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      Dx_ShaderHolder                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_ShaderHolder_Header
#define Class_Dx_ShaderHolder_Header

#include "DxCommandQueue.h"
#include <D3Dcompiler.h>
#include <vector>

#pragma comment(lib,"d3dcompiler.lib")

class addChar {
public:
	char* str = nullptr;
	size_t size = 0;

	void addStr(char* str1, char* str2);

	~addChar() {
		S_DELETE(str);
	}
};

class Dx_ShaderHolder {

private:
	friend Dx12Process;
	friend MeshData;
	friend BasicPolygon;
	friend PolygonData;
	friend PolygonData2D;
	friend ParticleData;
	friend SkinMesh;
	friend Wave;
	friend PostEffect;
	friend Common;
	friend DXR_Basic;
	friend SkinnedCom;

	//シェーダーバイトコード
	ComPtr<ID3DBlob> pGeometryShader_PSO = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_P = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_Smooth = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_Edge = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_vs = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_NoNormalMap_Smooth = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_NoNormalMap_Edge = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_vs_NoNormalMap = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_vs_Output = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_Output_Smooth = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_Before_ds_Output_Edge = nullptr;
	ComPtr<ID3DBlob> pGeometryShader_P_Output = nullptr;

	ComPtr<ID3DBlob> pHullShaderTriangle = nullptr;

	ComPtr<ID3DBlob> pDomainShader_Wave = nullptr;
	ComPtr<ID3DBlob> pDomainShaderTriangle = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_SKIN;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_PSO;
	std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_Output;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_P;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3DBC;
	std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_2D;

	ComPtr<ID3DBlob> pVertexShader_SKIN = nullptr;
	ComPtr<ID3DBlob> pVertexShader_SKIN_D = nullptr;
	ComPtr<ID3DBlob> pVertexShader_PSO = nullptr;
	ComPtr<ID3DBlob> pVertexShader_P = nullptr;
	ComPtr<ID3DBlob> pVertexShader_MESH_D = nullptr;
	ComPtr<ID3DBlob> pVertexShader_MESH = nullptr;
	ComPtr<ID3DBlob> pVertexShader_TC = nullptr;
	ComPtr<ID3DBlob> pVertexShader_BC = nullptr;
	ComPtr<ID3DBlob> pVertexShader_2D = nullptr;
	ComPtr<ID3DBlob> pVertexShader_2DTC = nullptr;

	ComPtr<ID3DBlob> pPixelShader_P = nullptr;
	ComPtr<ID3DBlob> pPixelShader_3D = nullptr;
	ComPtr<ID3DBlob> pPixelShader_3D_NoNormalMap = nullptr;
	ComPtr<ID3DBlob> pPixelShader_Emissive = nullptr;
	ComPtr<ID3DBlob> pPixelShader_BC = nullptr;
	ComPtr<ID3DBlob> pPixelShader_2D = nullptr;
	ComPtr<ID3DBlob> pPixelShader_2DTC = nullptr;

	ComPtr<ID3DBlob> pComputeShader_Wave = nullptr;
	ComPtr<ID3DBlob> pComputeShader_Post[3] = { nullptr };
	ComPtr<ID3DBlob> pVertexShader_SKIN_Com = nullptr;

	std::unique_ptr<char[]> ShaderNormalTangentCopy = nullptr;
	std::unique_ptr<char[]> ShaderCalculateLightingCopy = nullptr;

	bool CreateShaderByteCodeBool = true;

	ComPtr<ID3DBlob> CompileShader(LPSTR szFileName, size_t size, LPSTR szFuncName, LPSTR szProfileName);
	bool CreateShaderByteCode();
};

#endif
