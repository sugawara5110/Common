///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderParametersDXR.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderParametersDXR =

"struct Vertex {\n"
"    float3 Pos;\n"
"    float3 normal;\n"
"    float3 tangent;\n"
"    float2 tex[2];\n"
"};\n"

"struct Vertex3 {\n"
"    Vertex v[3];\n"
"};\n"

"struct MaterialCB {\n"
"    float4 Diffuse;\n"
"    float4 Speculer; \n"
"    float4 Ambient;\n"
"    float4 AddObjColor;\n"
"    float shininess;\n"
"    float RefractiveIndex;\n"//���ܗ�
"    float AlphaBlend;\n"
"    uint materialNo;\n"
"};\n"

"struct WVPCB {\n"
"    matrix wvp;\n"
"    matrix world;\n"
"};\n"

"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RWTexture2D<float> gDepthOut : register(u1, space0);\n"
"StructuredBuffer<uint> Indices[] : register(t0, space1);\n"//�������z��̏ꍇ,�ʂȃ��W�X�^��Ԃɂ�������(�E�́E)��!! �݂���

"cbuffer global : register(b0, space0)\n"
"{\n"
"    matrix projectionToWorld;\n"
"    float4 cameraPosition;\n"
"    float4 emissivePosition[256];\n"//.w:onoff
"    float4 numEmissive;\n"//.x�̂�
"    float4 lightst[256];\n"//�����W, ����1, ����2, ����3
"    float4 GlobalAmbientColor;\n"
"    float4 emissiveNo[256];"//.x�̂�
"    float4 dDirection;\n"
"    float4 dLightColor;\n"
"    float4 dLightst;\n"//.x:�I���I�t
"    uint maxRecursion;\n"
"};\n"

"ConstantBuffer<MaterialCB> material[] : register(b1, space3);\n"
"ConstantBuffer<WVPCB> wvp[] : register(b2, space4);\n"

"Texture2D g_texDiffuse[] : register(t0, space10);\n"
"Texture2D g_texNormal[] : register(t1, space11);\n"
"Texture2D g_texSpecular[] : register(t2, space12);\n"
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
"};\n";