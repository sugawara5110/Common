///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                ShaderCommonParameters.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderCommonParameters =
//////////////////////////////////共通パラメーター////////////////////////////////////////////////////////
"Texture2D g_texDiffuse : register(t0);\n"
"Texture2D g_texNormal : register(t1);\n"
"Texture2D g_texSpecular : register(t2);\n"
"SamplerState g_samLinear : register(s0);\n"

"cbuffer global : register(b0)\n"
"{\n"
"    matrix g_World[256]; \n"
"    matrix g_WVP[256];\n"
//視点
"    float4 g_C_Pos;\n"
//視点上方向
"    float4 g_viewUp;"
//オブジェクト追加カラー
"    float4 g_ObjCol;\n"
//グローバルアンビエント
"    float4 g_GlobalAmbientLight;\n"
//xyz:光源位置, w:オンオフ
"    float4 g_LightPos[256];\n"
//ライト色
"    float4 g_LightColor[256];\n"
//レンジ, 減衰1, 減衰2, 減衰3
"    float4 g_Lightst[256];\n"
//x:ライト個数
"    float4 g_numLight;\n"
//平行光源方向
"    float4 g_DLightDirection;\n"
//平行光源色
"    float4 g_DLightColor;\n"
//x:平行光源オンオフ
"    float4 g_DLightst;\n"
//フォグ量x, 密度y, onoffz
"    float4 g_FogAmo_Density;\n"
//フォグ色
"    float4 g_FogColor;\n"
//x:ディスプ起伏量, y:divide配列数, z:shininess
"    float4 g_DispAmount;\n"
//divide配列 x:distance, y:divide
"    float4 g_divide[16];\n"
//UV座標移動用
"    float4 g_pXpYmXmY;\n"
"};\n"

//マテリアル毎の色
"cbuffer global_1 : register(b1)\n"
"{\n"
"    float4 g_Diffuse;\n"
"    float4 g_Speculer; \n"
"    float4 g_Ambient;\n"
"    float4 g_uvSw;\n"
"};\n"

"cbuffer global_2 : register(b3)\n"
"{\n"
//DXR用
"    float4 g_instanceID;\n"//x:ID, y:1.0f on 0.0f off
"};\n"

"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 Nor    : NORMAL;\n"
"    float3 GNor   : GEO_NORMAL;\n"
"    float2 Tex0   : TEXCOORD0;\n"
"    float2 Tex1   : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct GS_Mesh_INPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

"struct PS_INPUT\n"
"{\n"
"    float4 Pos      : SV_POSITION;\n"
"    float4 wPos     : POSITION;\n"
"    float3 Nor      : NORMAL;\n"
"    float2 Tex0     : TEXCOORD0;\n"
"    float2 Tex1     : TEXCOORD1;\n"
"    float3 tangent  : TANGENT;\n"
"};\n"

"struct HS_CONSTANT_OUTPUT\n"
"{\n"
"	 float factor[3]    : SV_TessFactor;\n"
"	 float inner_factor : SV_InsideTessFactor;\n"
"};\n"

"struct HS_OUTPUT\n"
"{\n"
"    float4 Pos   : POSITION;\n"
"    float3 Nor   : NORMAL;\n"
"    float3 GNor  : GEO_NORMAL;\n"
"    float2 Tex0  : TEXCOORD0;\n"
"    float2 Tex1  : TEXCOORD1;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

/////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////フォグ計算(テクスチャに対して計算)////////////////////////////////////////
"float4 FogCom(float4 FogCol, float4 Fog, float4 CPos, float4 wPos, float4 Tex)\n"
"{\n"
"    float fd;\n"//距離
"    float ff;\n"//フォグファクター
"    if(Fog.z == 1.0f){\n"
"       fd = length(CPos.xyz - wPos.xyz) * 0.01f;\n"//距離計算, 0.01は補正値
"       ff = pow(2.71828, -fd * Fog.y);\n"//フォグファクター計算(変化量)
"       ff *= Fog.x;\n"//フォグ全体の量(小さい方が多くなる)
"       ff = saturate(ff);\n"
"       if(Tex.w > 0.3f){\n"
"         Tex = ff * Tex + (1.0f - ff) * FogCol;\n"
"       }\n"
"    }\n"
"   return Tex;\n"
"}\n";
//////////////////////////////////////////////////////////////////////////////////////////////////////////