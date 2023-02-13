///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderGlobalParameters.hlsl                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderGlobalParameters =

"#define NONREFLECTION  0b0000 \n"
"#define METALLIC       0b1000 \n"
"#define EMISSIVE       0b0100 \n"
"#define DIRECTIONLIGHT 0b0010 \n"
"#define TRANSLUCENCE   0b0001 \n"

"Texture2D g_texDiffuse[] : register(t0, space10);\n"
"Texture2D g_texNormal[] : register(t1, space11);\n"
"Texture2D g_texSpecular[] : register(t2, space12);\n"

"struct Vertex {\n"
"    float3 Pos;\n"
"    float3 normal;\n"
"    float3 tangent;\n"
"    float2 tex[2];\n"
"};\n"

"struct Vertex3 {\n"
"    Vertex v[3];\n"
"};\n"
"StructuredBuffer<Vertex> Vertices[] : register(t3, space13);\n"

"SamplerState g_samLinear : register(s0, space14);\n"
"RaytracingAccelerationStructure gRtScene : register(t4, space15);\n"

"struct RayPayload\n"
"{\n"
"    float3 color;\n"
"    float3 hitPosition;\n"
"    bool reTry;\n"
"    bool hit;\n"
"    float Alpha;\n"
"    uint RecursionCnt;\n"
"    uint instanceID;\n"
"    uint mNo;\n"
"    float depth;\n"
"    int hitInstanceId;\n"
"};\n";