//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　      Dx_ShaderHolder                                       **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_ShaderHolder_Header
#define Class_Dx_ShaderHolder_Header

#include "Dx_Resource.h"
#include <D3Dcompiler.h>
#include <vector>

#pragma comment(lib,"d3dcompiler.lib")

class addChar {
public:
	char* str = nullptr;
	size_t size = 0;

	void addStr(char* str1, char* str2);

	~addChar() {
		ARR_DELETE(str);
	}
};

class Dx_ShaderHolder {

private:
	Dx_ShaderHolder() {};
	~Dx_ShaderHolder() {};

	static bool CreateShaderByteCodeBool;
	static bool CreateFin;
	static bool setCommonPass_fin;
	static char* middle_pass;

	static std::unique_ptr<char[]> CommonPass;
	static std::unique_ptr<char[]> getShaderPass(char* file_name, char* middle_pass);

	static std::unique_ptr<char[]> getShaderRead_ShaderCG(char* file_name);

public:
	static void setCommonPass(char* pass);
	static bool CreateShaderByteCode();
	static void setNorTestPS();
	//シェーダーバイトコード
	static ComPtr<ID3DBlob> pGeometryShader_Before_ds_Smooth;
	static ComPtr<ID3DBlob> pGeometryShader_Before_ds_Edge;
	static ComPtr<ID3DBlob> pGeometryShader_Before_vs;
	static ComPtr<ID3DBlob> pGeometryShader_Before_ds_NoNormalMap_Smooth;
	static ComPtr<ID3DBlob> pGeometryShader_Before_ds_NoNormalMap_Edge;
	static ComPtr<ID3DBlob> pGeometryShader_Before_vs_NoNormalMap;
	static ComPtr<ID3DBlob> pGeometryShader_Before_vs_Output;
	static ComPtr<ID3DBlob> pGeometryShader_Before_ds_Output_Smooth;
	static ComPtr<ID3DBlob> pGeometryShader_Before_ds_Output_Edge;

	static ComPtr<ID3DBlob> pHullShaderTriangle;

	static ComPtr<ID3DBlob> pDomainShaderTriangle;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_SKIN;
	static std::vector<D3D12_SO_DECLARATION_ENTRY> pDeclaration_Output;
	static std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_MESH;
	static std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_3DBC;
	static std::vector<D3D12_INPUT_ELEMENT_DESC> pVertexLayout_2D;

	static ComPtr<ID3DBlob> pVertexShader_SKIN;
	static ComPtr<ID3DBlob> pVertexShader_SKIN_D;
	static ComPtr<ID3DBlob> pVertexShader_MESH_D;
	static ComPtr<ID3DBlob> pVertexShader_MESH;
	static ComPtr<ID3DBlob> pVertexShader_BC;
	static ComPtr<ID3DBlob> pVertexShader_2D;
	static ComPtr<ID3DBlob> pVertexShader_2DTC;

	static ComPtr<ID3DBlob> pPixelShader_3D;
	static ComPtr<ID3DBlob> pPixelShader_3D_NoNormalMap;
	static ComPtr<ID3DBlob> pPixelShader_Emissive;
	static ComPtr<ID3DBlob> pPixelShader_BC;
	static ComPtr<ID3DBlob> pPixelShader_2D;
	static ComPtr<ID3DBlob> pPixelShader_2DTC;

	static ComPtr<ID3DBlob> pVertexShader_SKIN_Com;

	static std::unique_ptr<char[]> ShaderNormalTangentCopy;
	static std::unique_ptr<char[]> ShaderCalculateLightingCopy;
	static std::unique_ptr<char[]> ShaderCommonParametersCopy;

	static std::unique_ptr<char[]> getShaderRead(char* file_name, char* middle_pass);
	static ComPtr<ID3DBlob> CompileShader(LPCVOID pSrcData, size_t size, LPCSTR pSourceName, LPSTR szFuncName, LPSTR szProfileName, ID3DInclude* pInclude);
	static ComPtr<ID3DBlob> CompileShader(LPCVOID pSrcData, size_t size, LPSTR szFuncName, LPSTR szProfileName);
	static ComPtr<ID3DBlob> CompileShader(LPCSTR pSourceName, LPSTR szFuncName, LPSTR szProfileName);
};

#endif
