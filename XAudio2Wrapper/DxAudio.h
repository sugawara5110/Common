//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	       DxAudio                                           **//
//**                                                                                     **//
//*****************************************************************************************//
#ifndef Class_DxAudio_Header
#define Class_DxAudio_Header

#include <stdio.h>
#include <xaudio2.h>
#include <stdint.h>
#include <memory>
#pragma comment(lib,"xaudio2.lib")

class DxAudio {

private:
	DxAudio() {};

public:
	~DxAudio();
	static HRESULT ComInitialize();//DirectX使わない時使用
	static void ComUninitialize();
	static DxAudio* GetInstance();
	static void DeleteInstance();

	HRESULT Create();
};

class DxAudioSourceVoice {

private:
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	std::unique_ptr<uint8_t[]> wave_data;
	XAUDIO2_BUFFER xbuffer = {};

	void waitFence();

public:
	~DxAudioSourceVoice();

	HRESULT CreateSource(
		const char* szFileName,
		UINT32 LoopLength = 0, UINT32 LoopBegin = 0, UINT32 LoopCount = 0);

	HRESULT Start();
	HRESULT Stop();
};

#endif