///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ShaderSkinMesh.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderSkinMesh =
"cbuffer global_bones : register(b1, space0)\n"//ボーンのポーズ行列が入る
"{\n"
"   matrix g_mConstBoneWorld[256];\n"
"};\n"

//スキニング後の頂点・法線が入る
"struct Skin\n"
"{\n"
"   float4 Pos : POSITION;\n"
"   float3 Nor : NORMAL;\n"
"   float3 Tan : TANGENT;\n"
"};\n"
//バーテックスバッファーの入力
"struct VSSkinIn\n"
"{\n"
"   float3 Pos : POSITION;\n"//頂点   
"   float3 Nor : NORMAL;\n"//法線
"   float3 Tan : TANGENT;\n"//接ベクトル
"   float2 Tex0 : TEXCOORD0;\n"//テクスチャー座標0
"   float2 Tex1 : TEXCOORD1;\n"//テクスチャー座標1
"   uint4  Bones : BONE_INDEX;\n"//ボーンのインデックス
"   float4 Weights : BONE_WEIGHT;\n"//ボーンの重み
"};\n"

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
"GS_Mesh_INPUT VSSkin(VSSkinIn input, uint instanceID : SV_InstanceID)\n"
"{\n"
"    GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

"    Skin vSkinned = SkinVert(input);\n"

"    output.Pos = vSkinned.Pos;\n"
"    output.Nor = vSkinned.Nor;\n"
"    output.Tan = vSkinned.Tan;\n"
"    output.instanceID = instanceID;\n"
"    output.Tex0 = input.Tex0;\n"
"    output.Tex1 = input.Tex1;\n"

"    return output;\n"
"}\n";
//****************************************メッシュ頂点**************************************************************//
