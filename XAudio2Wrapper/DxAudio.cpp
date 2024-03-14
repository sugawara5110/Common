//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　	       DxAudio                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DxAudio.h"
#include "../../WAVELoader/WAVELoader.h"

namespace {

    DxAudio* da = nullptr;
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2MasteringVoice* pMasteringVoice = nullptr;

    void ErrorMessage(char* E_mes) {
        MessageBoxA(0, E_mes, 0, MB_OK);
    }
}

DxAudio::~DxAudio() {
    pMasteringVoice->DestroyVoice();
    pMasteringVoice = nullptr;
    pXAudio2->Release();
    pXAudio2 = nullptr;
}

HRESULT DxAudio::ComInitialize() {
	return CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

void DxAudio::ComUninitialize() {
	CoUninitialize();
}

DxAudio* DxAudio::GetInstance() {
	if (!da)da = new DxAudio();
	return da;
}

void DxAudio::DeleteInstance() {
	delete da;
	da = nullptr;
}

HRESULT DxAudio::Create() {

    UINT32 flags = 0;
    HRESULT hr = XAudio2Create(&pXAudio2, flags);
    if (FAILED(hr)) {
        ErrorMessage("XAudio2 initialization failed.");
        return hr;
    }

#if defined(DEBUG) || defined(_DEBUG) 
    XAUDIO2_DEBUG_CONFIGURATION debug = { 0 };
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    pXAudio2->SetDebugConfiguration(&debug, 0);
#endif

    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice))) {
        ErrorMessage("MasteringVoice creation failed.");
        pXAudio2->Release();
        pXAudio2 = nullptr;
        return hr;
    }

    return hr;
}

DxAudioSourceVoice::~DxAudioSourceVoice() {
    if (pSourceVoice) {
        waitFence();
        pSourceVoice->DestroyVoice();
        pSourceVoice = nullptr;
    }
}

HRESULT DxAudioSourceVoice::CreateSource(
    const char* szFileName,
    UINT32 LoopLength, UINT32 LoopBegin, UINT32 LoopCount) {

    WAVELoader wav;
    WAVE_Output out = wav.loadWAVE(szFileName);
    WAVEFORMATEX wfx = {};

    switch (out.format) {
    case PCM:
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        break;

    case ALAW:

        break;

    case MULAW:
        break;
    }

    switch (out.numChannels) {
    case MONAURAL:
        wfx.nChannels = 1;
        break;

    case STEREO:
        wfx.nChannels = 2;
        break;
    }

    wfx.nSamplesPerSec = out.SamplesPerSec;
    wfx.nAvgBytesPerSec = out.AvgBytesPerSec;
    wfx.nBlockAlign = out.BlockAlign;
    wfx.wBitsPerSample = out.BitsPerSample;
    wfx.cbSize = out.cbSize;

    HRESULT hr = S_OK;
    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &wfx))) {

        ErrorMessage("Error CreateSourceVoice");
        return hr;
    }

    wave_data = std::move(out.wave_data);
    xbuffer.pAudioData = wave_data.get(); //波形データアドレスを登録
    xbuffer.Flags = XAUDIO2_END_OF_STREAM;    //この後はデータ無しの意味
    xbuffer.AudioBytes = out.wave_size; //波形データサイズ

    if (LoopLength > 0) {              //0の場合全て再生
        xbuffer.LoopBegin = LoopBegin;  //再生開始位置
        xbuffer.LoopLength = LoopLength;//再生範囲長さ
        xbuffer.LoopCount = LoopCount;  //ループ回数(1で1回ループ)
    }

    return hr;
}

HRESULT DxAudioSourceVoice::Start() {

    HRESULT hr = S_OK;
    if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&xbuffer))) {

        ErrorMessage("Error SubmitSourceBuffer");
        pSourceVoice->DestroyVoice();
        pSourceVoice = nullptr;
        return hr;
    }

    return pSourceVoice->Start(0);
}

HRESULT DxAudioSourceVoice::Stop() {

    HRESULT hr = S_OK;

    if (FAILED(hr = pSourceVoice->Stop())) {
        ErrorMessage("Error Stop");
        pSourceVoice->DestroyVoice();
        pSourceVoice = nullptr;
        return hr;
    }
    if (FAILED(hr = pSourceVoice->FlushSourceBuffers())) {
        ErrorMessage("Error FlushSourceBuffers");
        pSourceVoice->DestroyVoice();
        pSourceVoice = nullptr;
        return hr;
    }

    waitFence();
    return hr;
}

void DxAudioSourceVoice::waitFence() {

    bool isRunning = true;
    while (isRunning) {
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState(&state);
        isRunning = state.BuffersQueued > 0;
    }
}