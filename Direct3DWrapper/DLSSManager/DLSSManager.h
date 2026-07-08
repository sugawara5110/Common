//*****************************************************************************************//
//**                                                                                     **//
//**                   Å@Å@Å@         DLSSManager                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DLSSManager_Header
#define Class_DLSSManager_Header

#include "../DX_3DCG/DXR/DxrRenderer.h"
#ifdef free
#undef free
#endif
#include "../NVIDIA_Streamline/include/sl.h"
#include "../NVIDIA_Streamline/include/sl_dlss.h"
#pragma comment(lib, "sl.interposer.lib")

class DLSSManager {

public:

    static void InstanceCreate();
    static DLSSManager* GetInstance();
    static void DeleteInstance();

    bool Initialize(
        ID3D12Device* device,
        IDXGIAdapter1* adapter);

    enum Mode
    {
        eOff,
        eMaxPerformance,
        eBalanced,
        eMaxQuality,
        eUltraPerformance,
        eUltraQuality,
        eDLAA,
    };

    bool Configure(
        uint32_t comIndex,
        Mode mode,
        uint32_t outputWidth,
        uint32_t outputHeight,
        uint32_t* renderWidth,
        uint32_t* renderHeight);

    bool Evaluate(
        uint32_t comIndex,
        Dx_Resource* color,
        Dx_Resource* depth,
        Dx_Resource* motion,
        CameraData& camera);

    Dx_Resource* GetOutput()
    {
        return &mOutputTexture;
    }

private:

    bool CreateOutputTexture(uint32_t comIndex);

    void FillConstants(sl::Constants& c, CameraData& camera);

    void SetMode(Mode mode);

    void Shutdown();

    static DLSSManager* dl;

    DLSSManager() {}
    DLSSManager(const DLSSManager& obj) = delete;
    void operator=(const DLSSManager& obj) = delete;
    ~DLSSManager();

    ID3D12Device* mDevice = nullptr;

    IDXGIAdapter1* mAdapter = nullptr;

    Dx_Resource mOutputTexture = {};

    sl::ViewportHandle mViewport = sl::ViewportHandle(0);

    uint32_t mOutputWidth = 0;
    uint32_t mOutputHeight = 0;

    sl::DLSSMode dlssmode = sl::DLSSMode::eOff;
};

#endif
