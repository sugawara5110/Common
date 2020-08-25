///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderParametersDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderParametersDXR =

"struct Vertex {\n"
"    float3 Pos;\n"
"    float3 normal;\n"
"    float2 tex[2];\n"
"};\n"

"struct Instance {\n"
"    matrix world;\n"
"};\n"

"struct MaterialCB {\n"
"    float4 Diffuse;\n"
"    float4 Speculer; \n"
"    float4 Ambient;\n"
"    float shininess;\n"
"    float alphaTest;\n"//1.0f:on, 0.0f:off 
"    uint materialNo;\n"//0:metallic, 1:emissive, 2:nonReflection
"};\n"

"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RaytracingAccelerationStructure gRtScene : register(t0, space0);\n"
"StructuredBuffer<uint> Indices[] : register(t1, space1);\n"//無制限配列の場合,別なレジスタ空間にした方が(・∀・)ｲｲ!! みたい
"StructuredBuffer<Vertex> Vertices[] : register(t2, space2);\n"

"cbuffer global : register(b0, space0)\n"
"{\n"
"    matrix projectionToWorld;\n"
"    float4 cameraUp;"
"    float4 cameraPosition;\n"
"    float4 emissivePosition[256];\n"
"    float4 numEmissive;\n"//.xのみ
"    float4 lightst[256];\n"//レンジ, 減衰1, 減衰2, 減衰3
"    float4 GlobalAmbientColor;\n"
"};\n"

"ConstantBuffer<Instance> instance[] : register(b1, space3);\n"
"ConstantBuffer<MaterialCB> material[] : register(b2, space4);\n"
"Texture2D g_texDiffuse[] : register(t0, space10);\n"
"Texture2D g_texNormal[] : register(t1, space11);\n"
"Texture2D g_texSpecular[] : register(t2, space12);\n"
"SamplerState g_samLinear : register(s0, space13);\n"

"struct RayPayload\n"
"{\n"
"    float3 color;\n"
"    bool hit;\n"
"    float Alpha;\n"
"};\n";