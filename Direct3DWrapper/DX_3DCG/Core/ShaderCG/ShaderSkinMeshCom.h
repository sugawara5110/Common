///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ShaderSkinMeshCom.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderSkinMeshCom =
"cbuffer global_bones : register(b0, space0)\n"//ボーンのポーズ行列が入る
"{\n"
"   matrix g_mConstBoneWorld[256];\n"
"};\n"

//スキニング後の頂点・法線が入る
"struct Skin\n"
"{\n"
"   float4 Pos;\n"
"   float3 Nor;\n"
"   float3 Tan;\n"
"};\n"
//バーテックスバッファーの入力
"struct VSSkinIn\n"
"{\n"
"   float3 Pos;\n"//頂点   
"   float3 Nor;\n"//法線
"   float3 Tan;\n"//接ベクトル
"   float3 gNor;\n"//フォーマット合わせるため
"   float2 Tex0;\n"//テクスチャー座標0
"   float2 Tex1;\n"//テクスチャー座標1
"   uint4  Bones;\n"//ボーンのインデックス
"   float4 Weights;\n"//ボーンの重み
"};\n"
"struct DXR_INPUT\n"
"{\n"
"    float3 Pos;\n"
"    float3 Nor;\n"
"    float3 Tan;\n"
"    float2 Tex0;\n"
"    float2 Tex1;\n"
"};\n"

"StructuredBuffer<VSSkinIn> VerticesSkin : register(t0, space0);\n"
"RWStructuredBuffer<DXR_INPUT> VerticesDXR : register(u0, space0);\n"

//指定した番号のボーンのポーズ行列を返す
"matrix FetchBoneMatrix(uint iBone)\n"
"{\n"
"   return g_mConstBoneWorld[iBone];\n"
"}\n"

//頂点をスキニング
"Skin SkinVert(VSSkinIn Input)\n"
"{\n"
"   Skin Output = (Skin)0;\n"

"   float4 Pos = float4(Input.Pos, 1);\n"
"   float3 Nor = Input.Nor;\n"
"   float3 Tan = Input.Tan;\n"
//ボーン0
"   uint iBone = Input.Bones.x;\n"
"   float fWeight = Input.Weights.x;\n"
"   matrix m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"
//ボーン1
"   iBone = Input.Bones.y;\n"
"   fWeight = Input.Weights.y;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"
//ボーン2
"   iBone = Input.Bones.z;\n"
"   fWeight = Input.Weights.z;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"
//ボーン3
"   iBone = Input.Bones.w;\n"
"   fWeight = Input.Weights.w;\n"
"   m = FetchBoneMatrix(iBone);\n"
"   Output.Pos += fWeight * mul(Pos, m);\n"
"   Output.Nor += fWeight * mul(Nor, (float3x3)m);\n"
"   Output.Tan += fWeight * mul(Tan, (float3x3)m);\n"

"   return Output;\n"
"}\n"

//****************************************メッシュ頂点**************************************************************//
"[numthreads(1, 1, 1)]\n"
"void VSSkinCS(int2 id : SV_DispatchThreadID)\n"
"{\n"
"   int verID = id.x;\n"

"   DXR_INPUT output = (DXR_INPUT)0;\n"

"   VSSkinIn input = VerticesSkin[verID];\n"

"   Skin vSkinned = SkinVert(input);\n"

"   output.Pos = vSkinned.Pos.xyz;\n"
"   output.Nor = vSkinned.Nor;\n"
"   output.Tan = vSkinned.Tan;\n"
"   output.Tex0 = input.Tex0;\n"
"   output.Tex1 = input.Tex1;\n"

"   VerticesDXR[verID] = output;\n"
"}\n";
//****************************************メッシュ頂点**************************************************************//

