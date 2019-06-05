///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderNNCommon.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderNeuralNetworkTexture =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWTexture2D<float4> gOutput : register(u1); \n"

"cbuffer global2  : register(b0)\n"
"{\n"
"    float4 gWid_Hei;\n"
"};\n"

//テクスチャコピー
"[numthreads(?**, ?**, 1)]\n"//最大X * Y * Z = 1024
"void NNTexCopyCS(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   float tmp = gInput[gWid_Hei.x * inid.y + inid.x];\n"
"   float4 tmp4;\n"
"   tmp4.x = max(0, tmp);\n"
"   tmp4.y = 0.0f;\n"
"   tmp4.z = max(0, -tmp);\n"
"   tmp4.w = 1.0f;\n"
"   gOutput[inid] = tmp4;\n"
"}\n";