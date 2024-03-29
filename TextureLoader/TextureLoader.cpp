//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	      TextureLoader                                      **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "TextureLoader.h"
#include "MicroSoftLibrary/WICTextureLoader12.h"
#include "../../PNGLoader/PNGLoader.h"
#include "../../JPGLoader/JPGLoader.h"
#include <locale.h>

bool TextureLoader::GetTexture(int numTexArr, Texture* tex, Dx_TextureHolder* dx) {

	char str[100];

	for (int i = 0; i < numTexArr; i++) {

		if (tex[i].texName == nullptr)continue;

		std::unique_ptr<uint8_t[]> decodedData = nullptr;
		D3D12_SUBRESOURCE_DATA subresource;
		ComPtr<ID3D12Resource> t = nullptr;

		if (tex[i].binary_ch != nullptr) {
			if (FAILED(DirectX::LoadWICTextureFromMemory(Dx_Device::GetInstance()->getDevice(),
				(uint8_t*)tex[i].binary_ch, tex[i].binary_size, &t, decodedData, subresource))) {
				sprintf(str, "テクスチャ��%d読み込みエラー", (i));
				Dx_Util::ErrorMessage(str);
				return false;
			}
		}
		else {
			wchar_t ws[200];
			setlocale(LC_CTYPE, "jpn");
			mbstowcs(ws, tex[i].texName, 200);
			tex[i].texName = Dx_Util::GetNameFromPass(tex[i].texName);

			if (FAILED(DirectX::LoadWICTextureFromFile(Dx_Device::GetInstance()->getDevice(),
				ws, &t, decodedData, subresource))) {
				sprintf(str, "テクスチャ��%d読み込みエラー", (i));
				Dx_Util::ErrorMessage(str);
				return false;
			}
		}

		D3D12_RESOURCE_DESC texDesc;
		texDesc = t->GetDesc();
		//テクスチャの横サイズ取得
		int width = (int)texDesc.Width;
		//テクスチャの縦サイズ取得
		int height = (int)texDesc.Height;
		UCHAR* byteArr = (UCHAR*)subresource.pData;
		tex[i].width = width;
		tex[i].height = height;

		dx->createTextureArr(numTexArr, i, tex[i].texName, byteArr, texDesc.Format,
			width, subresource.RowPitch, height);
	}
	return true;
}

bool TextureLoader::GetTexture2(int numTexArr, Texture* tex, Dx_TextureHolder* dx) {

	char str[100];
	PNGLoader png;
	JPGLoader jpg;
	UCHAR* byteArr = nullptr;
	char er[255] = {};
	for (int i = 0; i < numTexArr; i++) {

		if (tex[i].texName == nullptr)continue;

		if (tex[i].binary_ch != nullptr) {
			byteArr = (UCHAR*)png.loadPngInByteArray((UCHAR*)tex[i].binary_ch, tex[i].binary_size, 0, 0);
			tex[i].width = png.getSrcWidth();
			tex[i].height = png.getSrcHeight();
			if (!byteArr) {
				byteArr = (UCHAR*)jpg.loadJpgInByteArray((UCHAR*)tex[i].binary_ch, tex[i].binary_size, 0, 0, er);
				tex[i].width = jpg.getSrcWidth();
				tex[i].height = jpg.getSrcHeight();
			}
			if (!byteArr) {
				Dx_Util::ErrorMessage(er);
			}
		}
		else {
			byteArr = (UCHAR*)png.loadPNG(tex[i].texName, 0, 0);
			tex[i].width = png.getSrcWidth();
			tex[i].height = png.getSrcHeight();
			if (!byteArr) {
				byteArr = (UCHAR*)jpg.loadJPG(tex[i].texName, 0, 0, er);
				tex[i].width = jpg.getSrcWidth();
				tex[i].height = jpg.getSrcHeight();
			}
			tex[i].texName = Dx_Util::GetNameFromPass(tex[i].texName);
			if (!byteArr) {
				Dx_Util::ErrorMessage(er);
			}
		}
		if (!byteArr) {
			sprintf(str, "テクスチャ��%d読み込みエラー", (i));
			Dx_Util::ErrorMessage(str);
			return false;
		}
		dx->createTextureArr(numTexArr, i, tex[i].texName, byteArr, DXGI_FORMAT_R8G8B8A8_UNORM,
			tex[i].width, tex[i].width * 4, tex[i].height);

		ARR_DELETE(byteArr);
	}
	return true;
}