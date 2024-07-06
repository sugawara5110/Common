///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderGlobalParameters.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderGlobalParameters =

"static uint Seed = 0;\n"

"struct RayPayload\n"
"{\n"
"    float3 color;\n"
"    float3 hitPosition;\n"
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
"    matrix projectionToWorld;\n"
"    float4 cameraPosition;\n"
"    float4 emissivePosition[256];\n"//.w:onoff
"    float4 numEmissive;\n"//x:Em, y:numInstance
"    float4 lightst[256];\n"//レンジ, 減衰1, 減衰2, 減衰3
"    float4 GlobalAmbientColor;\n"
"    float4 emissiveNo[256];\n"//x:emissiveNo
"    float4 dDirection;\n"
"    float4 dLightColor;\n"
"    float4 dLightst;\n"//.x:オンオフ
"    float4 TMin_TMax;\n"//.x, .y
"    float4 LightArea_RandNum;\n"//x:乱数範囲area(2.0で全方向), y:乱数個数
"    uint frameInd;\n"//フレームインデックス
"    uint maxRecursion;\n"
"};\n"

"SamplerState g_samLinear : register(s0, space14);\n"
"RaytracingAccelerationStructure gRtScene : register(t4, space15);\n";