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
    IDXGIAdapter1* adapter)
{
    //Streamline設定
    sl::Preferences pref{};

    pref.showConsole = false;
    pref.logLevel = sl::LogLevel::eVerbose;

    pref.applicationId = 0;//開発用dllなら0

    sl::Feature features[] =
    {
        sl::kFeatureDLSS
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
        sl::kFeatureDLSS,
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
    uint32_t* renderHeight)
{
    SetMode(mode);

    mOutputWidth = outputWidth;
    mOutputHeight = outputHeight;

    if (!CreateOutputTexture(comIndex))
    {
        return false;
    }

    mViewport = sl::ViewportHandle(0);

    //DLSS設定
    sl::DLSSOptions option{};
    option.mode = dlssmode;
    option.outputWidth = outputWidth;
    option.outputHeight = outputHeight;
    option.colorBuffersHDR = sl::Boolean::eFalse;
    option.useAutoExposure = sl::Boolean::eTrue;

    auto result = slDLSSSetOptions(
        mViewport,
        option);

    if (result != sl::Result::eOk)
    {
        return false;
    }

    sl::DLSSOptimalSettings setting{};

    result = slDLSSGetOptimalSettings(option, setting);

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

bool DLSSManager::CreateOutputTexture(uint32_t comIndex)
{
    if (FAILED(mOutputTexture.createDefaultResourceTEXTURE2D_UNORDERED_ACCESS(mOutputWidth, mOutputHeight))) {
        return false;
    }
    mOutputTexture.ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return true;
}

bool DLSSManager::Evaluate(
    uint32_t comIndex,
    Dx_Resource* color,
    Dx_Resource* depth,
    Dx_Resource* motion,
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

    color->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    depth->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    motion->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource colorRes(
        sl::ResourceType::eTex2d,
        color->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource depthRes(
        sl::ResourceType::eTex2d,
        depth->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource motionRes(
        sl::ResourceType::eTex2d,
        motion->getResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    sl::Resource outputRes(
        sl::ResourceType::eTex2d,
        mOutputTexture.getResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    sl::ResourceTag tags[]
    {
        {
            &colorRes,
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
        sl::kFeatureDLSS,
        *frame,
        inputs,
        _countof(inputs),
        reinterpret_cast<sl::CommandBuffer*>(cmdList));

    color->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    depth->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    motion->ResourceBarrier(comIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (result != sl::Result::eOk)
    {
        std::cout << "slEvaluateFeature : "
            << (int)result
            << std::endl;

        return false;
    }

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

    c.mvecScale =
    {
        1.0f,
        1.0f
    };

    c.depthInverted =
        sl::Boolean::eFalse;

    c.jitterOffset =
    {
        0.0f,
        0.0f
    };

    c.reset =
        sl::Boolean::eFalse;
}