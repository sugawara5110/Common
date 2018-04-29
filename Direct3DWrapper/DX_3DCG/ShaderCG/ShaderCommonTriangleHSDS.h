///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleHSDS.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//ShaderFunction.hに連結させて使う
char *ShaderCommonTriangleHSDS =
"struct VS_OUTPUT\n"
"{\n"
"    float4 Pos    : POSITION;\n"
"    float3 Nor    : NORMAL;\n"
"    float3 GNor   : GEO_NORMAL;\n"
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
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
"    float2 Tex        : TEXCOORD;\n"
"    uint   instanceID : SV_InstanceID;\n"
"};\n"

//***************************************ハルシェーダーコンスタント*************************************************//
"HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT, 3> ip, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT)0;\n"

//ワールド変換
"   float4 wPos = mul(ip[0].Pos, g_World[ip[0].instanceID]);\n"
//頂点から現在地までの距離を計算
"   float distance = length(g_C_Pos.xyz - wPos.xyz);\n"

//距離でポリゴン数決定
"   float divide = 1;\n"
"   if(distance < 1000){divide = 2;}\n"
"   if(distance < 500){divide = 5;}\n"
"   if(distance < 300){divide = 10;}\n"

"	output.factor[0] = divide;\n"
"	output.factor[1] = divide;\n"
"	output.factor[2] = divide;\n"
//u 縦の分割数（横のラインを何本ひくか）
"	output.inner_factor = divide;\n"

"	return output;\n"
"}\n"
//***************************************ハルシェーダーコンスタント*************************************************//

//***************************************ハルシェーダー*************************************************************//
"[domain(\"tri\")]\n"
"[partitioning(\"integer\")]\n"
"[outputtopology(\"triangle_cw\")]\n"//裏表cw, ccw
"[outputcontrolpoints(3)]\n"
"[patchconstantfunc(\"HSConstant\")]\n"
"HS_OUTPUT HS(InputPatch<VS_OUTPUT, 3> ip, uint cpid : SV_OutputControlPointID, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_OUTPUT output = (HS_OUTPUT)0;\n"
"	output.Pos = ip[cpid].Pos;\n"
"	output.Nor = ip[cpid].Nor;\n"
"	output.GNor = ip[cpid].GNor;\n"
"	output.Tex = ip[cpid].Tex;\n"
"   output.instanceID = ip[cpid].instanceID;\n"
"	return output;\n"
"}\n"
//***************************************ハルシェーダー*************************************************************//

//**************************************ドメインシェーダー**********************************************************//
//三角形は重心座標系  (UV.x + UV.y + UV.z) == 1.0f が成り立つ
"[domain(\"tri\")]\n"
"PS_INPUT DS(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)\n"
"{\n"
"	PS_INPUT output = (PS_INPUT)0;\n"

//UV座標計算
"   float2 uv = patch[0].Tex * UV.x + patch[1].Tex * UV.y + patch[2].Tex * UV.z;\n"
"   output.Tex = uv;\n"

//画像から高さを算出
"   float4 height = g_texColor.SampleLevel(g_samLinear, uv, 0);\n"
"   float hei = (height.x + height.y + height.z) / 3;\n"

//  法線ベクトル計算
"   float3 Normal0 = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;\n"

//pos座標計算
"   output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;\n"

//ローカル法線の方向にhei分頂点移動(コントロールポイント位置で処理を分ける)
"   if(UV.x == 0.0f || UV.y == 0.0f || UV.z == 0.0f)\n"//どれかの要素が0.0fの場合端に有る状態
"   {\n"
"      float3 geoDir = patch[0].GNor * UV.x + patch[1].GNor * UV.y + patch[2].GNor * UV.z;\n"
"      output.Pos.xyz += hei * geoDir * g_DispAmount.x;\n"//端はジオメトリ法線使用(クラッキング対策)
"   }\n"
"   else\n"
"   {\n"
"      output.Pos.xyz += hei * Normal0 * g_DispAmount.x;\n"
"   }\n"

//画像から生成したベクトルにローカル法線を足し法線ベクトルとする
"   float3 Normal = height.xyz + Normal0;\n"

//座標変換
"   output.wPos = mul(output.Pos, g_World[patch[0].instanceID]);\n"
"   output.Pos = mul(output.Pos, g_WVP[patch[0].instanceID]);\n"

//出力する法線の作成
"   output.Nor = mul(Normal, (float3x3)g_World[patch[0].instanceID]);\n"

"	return output;\n"
"}\n";
//**************************************ドメインシェーダー**********************************************************//