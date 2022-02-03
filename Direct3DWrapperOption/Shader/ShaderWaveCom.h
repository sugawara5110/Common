///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           ShaderWaveCom.hlsl                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderWaveCom =
"RWTexture2D<float> gInput : register(u0, space0);\n"
"RWTexture2D<float> gOutput : register(u1, space0);\n"
"RWTexture2D<float> gPrevInput : register(u2, space0);\n"

//wave
"cbuffer cbWave  : register(b0, space0)\n"
"{\n"
"    float4 wHei_mk012;\n"//x:waveHeight(sinWave), y:mk0, z:mk1, w:mk2
"    float2 gDisturbIndex;\n"//波紋中心
"    float gDisturbMag;\n"
"    float g_speed;\n"
"};\n"

//Dispatch(APP側)(X1, Y1, Z1)numthreads(CS側)(X, Y, Z)
//x,y,z,x1,y1,z1 は～番目
//X,Y,Z,X1,Y1,Z1 はMax値
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint その他uint3

"[numthreads(16, 16, 1)]\n"
"void sinWavesCS(int2 dtid : SV_DispatchThreadID)\n"
"{\n"
"   int x = dtid.x;\n"
"   int y = dtid.y;\n"
"	gInput[int2(x, y)].r = (gInput[int2(x, y)].r + g_speed) % 360;\n"
"	gOutput[int2(x, y)].r = (sin(gInput[int2(x, y)].r) + 1.01f) * wHei_mk012.x;\n"//1.01はマイナス防ぐ
"}\n"

"[numthreads(1, 1, 1)]\n"
"void DisturbWavesCS(int3 groupThreadID : SV_GroupThreadID, \n"
"	int3 dispatchThreadID : SV_DispatchThreadID)\n"
"{\n"
"	int x = gDisturbIndex.x;\n"
"	int y = gDisturbIndex.y;\n"

"	float halfMag = 0.5f * gDisturbMag;\n"

"	gOutput[int2(x, y)] += gDisturbMag;\n"
"	gOutput[int2(x + 1, y)] += halfMag;\n"
"	gOutput[int2(x - 1, y)] += halfMag;\n"
"	gOutput[int2(x, y + 1)] += halfMag;\n"
"	gOutput[int2(x, y - 1)] += halfMag;\n"
"}\n"

"[numthreads(16, 16, 1)]\n"
"void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)\n"
"{\n"
"	int x = dispatchThreadID.x;\n"
"	int y = dispatchThreadID.y;\n"

"	gOutput[int2(x, y)] = \n"
"		wHei_mk012.y * gPrevInput[int2(x, y)].r + \n"
"		wHei_mk012.z * gInput[int2(x, y)].r + \n"
"		wHei_mk012.w * ( \n"
"			gInput[int2(x, y + 1)].r + \n"
"			gInput[int2(x, y - 1)].r + \n"
"			gInput[int2(x + 1, y)].r + \n"
"			gInput[int2(x - 1, y)].r);\n"
"}\n";


