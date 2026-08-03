//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         DLSSManager                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#include "DLSSManager.h"

namespace {
    sl::float4x4 ToSLMatrix(const CoordTf::MATRIX& m)
    {
        sl::float4x4 r{};

        r.row[0] = { m._11, m._12, m._13, m._14 };
        r.row[1] = { m._21, m._22, m._23, m._24 };
        r.row[2] = { m._31, m._32, m._33, m._34 };
        r.row[3] = { m._41, m._42, m._43, m._44 };

        return r;
    }
}

DLSSManager* DLSSManager::dl = nullptr;

void DLSSManager::InstanceCreate() {
    if (!dl)dl = NEW DLSSManager();
}

DLSSManager* DLSSManager::GetInstance() {
    return dl;
}

void DLSSManager::DeleteInstance() {
    S_DELETE(dl);
}

DLSSManager::~DLSSManager() {
    Shutdown();
}

bool DLSSManager::Initialize(
    ID3D12Device* device,
    IDXGIAdapter1* adapter,
    bool show_log)
{
    //Streamline設定
    sl::Preferences pref{};

    pref.showConsole = false;
    pref.logLevel = sl::LogLevel::eOff;

    if (show_log) {
        pref.logLevel = sl::LogLevel::eVerbose;
    }

    pref.applicationId = 0;//開発用dllなら0

    sl::Feature features[] =
    {
        sl::kFeatureDLSS,
        sl::kFeatureDLSS_RR
    };

    pref.featuresToLoad = features;
    pref.numFeaturesToLoad = _countof(features);;

    //初期化
    sl::Result result = slInit(pref);

    if (result != sl::Result::eOk)
    {
        std::cout << "slInit Failed : "
            << (int)result
            << std::endl;

        return false;
    }

    //Device登録
    result = slSetD3DDevice(device);

    if (result != sl::Result::eOk)
    {
        std::cout << "slSetD3DDevice Error : "
            << (int)result
            << std::endl;
        return false;
    }

    //Adapter情報
    sl::AdapterInfo info{};
    DXGI_ADAPTER_DESC1 desc{};
    adapter->GetDesc1(&desc);
    info.deviceLUID = (uint8_t*)&desc.AdapterLuid;
    info.deviceLUIDSizeInBytes = sizeof(LUID);
    //DLSS対応確認
    result = slIsFeatureSupported(
        sl::kFeatureDLSS_RR,
        info);

    if (result == sl::Result::eOk)
    {
        std::cout << "DLSS Supported!"
            << std::endl;

        return true;
    }

    std::cout << "DLSS NOT Supported : "
        << (int)result
        << std::endl;

    return false;
}

void DLSSManager::SetMode(
    DLSSManager::Mode mode)
{
    switch (mode)
    {
    case eOff:
        dlssmode = sl::DLSSMode::eOff;
        break;

    case eMaxPerformance:
        dlssmode = sl::DLSSMode::eMaxPerformance;
        break;

    case eBalanced:
        dlssmode = sl::DLSSMode::eBalanced;
        break;

    case eMaxQuality:
        dlssmode = sl::DLSSMode::eMaxQuality;
        break;

    case eUltraPerformance:
        dlssmode = sl::DLSSMode::eUltraPerformance;
        break;

    case eUltraQuality:
        dlssmode = sl::DLSSMode::eUltraQuality;
        break;

    case eDLAA:
        dlssmode = sl::DLSSMode::eDLAA;
        break;
    }
}

bool DLSSManager::Configure(
    uint32_t comIndex,
    Mode mode,
    uint32_t outputWidth,
    uint32_t outputHeight,
    uint32_t* renderWidth,
    uint32_t* renderHeight,
    bool Hdr)
{
    HDR = Hdr;
    SetMode(mode);

    mOutputWidth = outputWidth;
    mOutputHeight = outputHeight;

    if (!CreateOutputTexture(comIndex, HDR))
    {
        return false;
    }

    mViewport = sl::ViewportHandle(0);

    //DLSS設定
    sl::DLSSDOptions option{};
    option.mode = dlssmode;
    option.outputWidth = outputWidth;
    option.outputHeight = outputHeight;

    option.colorBuffersHDR =
        HDR ? sl::Boolean::eTrue
        : sl::Boolean::eFalse;

    option.normalRoughnessMode =
        sl::DLSSDNormalRoughnessMode::eUnpacked;

    auto result = slDLSSDSetOptions(
        mViewport,
        option);

    if (result != sl::Result::eOk)
    {
        return false;
    }

    sl::DLSSDOptimalSettings setting{};

    result = slDLSSDGetOptimalSettings(option, setting);

    if (result != sl::Result::eOk)
    {
        return false;
    }

    *renderWidth = setting.optimalRenderWidth;
    *renderHeight = setting.optimalRenderHeight;

    return true;
}

void DLSSManager::Shutdown()
{
    slShutdown();
}

bool DLSSManager::CreateOutputTexture(uint32_t comIndex, bool HDR)
{
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (HDR)format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    if (FAILED(mOutputTexture.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputWidth, mOutputHeight,
        D3D12_RESOURCE_STATE_COMMON, format))) {
        return false;
    }
    mOutputTexture.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return true;
}

bool DLSSManager::Evaluate(
    uint32_t comIndex,
    Dx_Resource* Radiance,
    Dx_Resource* depth,
    Dx_Resource* motion,
    Dx_Resource* normalBuffer,
    Dx_Resource* DiffuseAlbedoBuffer,
    Dx_Resource* SpecularAlbedoBuffer,
    Dx_Resource* roughnessBuffer,
    CameraData& camera)
{
    Dx_CommandListObj* d = Dx_CommandManager::GetInstance()->getGraphicsComListObj(comIndex);
    ID3D12GraphicsCommandList4* cmdList = d->getCommandList();

    sl::FrameToken* frame = nullptr;

    sl::Result result = slGetNewFrameToken(frame);

    if (result != sl::Result::eOk)
        return false;

    sl::Constants constants{};

    FillConstants(constants, camera);

    result = slSetConstants(
        constants,
        *frame,
        mViewport);

    if (result != sl::Result::eOk)
        return false;

    Radiance->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    depth->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    motion->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    normalBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    DiffuseAlbedoBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    roughnessBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    SpecularAlbedoBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource RadianceRes(
        sl::ResourceType::eTex2d,
        Radiance->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource depthRes(
        sl::ResourceType::eTex2d,
        depth->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource motionRes(
        sl::ResourceType::eTex2d,
        motion->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource normalBufferRes(
        sl::ResourceType::eTex2d,
        normalBuffer->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource DiffuseAlbedoBufferRes(
        sl::ResourceType::eTex2d,
        DiffuseAlbedoBuffer->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource SpecularAlbedoBufferRes(
        sl::ResourceType::eTex2d,
        SpecularAlbedoBuffer->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource roughnessBufferRes(
        sl::ResourceType::eTex2d,
        roughnessBuffer->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource outputRes(
        sl::ResourceType::eTex2d,
        mOutputTexture.getResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    sl::ResourceTag tags[]
    {
        {
            &RadianceRes,
            sl::kBufferTypeScalingInputColor,
            sl::eValidUntilPresent
        },

        {
            &depthRes,
            sl::kBufferTypeDepth,
            sl::eValidUntilPresent
        },

        {
            &motionRes,
            sl::kBufferTypeMotionVectors,
            sl::eValidUntilPresent
        },

        {
            &normalBufferRes,
            sl::kBufferTypeNormals,
            sl::eValidUntilPresent
        },

        {
            &DiffuseAlbedoBufferRes,
            sl::kBufferTypeAlbedo,
            sl::eValidUntilPresent
        },

        {
            &SpecularAlbedoBufferRes,
            sl::kBufferTypeSpecularAlbedo,
            sl::eValidUntilPresent
        },

        {
            &roughnessBufferRes,
            sl::kBufferTypeRoughness,
            sl::eValidUntilPresent
        },

        {
            &outputRes,
            sl::kBufferTypeScalingOutputColor,
            sl::eValidUntilPresent
        }
    };

    result = slSetTag(
        mViewport,
        tags,
        _countof(tags),
        reinterpret_cast<sl::CommandBuffer*>(cmdList));

    if (result != sl::Result::eOk)
        return false;

    const sl::BaseStructure* inputs[]
    {
        &mViewport
    };

    result = slEvaluateFeature(
        sl::kFeatureDLSS_RR,
        *frame,
        inputs,
        _countof(inputs),
        reinterpret_cast<sl::CommandBuffer*>(cmdList));

    if (result != sl::Result::eOk)
    {
        std::cout << "slEvaluateFeature : "
            << (int)result
            << std::endl;

        return false;
    }

    Radiance->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    depth->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    motion->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    normalBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    DiffuseAlbedoBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    roughnessBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    SpecularAlbedoBuffer->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    return true;
}

void DLSSManager::FillConstants(
    sl::Constants& c,
    CameraData& camera)
{
    using namespace CoordTf;

    c.cameraViewToClip = ToSLMatrix(camera.Proj);

    MATRIX InvProj = {};
    MatrixInverse(&InvProj, &camera.Proj);
    c.clipToCameraView = ToSLMatrix(InvProj);

    // clip(Current) → clip(Previous)
    MATRIX CurrentVPInverse = {};
    MatrixInverse(&CurrentVPInverse, &camera.CurrentVP);
    c.clipToPrevClip =
        ToSLMatrix(CurrentVPInverse * camera.PreviousVP);

    // Previous → Current
    MATRIX PreviousVPInverse = {};
    MatrixInverse(&PreviousVPInverse, &camera.PreviousVP);
    c.prevClipToClip =
        ToSLMatrix(PreviousVPInverse * camera.CurrentVP);

    c.cameraPos =
    {
        camera.Position.x,
        camera.Position.y,
        camera.Position.z
    };

    c.cameraRight =
    {
        camera.Right.x,
        camera.Right.y,
        camera.Right.z
    };

    c.cameraUp =
    {
        camera.Up.x,
        camera.Up.y,
        camera.Up.z
    };

    c.cameraFwd =
    {
        camera.Forward.x,
        camera.Forward.y,
        camera.Forward.z
    };

    c.cameraNear = camera.Near;

    c.cameraFar = camera.Far;

    c.cameraFOV = camera.Fov;

    c.cameraAspectRatio =
        float(camera.Width) /
        float(camera.Height);

    c.cameraMotionIncluded =
        sl::Boolean::eTrue;

    c.motionVectors3D =
        sl::Boolean::eFalse;

    c.motionVectorsDilated =
        sl::Boolean::eFalse;

    c.motionVectorsJittered =
        sl::Boolean::eFalse;

    if (camera.isJitter_F()) {
        c.motionVectorsJittered =
            sl::Boolean::eTrue;
    }

    c.mvecScale =
    {
        1.0f,
        1.0f
    };

    c.depthInverted =
        sl::Boolean::eFalse;

    c.jitterOffset =
    {
        camera.jitter.pixelX,
        camera.jitter.pixelY
    };

    c.reset = sl::Boolean::eFalse;

    sl::DLSSDOptions option{};

    option.mode = dlssmode;
    option.outputWidth = mOutputWidth;
    option.outputHeight = mOutputHeight;

    option.colorBuffersHDR =
        HDR ? sl::Boolean::eTrue
        : sl::Boolean::eFalse;

    option.normalRoughnessMode =
        sl::DLSSDNormalRoughnessMode::eUnpacked;

    option.worldToCameraView =
        ToSLMatrix(camera.View);

    MATRIX invView{};
    MatrixInverse(&invView, &camera.View);

    option.cameraViewToWorld =
        ToSLMatrix(invView);

    slDLSSDSetOptions(
        mViewport,
        option);
}