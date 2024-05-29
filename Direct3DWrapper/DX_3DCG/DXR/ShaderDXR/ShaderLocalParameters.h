///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderLocalParameters.hlsl                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderLocalParameters =

"#define NONREFLECTION  0b0000 \n"
"#define METALLIC       0b1000 \n"
"#define EMISSIVE       0b0100 \n"
"#define DIRECTIONLIGHT 0b0010 \n"
"#define TRANSLUCENCE   0b0001 \n"

"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RWTexture2D<float> gDepthOut : register(u1, space0);\n"
"RWTexture2D<float> gInstanceIdMap : register(u2, space0);\n"

"StructuredBuffer<uint> Indices[] : register(t0, space1);\n"//無制限配列の場合,別なレジスタ空間にした方が(・∀・)ｲｲ!! みたい

"struct MaterialCB {\n"
"    float4 Diffuse;\n"
"    float4 Speculer;\n"
"    float4 Ambient;\n"
"    float shininess;\n"
"    float RefractiveIndex;\n"//屈折率
"    uint materialNo;\n"
"};\n"
"ConstantBuffer<MaterialCB> material[] : register(b1, space3);\n"

"struct WVPCB {\n"
"    matrix wvp;\n"
"    matrix world;\n"
"    float4 AddObjColor;\n"
"};\n"
"ConstantBuffer<WVPCB> wvp[] : register(b2, space4);\n"

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
"StructuredBuffer<Vertex> Vertices[] : register(t3, space13);\n";