///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderGlobalParameters.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

static const float PI = 3.1415926;
static const float3 local_normal = float3(0.0f, 0.0f, 1.0f);
static const float AIR_RefractiveIndex = 1.0f;
#define LIGHT_PCS 256

struct RayPayload
{
    float3 color;
    float3 hitPosition;
    float3 normal;
    float3 throughput;
    bool reTry;
    bool hit;
    float Alpha;
    uint RecursionCnt;
    int EmissiveIndex;
    uint mNo;
    float depth;
    int hitInstanceId;
    uint Seed;
};

cbuffer global : register(b0, space0)
{
    matrix prevViewProjection;
    matrix projectionToWorld;
    matrix ImageBasedLighting_Matrix;
    float4 cameraPosition;
    float4 emissivePosition[LIGHT_PCS]; //xyz:Pos, w:オンオフ
    float4 numEmissive; //x:Em, y:numInstance
    float4 lightst[LIGHT_PCS]; //レンジ, 減衰1, 減衰2, 減衰3
    float4 GlobalAmbientColor;
    float4 emissiveNo[LIGHT_PCS]; //x:emissiveNo, y:SizeX, z:SizeY, w:SizeZ
    float4 TMin_TMax; //.x, .y
    float4 frameReset_DepthRange_NorRange; //.x:フレームインデックスリセット(1.0でリセット), .y:深度レンジ, .z:法線レンジ
    uint maxRecursion;
    uint traceMode;
    uint SeedFrame;
    float IBL_size;
    bool useImageBasedLighting;
};

RWTexture2D<uint> gFrameIndexMap : register(u6, space0);

Texture2D g_texImageBasedLighting : register(t5, space16);

SamplerState g_samLinear : register(s0, space14);
RaytracingAccelerationStructure gRtScene : register(t4, space15);