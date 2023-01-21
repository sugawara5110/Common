///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderBloom.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderBloom =

"StructuredBuffer<float> gGaussianFilter : register(t0, space0);\n"
"Texture2D<float4> gInput : register(t1, space0);\n"
"Texture2D<float4> gGaussTex[10] : register(t2, space1);\n"
"Texture2D<float> gInstanceIdMap : register(t3, space0);\n"
"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"RWTexture2D<float4> gGaussInOut_0 : register(u1, space0);\n"
"RWTexture2D<float4> gGaussInOut_1 : register(u2, space0);\n"
"RWTexture2D<float4> gLuminance : register(u3, space0);\n"
"SamplerState g_samLinear : register(s0, space0);\n"

"cbuffer global  : register(b0, space0)\n"
"{\n"
"    float GaussianWid;\n"//ガウス幅
"    float bloomStrength;\n"//ブルーム強さ
"    float thresholdLuminance;\n"//輝度閾値
"    float numGaussFilter;\n"//ガウスフィルター数
"    int   InstanceID;\n"//処理対象ID
"};\n"

"void getInputUV(in int2 dtid, out float2 uv)\n"
"{\n"
"   float width;\n"
"   float height;\n"
"   gInput.GetDimensions(width, height);\n"
"   uv.x = (float)dtid.x / width;\n"
"   uv.y = (float)dtid.y / height;\n"
"}\n"

"void getLuminanceUV(in int2 dtid, out float2 uv)\n"
"{\n"
"   float width;\n"
"   float height;\n"
"   gLuminance.GetDimensions(width, height);\n"
"   uv.x = (float)dtid.x / width;\n"
"   uv.y = (float)dtid.y / height;\n"
"}\n"

//輝度抽出
"[numthreads(8, 8, 1)]\n"
"void BloomCS0(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   float2 uv;\n"
"   getLuminanceUV(dtid, uv);\n"
"   float4 L = gInput.SampleLevel(g_samLinear, uv, 0);\n"
"   L.w = 0.0f;\n"

"   float width;\n"
"   float height;\n"
"   gInstanceIdMap.GetDimensions(width, height);\n"

"   int2 msize;\n"

"   msize.x = width * uv.x;\n"
"   msize.y = height * uv.y;\n"

"   float idmap = gInstanceIdMap[msize];\n"
"   int instanceId = (int)idmap;\n"

"   if(L.x + L.y + L.z > thresholdLuminance * 3.0f && \n"
"      InstanceID == instanceId)\n"
"   {\n"
"     gLuminance[dtid] = L;\n"
"   }else\n"
"   {\n"
"     gLuminance[dtid] = float4(0.0f ,0.0f ,0.0f ,0.0f);\n"
"   }\n"
"}\n"

//Bloom横
"[numthreads(8, 8, 1)]\n"
"void BloomCS1(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;\n"
"   int y = dtid.y;\n"

"   int halfwid = GaussianWid;\n"
"   float3 col = float3(0.0f, 0.0f, 0.0f);\n"

"   for(int i = 0; i < halfwid; i++){\n"
"     float4 tmp = gLuminance[int2(x + i, y)];\n"
"     col += tmp.xyz * gGaussianFilter[i];\n"
"     tmp = gLuminance[int2(x - i, y)];\n"
"     col += tmp.xyz * gGaussianFilter[i];\n"
"   }\n"

"   float4 Out = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
"   Out.xyz = col;\n"
"   gGaussInOut_0[dtid] = Out;\n"
"}\n"

//Bloom縦
"[numthreads(8, 8, 1)]\n"
"void BloomCS2(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;\n"
"   int y = dtid.y;\n"

"   int halfwid = GaussianWid;\n"
"   float3 col = float3(0.0f, 0.0f, 0.0f);\n"

"   for(int i = 0; i < halfwid; i++){\n"
"     float4 tmp = gGaussInOut_0[int2(x, y + i)];\n"
"     col += tmp.xyz * gGaussianFilter[i];\n"
"     tmp = gGaussInOut_0[int2(x, y - i)];\n"
"     col += tmp.xyz * gGaussianFilter[i];\n"
"   }\n"

"   float4 Out = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
"   Out.xyz = col;\n"
"   gGaussInOut_1[dtid] = Out;\n"
"}\n"

//Bloom横縦
"[numthreads(8, 8, 1)]\n"
"void BloomCS12(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;\n"
"   int y = dtid.y;\n"

"   int halfwid = GaussianWid;\n"
"   float3 col = float3(0.0f, 0.0f, 0.0f);\n"
"   for(int j = 0; j < halfwid; j++){\n"
"      for(int i = 0; i < halfwid; i++){\n"
"        int gauInd = halfwid * j + i;\n"
"        float4 tmp = gLuminance[int2(x + i, y + j)];\n"
"        col += tmp.xyz * gGaussianFilter[gauInd];\n"
"        tmp = gLuminance[int2(x - i, y + j)];\n"
"        col += tmp.xyz * gGaussianFilter[gauInd];\n"
"        tmp = gLuminance[int2(x + i, y - j)];\n"
"        col += tmp.xyz * gGaussianFilter[gauInd];\n"
"        tmp = gLuminance[int2(x - i, y - j)];\n"
"        col += tmp.xyz * gGaussianFilter[gauInd];\n"
"      }\n"
"   }\n"
"   float4 Out = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
"   Out.xyz = col;\n"
"   gGaussInOut_1[dtid] = Out;\n"
"}\n"

"[numthreads(32, 8, 1)]\n"
"void BloomCS3(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   float2 uv;\n"
"   getInputUV(dtid, uv);\n"

"   int numGaus = (int)numGaussFilter;\n"
"   float4 gau = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
"   for(int i = 0; i < numGaus; i++){\n"
"      gau += gGaussTex[i].SampleLevel(g_samLinear, uv, 0);\n"
"   }\n"
"   gau = gau / numGaussFilter * bloomStrength;\n"
"   gOutput[dtid] = gau;\n"
"}\n";

char* ShaderBloom2 =
"Texture2D<float4> gInput : register(t0, space0);\n"
"Texture2D<float4> gBloom[replace_NUM_Bloom] : register(t1, space1);\n"
"RWTexture2D<float4> gOutput : register(u0, space0);\n"
"SamplerState g_samLinear : register(s0, space0);\n"

"cbuffer global  : register(b0, space0)\n"
"{\n"
"    int numInstance;\n"
"};\n"

//メッシュ毎にブルーム処理した値をメインバッファに加算
"[numthreads(32, 8, 1)]\n"
"void BloomCS4(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   float4 b = float4(0.0f, 0.0f, 0.0f, 0.0);\n"

"   for(int i = 0; i < numInstance; i++){\n"
"      float4 bloom = gBloom[i][dtid];\n"
"      b += bloom;\n"
"   }\n"
"   gOutput[dtid] = gInput[dtid] + b;\n"
"}\n";