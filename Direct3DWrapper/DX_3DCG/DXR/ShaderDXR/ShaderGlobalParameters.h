///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderGlobalParameters.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderGlobalParameters =

"static uint Seed = 0;\n"
"static const float PI = 3.1415926;\n"
"static const float3 local_normal = float3(0.0f, 0.0f, 1.0f);\n"
"static const float AIR_RefractiveIndex = 1.0f;\n"

"struct RayPayload\n"
"{\n"
"    float3 color;\n"
"    float3 hitPosition;\n"
"    float3 normal;\n"
"    float3 throughput;\n"
"    bool reTry;\n"
"    bool hit;\n"
"    float Alpha;\n"
"    uint RecursionCnt;\n"
"    uint EmissiveIndex;\n"
"    uint mNo;\n"
"    float depth;\n"
"    int hitInstanceId;\n"
"};\n"

"cbuffer global : register(b0, space0)\n"
"{\n"
"    matrix prevViewProjection;\n"
"    matrix projectionToWorld;\n"
"    float4 cameraPosition;\n"
"    float4 emissivePosition[256];\n"//.w:onoff
"    float4 numEmissive;\n"//x:Em, y:numInstance
"    float4 lightst[256];\n"//レンジ, 減衰1, 減衰2, 減衰3
"    float4 GlobalAmbientColor;\n"
"    float4 emissiveNo[256];\n"//x:emissiveNo, y:OutlineSize
"    float4 TMin_TMax;\n"//.x, .y
"    float4 frameReset_DepthRange_NorRange;\n"//.x:フレームインデックスリセット(1.0でリセット), .y:深度レンジ, .z:法線レンジ
"    uint maxRecursion;\n"
"    uint traceMode;\n"
"    uint SeedFrame;\n"
"    float IBL_size;\n"
"    bool useImageBasedLighting;\n"
"};\n"

"RWTexture2D<uint> gFrameIndexMap : register(u6, space0);\n"

"Texture2D g_texImageBasedLighting : register(t5, space16);\n"

"SamplerState g_samLinear : register(s0, space14);\n"
"RaytracingAccelerationStructure gRtScene : register(t4, space15);\n";