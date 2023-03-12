//*****************************************************************************************//
//**                                                                                     **//
//**                                Dx_TextureHolder                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_Dx_TextureHolder_Header
#define Class_Dx_TextureHolder_Header

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_ShaderHolder.h"
#include <stdlib.h>
#include <string>
#include <array>
#include <assert.h>
#include <Process.h>
#include <new>
#include <typeinfo>

using Microsoft::WRL::ComPtr;

class InternalTexture {
public:
	UCHAR* byteArr = nullptr;
	char* texName = nullptr; //ファイル名
	DXGI_FORMAT format = {};
	int width = 0;
	LONG_PTR RowPitch = 0;
	int height = 0;

	ComPtr<ID3D12Resource> textureUp = nullptr;
	ComPtr<ID3D12Resource> texture = nullptr;

	bool createRes = false;

	void setParameter(DXGI_FORMAT Format, int Width, LONG_PTR rowPitch, int Height) {
		format = Format;
		width = Width;
		RowPitch = rowPitch;
		height = Height;
	}
	void setName(char* name) {
		int ln = (int)strlen(name) + 1;
		texName = new char[ln];
		memcpy(texName, name, sizeof(char) * ln);
	}
	void setData(UCHAR* ByteArr) {
		byteArr = new UCHAR[RowPitch * height];
		memcpy(byteArr, ByteArr, sizeof(UCHAR) * RowPitch * height);
	}
	~InternalTexture() {
		ARR_DELETE(byteArr);
		ARR_DELETE(texName);
	}
};

class DxCommon;

class Dx_TextureHolder {

private:
	friend DxCommon;
	//テクスチャ
	int texNum = 0;    //配列数
	InternalTexture* texture = nullptr;

	static Dx_TextureHolder* dx;//クラス内でオブジェクト生成し使いまわす

	Dx_TextureHolder() {}//外部からのオブジェクト生成禁止
	Dx_TextureHolder(const Dx_TextureHolder& obj) = delete;   // コピーコンストラクタ禁止
	void operator=(const Dx_TextureHolder& obj) = delete;// 代入演算子禁止
	~Dx_TextureHolder();

public:
	HRESULT createTexture(int com_no, UCHAR* byteArr, DXGI_FORMAT format,
		ID3D12Resource** up, ID3D12Resource** def,
		int width, LONG_PTR RowPitch, int height);

	static void InstanceCreate();
	static Dx_TextureHolder* GetInstance();
	static void DeleteInstance();

	int GetTexNumber(CHAR* fileName);//リソースとして登録済みのテクスチャ配列番号をファイル名から取得

	void createTextureArr(int numTexArr, int resourceIndex, char* texName,
		UCHAR* byteArr, DXGI_FORMAT format,
		int width, LONG_PTR RowPitch, int height,
		ComPtr<ID3D12Resource> texture = nullptr,
		ComPtr<ID3D12Resource> textureUp = nullptr);

	HRESULT createTextureResourceArr(int com_no);

	InternalTexture* getInternalTexture(int index) { return &texture[index + 2]; }
};

#endif
