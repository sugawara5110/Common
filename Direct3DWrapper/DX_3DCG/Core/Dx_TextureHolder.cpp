//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@       Dx_TextureHolder                                     **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "Dx_TextureHolder.h"
#include <WindowsX.h>
#include <locale.h>

Dx_TextureHolder* Dx_TextureHolder::dx = nullptr;

void Dx_TextureHolder::InstanceCreate() {

	if (dx == nullptr)dx = NEW Dx_TextureHolder();
}

Dx_TextureHolder* Dx_TextureHolder::GetInstance() {

	if (dx != nullptr)return dx;
	return nullptr;
}

void Dx_TextureHolder::DeleteInstance() {

	if (dx != nullptr) {
		delete dx;
		dx = nullptr;
	}
}

Dx_TextureHolder::~Dx_TextureHolder() {
	ARR_DELETE(texture);
}

int Dx_TextureHolder::GetTexNumber(CHAR* fileName) {

	fileName = Dx_Util::GetNameFromPath(fileName);

	for (int i = 0; i < texNum; i++) {
		if (texture[i].texName == '\0')continue;
		char str[50];
		char str1[50];
		strcpy(str, texture[i].texName);
		strcpy(str1, fileName);
		int i1 = -1;
		while (str[++i1] != '\0' && str[i1] != '.' && str[i1] == str1[i1]);
		if (str[i1] == '.' && str1[i1] == '.')return i;
	}

	return -1;
}

void Dx_TextureHolder::createTextureArr(int numTexArr, int resourceIndex, char* texName,
	UCHAR* byteArr, DXGI_FORMAT format,
	int width, LONG_PTR RowPitch, int height,
	ComPtr<ID3D12Resource> inTexture,
	ComPtr<ID3D12Resource> inTextureUp) {

	if (!texture) {
		texNum = numTexArr + 2;//dummyNor,dummyDifSpeï™
		texture = NEW InternalTexture[texNum];
		UCHAR dm[8 * 4 * 8] = {};
		UCHAR ndm[4] = { 128,128,255,0 };
		for (int i = 0; i < 8 * 4 * 8; i += 4)
			memcpy(&dm[i], ndm, sizeof(UCHAR) * 4);
		texture[0].setParameter(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8 * 4, 8);
		texture[0].setName("dummyNor.");
		texture[0].setData(dm);

		UCHAR sdm[4] = { 255,255,255,255 };
		for (int i = 0; i < 8 * 4 * 8; i += 4)
			memcpy(&dm[i], sdm, sizeof(UCHAR) * 4);
		texture[1].setParameter(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8 * 4, 8);
		texture[1].setName("dummyDifSpe.");
		texture[1].setData(dm);
	}

	InternalTexture* tex = &texture[resourceIndex + 2];
	tex->setParameter(format, width, RowPitch, height);
	tex->setName(texName);
	tex->setData(byteArr);

	if (inTexture) {
		tex->texture = inTexture;
		tex->createRes = true;
		tex->textureUp = inTextureUp;
	}
}

HRESULT Dx_TextureHolder::createTextureResourceArr(int com_no) {
	for (int i = 0; i < texNum; i++) {
		InternalTexture* tex = &texture[i];
		HRESULT hr = Dx_CommandManager::GetInstance()->createTexture(com_no, tex->byteArr, tex->format,
			tex->textureUp.GetAddressOf(), tex->texture.GetAddressOf(),
			tex->width, tex->RowPitch, tex->height, false);
		tex->textureUp->SetName(Dx_Util::charToLPCWSTR("Up", tex->texName));
		tex->texture->SetName(Dx_Util::charToLPCWSTR("def", tex->texName));

		if (FAILED(hr)) {
			Dx_Util::ErrorMessage("Dx_TextureHolder::createTextureResourceArr Error!!");
			return hr;
		}
		tex->createRes = true;
	}
	return S_OK;
}
