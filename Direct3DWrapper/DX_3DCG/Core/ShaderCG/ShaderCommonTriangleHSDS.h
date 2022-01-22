///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 ShaderCommonTriangleHSDS.hlsl                                         //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderCommonTriangleHSDS =
//***************************************ハルシェーダーコンスタント*************************************************//
"HS_CONSTANT_OUTPUT HSConstant(InputPatch<VS_OUTPUT, 3> ip, uint pid : SV_PrimitiveID)\n"
"{\n"
"	HS_CONSTANT_OUTPUT output = (HS_CONSTANT_OUTPUT)0;\n"

//instanceID切り替え, DXR テセレーション時のみ
"   uint instanceID = ip[0].instanceID;\n"
"   if(g_instanceID.y == 1.0f) instanceID = g_instanceID.x;\n"
//ワールド変換
"   float4 wPos = mul(ip[0].Pos, wvpCb[instanceID].world);\n"
//頂点から現在地までの距離を計算
"   float distance = length(g_C_Pos.xyz - wPos.xyz);\n"

//距離でポリゴン数決定
"   float divide = 2.0f;\n"
"   for(int i = 0;i < g_DispAmount.y;i++){\n"
"      if(distance < g_divide[i].x){divide = g_divide[i].y;}\n"
"   }\n"

"	output.factor[0] = divide;\n"
"	output.factor[1] = divide;\n"
"	output.factor[2] = divide;\n"
//u 縦の分割数（横のラインを何本ひくか）
"	output.inner_factor = divide;\n"
//divideが2  →   3 *  6頂点
//divideが4  →   3 * 24
//divideが8  →   3 * 96
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
"   output.Tan = ip[cpid].Tan;\n"
"	output.GNor = ip[cpid].GNor;\n"
"	output.Tex0 = ip[cpid].Tex0;\n"
"	output.Tex1 = ip[cpid].Tex1;\n"
"   output.instanceID = ip[cpid].instanceID;\n"
"	return output;\n"
"}\n"
//***************************************ハルシェーダー*************************************************************//

//**************************************ドメインシェーダー**********************************************************//
//三角形は重心座標系  (UV.x + UV.y + UV.z) == 1.0f が成り立つ
"[domain(\"tri\")]\n"
"GS_Mesh_INPUT DS(HS_CONSTANT_OUTPUT In, float3 UV : SV_DomaInLocation, const OutputPatch<HS_OUTPUT, 3> patch)\n"
"{\n"
"	GS_Mesh_INPUT output = (GS_Mesh_INPUT)0;\n"

//UV座標計算
"   output.Tex0 = patch[0].Tex0 * UV.x + patch[1].Tex0 * UV.y + patch[2].Tex0 * UV.z;\n"
"   output.Tex1 = patch[0].Tex1 * UV.x + patch[1].Tex1 * UV.y + patch[2].Tex1 * UV.z;\n"

//画像から高さを算出
"   float4 height = g_texDiffuse.SampleLevel(g_samLinear, output.Tex0, 0);\n"
"   float hei = (height.x + height.y + height.z) / 3 * g_DispAmount.x;\n"

//法線ベクトル
"   output.Nor = patch[0].Nor * UV.x + patch[1].Nor * UV.y + patch[2].Nor * UV.z;\n"

//接ベクトル
"   output.Tan = patch[0].Tan * UV.x + patch[1].Tan * UV.y + patch[2].Tan * UV.z;\n"

//pos座標計算
"   output.Pos = patch[0].Pos * UV.x + patch[1].Pos * UV.y + patch[2].Pos * UV.z;\n"

//ローカル法線の方向にhei分頂点移動(コントロールポイント位置で処理を分ける)
"   if(UV.x == 0.0f || UV.y == 0.0f || UV.z == 0.0f)\n"//どれかの要素が0.0fの場合端に有る状態
"   {\n"
"      float3 geoDir = patch[0].GNor * UV.x + patch[1].GNor * UV.y + patch[2].GNor * UV.z;\n"
"      output.Pos.xyz += hei * geoDir;\n"//端はジオメトリ法線使用(クラッキング対策)
"   }\n"
"   else\n"
"   {\n"
"      output.Pos.xyz += hei * output.Nor;\n"
"   }\n"

//Smooth用
"   output.AddNor = NormalRecalculationSmoothPreparation(output.Tex0);\n"
"   output.AddNor = normalTexConvert(output.AddNor, output.Nor, output.Tan);\n"

"   output.instanceID = patch[0].instanceID;\n"

"	return output;\n"
"}\n";
//**************************************ドメインシェーダー**********************************************************//
