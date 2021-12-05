///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderPostEffect.hlsl                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderPostEffect =
"Texture2D<float> gDepth : register(t0, space0);\n"
"RWTexture2D<float4> gInput : register(u0, space0);\n"
"RWTexture2D<float4> gOutput : register(u1, space0);\n"

//モザイク大きさx
//ブラー中心座標xy,ぼけ強度z,ピントが合う深さw
"cbuffer global  : register(b0, space0)\n"
"{\n"
"    float4 g_mosaicSize;\n"
"    float4 g_blur;\n"
"    float g_focusRange;\n"
"};\n"

//Dispatch(APP側)(X1, Y1, Z1)numthreads(CS側)(X, Y, Z)
//x,y,z,x1,y1,z1 は～番目
//X,Y,Z,X1,Y1,Z1 はMax値
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint その他uint3

//モザイクCS
"[numthreads(32, 8, 1)]\n"
"void MosaicCS(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   float size1 = 1 / g_mosaicSize.x;\n"
"   int size2 = g_mosaicSize.x;\n"
"   int x = dtid.x * size1;\n"
"   int y = dtid.y * size1;\n"
"	gOutput[dtid] = gInput[int2(x * size2, y * size2)];\n"
"}\n"

//ブラーCS
"[numthreads(32, 8, 1)]\n"
"void BlurCS(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   float4 Color[10];\n"
//ブラー中心座標 - 現ピクセル位置
"   float2 dir;\n"
"   dir.x = g_blur.x - dtid.x;\n"
"   dir.y = g_blur.y - dtid.y;\n"
//距離計算
"   float len = length(dir);\n"
//方向ベクトル正規化
"   dir = normalize(dir);\n"
//距離を積算することにより、爆発の中心位置に近いほどブラーの影響が小さくなるようにする
"   dir *= g_blur.z * len;\n"

//合成する
"   int x = dtid.x;\n"
"   int y = dtid.y;\n"
"   float dx = -dir.x;\n"
"   float dy = -dir.y;\n"

"   Color[0] = gInput[dtid] * 0.19f;\n"
"   Color[1] = gInput[int2(x + dx, y + dy)] * 0.17f;\n"
"   Color[2] = gInput[int2(x + dx * 2.0f, y + dy * 2.0f)] * 0.15f;\n"
"   Color[3] = gInput[int2(x + dx * 3.0f, y + dy * 3.0f)] * 0.13f;\n"
"   Color[4] = gInput[int2(x + dx * 4.0f, y + dy * 4.0f)] * 0.11f;\n"
"   Color[5] = gInput[int2(x + dx * 5.0f, y + dy * 5.0f)] * 0.09f;\n"
"   Color[6] = gInput[int2(x + dx * 6.0f, y + dy * 6.0f)] * 0.07f;\n"
"   Color[7] = gInput[int2(x + dx * 7.0f, y + dy * 7.0f)] * 0.05f;\n"
"   Color[8] = gInput[int2(x + dx * 8.0f, y + dy * 8.0f)] * 0.03f;\n"
"   Color[9] = gInput[int2(x + dx * 9.0f, y + dy * 9.0f)] * 0.01f;\n"
"	gOutput[dtid] = Color[0] + Color[1] + Color[2] + Color[3] + Color[4] + Color[5] + Color[6] + Color[7] + Color[8] + Color[9];\n"
"}\n"

//被写界深度
"[numthreads(32, 8, 1)]\n"
"void DepthOfFieldCS(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;\n"
"   int y = dtid.y;\n"

"   float curDepth = clamp(gDepth[dtid], 0.0f, 1.0f);\n"
"   float focusDepth = g_blur.w;\n"
"   int wSize = (int)(max(0, abs(curDepth - focusDepth) - g_focusRange) * g_blur.z) + 1;\n"
"   int Size = wSize * wSize;\n"

"   float3 tmp = float3(0.0f, 0.0f, 0.0f);\n"
"   for(int j = 0; j < wSize; j++) {\n"
"     for(int i = 0; i < wSize; i++){\n"
"       int mx = max(0, x - i);\n"
"       int my = max(0, y - j);\n"
"       float4 tm = gInput[int2(mx, my)];\n"
"       tmp += tm.xyz;\n"
"     }\n"
"   }\n"
"   float4 ret = gInput[dtid];\n"
"   ret.xyz = tmp / Size;\n"
"   gOutput[dtid] = ret;\n"
"}\n";
