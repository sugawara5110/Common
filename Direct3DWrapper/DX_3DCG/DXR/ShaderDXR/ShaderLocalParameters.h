///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ShaderLocalParameters.hlsl                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderLocalParameters =

"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RWTexture2D<float> gDepthOut : register(u1, space0);\n"
"RWTexture2D<float> gInstanceIdMap : register(u2, space0);\n"
"StructuredBuffer<uint> Indices[] : register(t0, space1);\n"//無制限配列の場合,別なレジスタ空間にした方が(・∀・)ｲｲ!! みたい

"cbuffer global : register(b0, space0)\n"
"{\n"
"    matrix projectionToWorld;\n"
"    float4 cameraPosition;\n"
"    float4 emissivePosition[256];\n"//.w:onoff
"    float4 numEmissive;\n"//x:Em, y:numInstance
"    float4 lightst[256];\n"//レンジ, 減衰1, 減衰2, 減衰3
"    float4 GlobalAmbientColor;\n"
"    float4 emissiveNo[256];\n"//.xのみ
"    float4 dDirection;\n"
"    float4 dLightColor;\n"
"    float4 dLightst;\n"//.x:オンオフ
"    float4 TMin_TMax;\n"//.x, .y
"    uint maxRecursion;\n"
"};\n"

"struct MaterialCB {\n"
"    float4 Diffuse;\n"
"    float4 Speculer; \n"
"    float4 Ambient;\n"
"    float shininess;\n"
"    float RefractiveIndex;\n"//屈折率
"    float AlphaBlend;\n"
"    uint materialNo;\n"
"};\n"
"ConstantBuffer<MaterialCB> material[] : register(b1, space3);\n"

"struct WVPCB {\n"
"    matrix wvp;\n"
"    matrix world;\n"
"    float4 AddObjColor;\n"
"};\n"
"ConstantBuffer<WVPCB> wvp[] : register(b2, space4);\n";